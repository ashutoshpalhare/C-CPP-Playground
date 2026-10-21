// Multithreaded Download Manager (Advanced C++)
//
// Downloads several files concurrently and, when the server supports HTTP
// Range requests, splits each file into segments fetched by parallel
// threads that write into the same pre-sized output file.
//
//   dlm [-j N] [-s S] [-o DIR] <url-or-path> [more...]
//   dlm -f urls.txt
//
//   -j N   maximum concurrent downloads (default 3)
//   -s S   segments per file when ranges are supported (default 4)
//   -o DIR output directory (default current directory)
//   -f F   read one URL per line from F
//
// Supported sources (no external libraries, so no TLS):
//   http://host[:port]/path    plain HTTP/1.1 (NOT https://)
//   file:///abs/path or a plain local path   copied through the same
//                               segmented pipeline - handy for offline tests
//
// Concepts: std::thread, std::mutex, std::condition_variable, std::atomic,
// a thread pool with a job queue, RAII socket wrapper, HTTP parsing,
// positioned file writes, progress reporting, retries.
//
// Platform: Windows 10/11 (Winsock2, link -lws2_32), Linux/macOS/WSL (-pthread).
// Standard: C++17.

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0601
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
using socket_t = SOCKET;
constexpr socket_t kBadSocket = INVALID_SOCKET;
inline void closeSocket(socket_t s) { closesocket(s); }
#else
#  include <netdb.h>
#  include <sys/socket.h>
#  include <unistd.h>
using socket_t = int;
constexpr socket_t kBadSocket = -1;
inline void closeSocket(socket_t s) { close(s); }
#endif

namespace {

// ===================== small utilities =====================

struct NetInit {
    NetInit() {
#ifdef _WIN32
        WSADATA w;
        if (WSAStartup(MAKEWORD(2, 2), &w) != 0) throw std::runtime_error("WSAStartup failed");
#endif
    }
    ~NetInit() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

std::mutex g_printMutex;
void log(const std::string& line) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << line << "\n" << std::flush;
}

std::string humanBytes(long long b) {
    const char* units[] = {"B", "KiB", "MiB", "GiB"};
    double v = static_cast<double>(b);
    int u = 0;
    while (v >= 1024 && u < 3) { v /= 1024; ++u; }
    std::ostringstream os;
    os << std::fixed << std::setprecision(u ? 1 : 0) << v << ' ' << units[u];
    return os.str();
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// ===================== URL parsing =====================

struct Url {
    bool local = false;  // local path or file://
    std::string host, path, filename;
    int port = 80;
};

std::optional<Url> parseUrl(const std::string& text) {
    Url u;
    if (text.rfind("file://", 0) == 0) {
        u.local = true;
        u.path = text.substr(7);
    } else if (text.rfind("http://", 0) == 0) {
        std::string rest = text.substr(7);
        const auto slash = rest.find('/');
        std::string hostport = rest.substr(0, slash);
        u.path = slash == std::string::npos ? "/" : rest.substr(slash);
        const auto colon = hostport.find(':');
        if (colon != std::string::npos) {
            try { u.port = std::stoi(hostport.substr(colon + 1)); } catch (...) { return std::nullopt; }
            if (u.port < 1 || u.port > 65535) return std::nullopt;
            hostport = hostport.substr(0, colon);
        }
        if (hostport.empty()) return std::nullopt;
        u.host = hostport;
    } else if (text.rfind("https://", 0) == 0) {
        return std::nullopt;  // TLS not supported without external libraries
    } else {
        u.local = true;
        u.path = text;
    }
    if (u.path.empty()) return std::nullopt;
    const std::string noQuery = u.path.substr(0, u.path.find('?'));
    const auto last = noQuery.find_last_of("/\\");
    u.filename = last == std::string::npos ? noQuery : noQuery.substr(last + 1);
    if (u.filename.empty()) u.filename = "index.html";
    // keep filenames safe
    for (char& c : u.filename) if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') c = '_';
    return u;
}

// ===================== HTTP client (minimal) =====================

class Socket {
public:
    Socket() = default;
    ~Socket() { if (s_ != kBadSocket) closeSocket(s_); }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    bool connect(const std::string& host, int port) {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0 || !res) return false;
        for (addrinfo* ai = res; ai; ai = ai->ai_next) {
            s_ = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (s_ == kBadSocket) continue;
            if (::connect(s_, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0) break;
            closeSocket(s_);
            s_ = kBadSocket;
        }
        freeaddrinfo(res);
        return s_ != kBadSocket;
    }
    bool sendAll(const std::string& data) {
        std::size_t sent = 0;
        while (sent < data.size()) {
            const int n = static_cast<int>(send(s_, data.data() + sent, static_cast<int>(data.size() - sent), 0));
            if (n <= 0) return false;
            sent += static_cast<std::size_t>(n);
        }
        return true;
    }
    int recv(char* buf, int len) { return static_cast<int>(::recv(s_, buf, len, 0)); }

private:
    socket_t s_ = kBadSocket;
};

struct HttpResponse {
    int status = 0;
    std::map<std::string, std::string> headers;  // lower-case keys
    std::string bodyPrefix;                       // bytes received after the headers
};

// Sends a request and parses the status line + headers. Body bytes beyond the
// header block are returned in bodyPrefix; the rest is streamed by the caller.
bool httpRequest(Socket& sock, const Url& u, const std::string& method, const std::string& extraHeaders, HttpResponse& out) {
    std::ostringstream req;
    req << method << ' ' << u.path << " HTTP/1.1\r\nHost: " << u.host << "\r\nUser-Agent: c-cpp-playground-dlm/1.0\r\nConnection: close\r\n" << extraHeaders << "\r\n";
    if (!sock.sendAll(req.str())) return false;
    std::string buf;
    char chunk[4096];
    std::size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        const int n = sock.recv(chunk, sizeof chunk);
        if (n <= 0) return false;
        buf.append(chunk, static_cast<std::size_t>(n));
        headerEnd = buf.find("\r\n\r\n");
        if (buf.size() > 64 * 1024) return false;  // absurd header block
    }
    std::istringstream hs(buf.substr(0, headerEnd));
    std::string line;
    std::getline(hs, line);
    if (line.compare(0, 5, "HTTP/") != 0 || line.size() < 12) return false;
    try { out.status = std::stoi(line.substr(9, 3)); } catch (...) { return false; }
    while (std::getline(hs, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = lower(line.substr(0, colon)), val = line.substr(colon + 1);
        val.erase(0, val.find_first_not_of(' '));
        out.headers[key] = val;
    }
    out.bodyPrefix = buf.substr(headerEnd + 4);
    return true;
}

// ===================== download job =====================

struct Job {
    Url url;
    std::string outputPath;
    long long size = -1;          // -1 unknown
    bool rangesOk = false;
    std::atomic<long long> done{0};
    std::atomic<bool> failed{false};
    std::string error;
    std::mutex errorMutex;

    void fail(const std::string& why) {
        std::lock_guard<std::mutex> lock(errorMutex);
        if (!failed.exchange(true)) error = why;
    }
};

// Writes a byte range into the output file at the right offset.
class OutputFile {
public:
    explicit OutputFile(const std::string& path) : fp_(std::fopen(path.c_str(), "wb")) {}
    ~OutputFile() { if (fp_) std::fclose(fp_); }
    bool ok() const { return fp_ != nullptr; }
    bool preallocate(long long size) {
        if (size <= 0) return true;
        if (std::fseek(fp_, static_cast<long>(size - 1), SEEK_SET) != 0) return false;
        return std::fputc(0, fp_) != EOF && std::fflush(fp_) == 0;
    }
    bool writeAt(long long offset, const char* data, std::size_t len) {
        std::lock_guard<std::mutex> lock(m_);
        if (std::fseek(fp_, static_cast<long>(offset), SEEK_SET) != 0) return false;
        return std::fwrite(data, 1, len, fp_) == len;
    }
    bool close() {
        const bool ok = fp_ && std::fclose(fp_) == 0;
        fp_ = nullptr;
        return ok;
    }
private:
    std::FILE* fp_;
    std::mutex m_;
};

// Streams `count` bytes (or until EOF when count<0) from an HTTP body into the file.
bool streamBody(Socket& sock, const std::string& prefix, OutputFile& out, long long offset, long long count, Job& job) {
    long long written = 0;
    auto write = [&](const char* data, std::size_t len) {
        if (count >= 0 && written + static_cast<long long>(len) > count) len = static_cast<std::size_t>(count - written);
        if (len == 0) return true;
        if (!out.writeAt(offset + written, data, len)) return false;
        written += static_cast<long long>(len);
        job.done += static_cast<long long>(len);
        return true;
    };
    if (!prefix.empty() && !write(prefix.data(), prefix.size())) return false;
    char buf[16384];
    while (count < 0 || written < count) {
        const int n = sock.recv(buf, sizeof buf);
        if (n <= 0) break;
        if (!write(buf, static_cast<std::size_t>(n))) return false;
    }
    return count < 0 || written == count;
}

// ----- local file source (for offline testing of the same pipeline) -----
bool copyLocalRange(const std::string& src, OutputFile& out, long long offset, long long count, Job& job) {
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    in.seekg(offset);
    char buf[16384];
    long long remaining = count;
    while (remaining > 0 && in) {
        in.read(buf, static_cast<std::streamsize>(std::min<long long>(remaining, sizeof buf)));
        const auto got = in.gcount();
        if (got <= 0) break;
        if (!out.writeAt(offset + (count - remaining), buf, static_cast<std::size_t>(got))) return false;
        remaining -= got;
        job.done += got;
    }
    return remaining == 0;
}

// ===================== thread pool =====================

class ThreadPool {
public:
    explicit ThreadPool(std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) workers_.emplace_back([this] { loop(); });
    }
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(m_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }
    void submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_);
            tasks_.push_back(std::move(task));
        }
        cv_.notify_one();
    }
    void wait() {
        std::unique_lock<std::mutex> lock(m_);
        idle_.wait(lock, [this] { return tasks_.empty() && active_ == 0; });
    }

private:
    std::vector<std::thread> workers_;
    std::vector<std::function<void()>> tasks_;
    std::mutex m_;
    std::condition_variable cv_, idle_;
    std::size_t active_ = 0;
    bool stop_ = false;

    void loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.erase(tasks_.begin());
                ++active_;
            }
            try { task(); } catch (const std::exception& e) { log(std::string("worker exception: ") + e.what()); }
            {
                std::lock_guard<std::mutex> lock(m_);
                --active_;
            }
            idle_.notify_all();
        }
    }
};

// ===================== download logic =====================

bool probe(Job& job) {
    if (job.url.local) {
        std::ifstream in(job.url.path, std::ios::binary | std::ios::ate);
        if (!in) { job.fail("cannot open local file"); return false; }
        job.size = static_cast<long long>(in.tellg());
        job.rangesOk = true;
        return true;
    }
    Socket s;
    if (!s.connect(job.url.host, job.url.port)) { job.fail("connect failed"); return false; }
    HttpResponse r;
    if (!httpRequest(s, job.url, "HEAD", "", r)) { job.fail("bad HEAD response"); return false; }
    if (r.status >= 300 && r.status < 400) { job.fail("redirect (" + std::to_string(r.status) + ") not followed - use the final URL"); return false; }
    if (r.status != 200) { job.fail("HTTP " + std::to_string(r.status)); return false; }
    auto cl = r.headers.find("content-length");
    if (cl != r.headers.end()) { try { job.size = std::stoll(cl->second); } catch (...) { job.size = -1; } }
    auto ar = r.headers.find("accept-ranges");
    job.rangesOk = job.size > 0 && ar != r.headers.end() && lower(ar->second) == "bytes";
    return true;
}

bool fetchSegment(Job& job, OutputFile& out, long long start, long long end /*inclusive*/) {
    for (int attempt = 1; attempt <= 3; ++attempt) {
        const long long alreadyDone = job.done.load();
        bool ok;
        if (job.url.local) {
            ok = copyLocalRange(job.url.path, out, start, end - start + 1, job);
        } else {
            Socket s;
            HttpResponse r;
            ok = s.connect(job.url.host, job.url.port) &&
                 httpRequest(s, job.url, "GET", "Range: bytes=" + std::to_string(start) + "-" + std::to_string(end) + "\r\n", r) &&
                 r.status == 206 && streamBody(s, r.bodyPrefix, out, start, end - start + 1, job);
        }
        if (ok) return true;
        job.done = alreadyDone;  // roll back progress for this segment
        log("  retry " + std::to_string(attempt) + " for segment " + std::to_string(start) + "-" + std::to_string(end) + " of " + job.url.filename);
        std::this_thread::sleep_for(std::chrono::milliseconds(300 * attempt));
    }
    return false;
}

bool fetchWhole(Job& job, OutputFile& out) {
    if (job.url.local) {
        std::ifstream in(job.url.path, std::ios::binary | std::ios::ate);
        if (!in) return false;
        return copyLocalRange(job.url.path, out, 0, static_cast<long long>(in.tellg()), job);
    }
    Socket s;
    HttpResponse r;
    if (!s.connect(job.url.host, job.url.port) || !httpRequest(s, job.url, "GET", "", r) || r.status != 200) return false;
    const bool chunked = r.headers.count("transfer-encoding") && lower(r.headers["transfer-encoding"]).find("chunked") != std::string::npos;
    if (chunked) { job.fail("chunked transfer encoding is not supported by this simple client"); return false; }
    return streamBody(s, r.bodyPrefix, out, 0, job.size, job);
}

void download(Job& job, int segments, ThreadPool& segmentPool) {
    const auto t0 = std::chrono::steady_clock::now();
    if (!probe(job)) { log("FAILED " + job.url.filename + ": " + job.error); return; }
    log("start  " + job.url.filename + "  size " + (job.size >= 0 ? humanBytes(job.size) : "unknown") + (job.rangesOk ? "  (ranges supported, " + std::to_string(segments) + " segments)" : "  (single stream)"));

    OutputFile out(job.outputPath);
    if (!out.ok()) { job.fail("cannot create " + job.outputPath); log("FAILED " + job.url.filename + ": " + job.error); return; }

    if (job.rangesOk && segments > 1 && job.size >= segments) {
        if (!out.preallocate(job.size)) { job.fail("cannot preallocate output"); log("FAILED " + job.url.filename); return; }
        const long long per = job.size / segments;
        std::atomic<int> remaining{segments};
        std::mutex doneMutex;
        std::condition_variable doneCv;
        for (int i = 0; i < segments; ++i) {
            const long long start = i * per;
            const long long end = (i == segments - 1) ? job.size - 1 : start + per - 1;
            segmentPool.submit([&, start, end] {
                if (!fetchSegment(job, out, start, end)) job.fail("segment " + std::to_string(start) + "-" + std::to_string(end) + " failed");
                if (--remaining == 0) { std::lock_guard<std::mutex> lock(doneMutex); doneCv.notify_all(); }
            });
        }
        std::unique_lock<std::mutex> lock(doneMutex);
        doneCv.wait(lock, [&] { return remaining == 0; });
    } else {
        if (!fetchWhole(job, out)) job.fail(job.error.empty() ? "download failed" : job.error);
    }

    if (!out.close()) job.fail("closing output file failed");
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (job.failed) {
        std::remove(job.outputPath.c_str());
        log("FAILED " + job.url.filename + ": " + job.error);
    } else {
        std::ostringstream os;
        os << "done   " << job.url.filename << "  " << humanBytes(job.done) << " in " << std::fixed << std::setprecision(2) << secs << " s";
        if (secs > 0) os << "  (" << humanBytes(static_cast<long long>(job.done / secs)) << "/s)";
        log(os.str());
    }
}

void usage() {
    std::cerr << "Usage: dlm [-j N] [-s S] [-o DIR] <http-url | file-path> [...]\n"
                 "       dlm [-j N] [-s S] [-o DIR] -f urls.txt\n"
                 "Only plain http:// and local files are supported (no https).\n";
}

}  // namespace

int main(int argc, char** argv) {
    int maxJobs = 3, segments = 4;
    std::string outDir = ".";
    std::vector<std::string> sources;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto needValue = [&](int& iref) -> const char* { if (iref + 1 >= argc) { usage(); std::exit(1); } return argv[++iref]; };
        try {
            if (a == "-j") maxJobs = std::stoi(needValue(i));
            else if (a == "-s") segments = std::stoi(needValue(i));
            else if (a == "-o") outDir = needValue(i);
            else if (a == "-f") {
                std::ifstream list(needValue(i));
                if (!list) { std::cerr << "Cannot open URL list file.\n"; return 1; }
                std::string line;
                while (std::getline(list, line)) { if (!line.empty() && line[0] != '#') sources.push_back(line); }
            } else if (a == "-h" || a == "--help") { usage(); return 0; }
            else sources.push_back(a);
        } catch (const std::exception&) { usage(); return 1; }
    }
    if (sources.empty() || maxJobs < 1 || maxJobs > 32 || segments < 1 || segments > 16) { usage(); return 1; }

    try {
        NetInit net;
        std::vector<std::unique_ptr<Job>> jobs;
        for (const std::string& s : sources) {
            auto url = parseUrl(s);
            if (!url) { log("skip   " + s + "  (unsupported or malformed URL; https is not supported)"); continue; }
            auto job = std::make_unique<Job>();
            job->url = *url;
            job->outputPath = outDir + "/" + url->filename;
            jobs.push_back(std::move(job));
        }
        if (jobs.empty()) return 1;

        ThreadPool jobPool(static_cast<std::size_t>(std::min<int>(maxJobs, static_cast<int>(jobs.size()))));
        ThreadPool segmentPool(static_cast<std::size_t>(maxJobs * segments));
        std::atomic<bool> finished{false};

        // Progress reporter thread.
        std::thread reporter([&] {
            while (!finished) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (finished) break;
                std::ostringstream os;
                os << "progress:";
                for (const auto& j : jobs) {
                    os << "  " << j->url.filename << ' ';
                    if (j->size > 0) os << (100 * j->done / j->size) << '%';
                    else os << humanBytes(j->done);
                }
                log(os.str());
            }
        });

        for (auto& j : jobs) jobPool.submit([&, jp = j.get()] { download(*jp, segments, segmentPool); });
        jobPool.wait();
        finished = true;
        reporter.join();

        int failed = 0;
        for (const auto& j : jobs) if (j->failed) ++failed;
        log("Finished: " + std::to_string(jobs.size() - static_cast<std::size_t>(failed)) + " ok, " + std::to_string(failed) + " failed.");
        return failed ? 2 : 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}

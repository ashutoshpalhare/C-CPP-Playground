// Chat Application using Sockets (Advanced C++)
//
// A single executable that runs either as a chat SERVER or a chat CLIENT:
//
//   chat server [port]                 start a server (default port 5555)
//   chat client <host> [port] [name]   connect to a server
//
// Server: accepts many clients, one thread per client, broadcasts messages,
//         supports nicknames and /list, /nick, /quit commands.
// Client: one thread reads from the socket, the main thread reads stdin.
//
// Wire protocol: UTF-8/ASCII text, one message per line ('\n' terminated).
//
// Platform support:
//   - Windows 10/11: Winsock2. Build with MinGW: add -lws2_32. MSVC: ws2_32.lib
//   - Linux / macOS / WSL: BSD sockets + pthreads (-pthread)
//
// Standard: C++17 (std::thread, std::mutex, std::atomic).

#include <atomic>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
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
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
inline void closeSocket(socket_t s) { closesocket(s); }
inline int lastError() { return WSAGetLastError(); }
#else
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <cerrno>
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
inline void closeSocket(socket_t s) { close(s); }
inline int lastError() { return errno; }
#endif

namespace {

constexpr int kDefaultPort = 5555;
constexpr std::size_t kMaxLine = 1024;

struct NetInit {
    bool ok = true;
    NetInit() {
#ifdef _WIN32
        WSADATA wsa;
        ok = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
        if (!ok) std::cerr << "WSAStartup failed\n";
#endif
    }
    ~NetInit() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

bool sendAll(socket_t s, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int n = static_cast<int>(send(s, data.data() + sent, static_cast<int>(data.size() - sent), 0));
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

// Reads one '\n'-terminated line (without the newline). Returns false on disconnect.
bool recvLine(socket_t s, std::string& buffer, std::string& line) {
    for (;;) {
        const auto pos = buffer.find('\n');
        if (pos != std::string::npos) {
            line = buffer.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            buffer.erase(0, pos + 1);
            return true;
        }
        if (buffer.size() > kMaxLine * 4) return false;  // abusive client: no newline for 4 KiB
        char chunk[512];
        const int n = static_cast<int>(recv(s, chunk, sizeof chunk, 0));
        if (n <= 0) return false;
        buffer.append(chunk, static_cast<std::size_t>(n));
    }
}

bool parsePort(const std::string& text, int& port) {
    try {
        std::size_t used = 0;
        port = std::stoi(text, &used);
        return used == text.size() && port >= 1 && port <= 65535;
    } catch (const std::exception&) {
        return false;
    }
}

std::string sanitise(std::string s) {
    for (char& c : s) if ((c < 32 && c != '\t') || c == 127) c = '?';
    if (s.size() > kMaxLine) s.resize(kMaxLine);
    return s;
}

// ===================== SERVER =====================

class ChatServer {
public:
    explicit ChatServer(int port) : port_(port) {}

    int run() {
        socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
        if (listener == kInvalidSocket) { std::cerr << "socket() failed: " << lastError() << "\n"; return 1; }
        int yes = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof yes);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(static_cast<unsigned short>(port_));
        if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
            std::cerr << "bind() failed on port " << port_ << " (error " << lastError() << "). Port in use?\n";
            closeSocket(listener);
            return 1;
        }
        if (listen(listener, 16) != 0) { std::cerr << "listen() failed: " << lastError() << "\n"; closeSocket(listener); return 1; }
        std::cout << "Chat server listening on port " << port_ << ". Press Ctrl+C to stop.\n";

        for (;;) {
            sockaddr_in peer{};
            socklen_t len = sizeof peer;
            socket_t client = accept(listener, reinterpret_cast<sockaddr*>(&peer), &len);
            if (client == kInvalidSocket) { std::cerr << "accept() failed: " << lastError() << "\n"; continue; }
            char ip[INET_ADDRSTRLEN] = "?";
            inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof ip);
            std::cout << "Connection from " << ip << ":" << ntohs(peer.sin_port) << "\n";
            std::thread(&ChatServer::handleClient, this, client).detach();
        }
    }

private:
    int port_;
    std::mutex mutex_;
    std::map<socket_t, std::string> clients_;  // socket -> nickname
    int anonCounter_ = 0;

    void broadcast(const std::string& line, socket_t except = kInvalidSocket) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << line << "\n";
        for (auto it = clients_.begin(); it != clients_.end();) {
            if (it->first != except && !sendAll(it->first, line + "\n")) {
                closeSocket(it->first);
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool nickInUse(const std::string& nick) {
        for (const auto& [s, n] : clients_) if (n == nick) return true;
        return false;
    }

    std::string listUsers() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string out = "* Online (" + std::to_string(clients_.size()) + "):";
        for (const auto& [s, n] : clients_) out += " " + n;
        return out;
    }

    void handleClient(socket_t sock) {
        std::string nick;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            nick = "guest" + std::to_string(++anonCounter_);
            clients_[sock] = nick;
        }
        sendAll(sock, "* Welcome! You are " + nick + ". Commands: /nick <name>, /list, /quit\n");
        broadcast("* " + nick + " joined the chat", sock);

        std::string buffer, line;
        while (recvLine(sock, buffer, line)) {
            line = sanitise(line);
            if (line.empty()) continue;
            if (line[0] == '/') {
                std::string cmd = line.substr(0, line.find(' '));
                std::string arg = line.size() > cmd.size() ? line.substr(cmd.size() + 1) : "";
                if (cmd == "/quit") break;
                if (cmd == "/list") { sendAll(sock, listUsers() + "\n"); continue; }
                if (cmd == "/nick") {
                    if (arg.empty() || arg.size() > 20 || arg.find(' ') != std::string::npos) { sendAll(sock, "* Usage: /nick <name without spaces, max 20 chars>\n"); continue; }
                    std::string old;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        if (nickInUse(arg)) { sendAll(sock, "* That name is taken.\n"); continue; }
                        old = clients_[sock];
                        clients_[sock] = arg;
                    }
                    nick = arg;
                    broadcast("* " + old + " is now known as " + nick);
                    continue;
                }
                sendAll(sock, "* Unknown command " + cmd + "\n");
                continue;
            }
            broadcast("[" + nick + "] " + line);
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            clients_.erase(sock);
        }
        closeSocket(sock);
        broadcast("* " + nick + " left the chat");
    }
};

// ===================== CLIENT =====================

int runClient(const std::string& host, int port, const std::string& name) {
    addrinfo hints{}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    const int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (rc != 0 || result == nullptr) { std::cerr << "Could not resolve host '" << host << "'\n"; return 1; }

    socket_t sock = kInvalidSocket;
    for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock == kInvalidSocket) continue;
        if (connect(sock, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) == 0) break;
        closeSocket(sock);
        sock = kInvalidSocket;
    }
    freeaddrinfo(result);
    if (sock == kInvalidSocket) { std::cerr << "Could not connect to " << host << ":" << port << " (error " << lastError() << ")\n"; return 1; }
    std::cout << "Connected to " << host << ":" << port << ". Type messages and press Enter. /quit to leave.\n";
    if (!name.empty()) sendAll(sock, "/nick " + name + "\n");

    std::atomic<bool> running{true};
    std::thread reader([&]() {
        std::string buffer, line;
        while (running && recvLine(sock, buffer, line)) std::cout << line << "\n" << std::flush;
        if (running) std::cout << "* Disconnected from server. Press Enter to exit.\n";
        running = false;
    });

    std::string input;
    while (running && std::getline(std::cin, input)) {
        if (input.empty()) continue;
        if (!sendAll(sock, sanitise(input) + "\n")) break;
        if (input == "/quit") break;
    }
    running = false;
#ifdef _WIN32
    shutdown(sock, SD_BOTH);
#else
    shutdown(sock, SHUT_RDWR);
#endif
    closeSocket(sock);
    reader.join();
    std::cout << "Bye.\n";
    return 0;
}

void usage(const char* prog) {
    std::cerr << "Usage:\n  " << prog << " server [port]\n  " << prog << " client <host> [port] [nickname]\n"
              << "Default port: " << kDefaultPort << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    NetInit net;
    if (!net.ok) return 1;
    if (argc < 2) { usage(argv[0]); return 1; }
    const std::string mode = argv[1];
    int port = kDefaultPort;

    if (mode == "server") {
        if (argc >= 3 && !parsePort(argv[2], port)) { std::cerr << "Invalid port.\n"; return 1; }
        ChatServer server(port);
        return server.run();
    }
    if (mode == "client") {
        if (argc < 3) { usage(argv[0]); return 1; }
        if (argc >= 4 && !parsePort(argv[3], port)) { std::cerr << "Invalid port.\n"; return 1; }
        const std::string name = argc >= 5 ? argv[4] : "";
        return runClient(argv[2], port, name);
    }
    usage(argv[0]);
    return 1;
}

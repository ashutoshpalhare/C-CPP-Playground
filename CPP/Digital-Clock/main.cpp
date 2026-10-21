// Digital Clock (Beginner C++)
//
// Displays a live clock in the terminal that refreshes every second, with
// 12/24-hour modes, an optional stopwatch and countdown timer.
//
// Terminal requirements: the clock redraws itself using ANSI escape codes.
//   - Linux / macOS / WSL terminals support these natively.
//   - Windows 10 (1511+) / 11 consoles support them once virtual terminal
//     processing is enabled, which this program does automatically.
//   - On older Windows consoles the program falls back to printing a new line
//     every second instead of redrawing in place.
//
// Standard: C++17 (uses <chrono>, <thread>, <ctime>).

#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#  include <conio.h>
#  include <windows.h>
#else
#  include <sys/select.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace {

bool g_ansi = true;

// ---------- platform glue ----------

#ifdef _WIN32
void setupTerminal() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode) && SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        g_ansi = true;
    } else {
        g_ansi = false;  // legacy console: no in-place redraw
    }
}
void restoreTerminal() {}
// Returns the pressed key, or 0 if none is waiting.
int pollKey() {
    if (_kbhit()) return _getch();
    return 0;
}
#else
termios g_original{};
bool g_rawEnabled = false;
void setupTerminal() {
    if (tcgetattr(STDIN_FILENO, &g_original) == 0) {
        termios raw = g_original;
        raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) g_rawEnabled = true;
    }
}
void restoreTerminal() {
    if (g_rawEnabled) tcsetattr(STDIN_FILENO, TCSANOW, &g_original);
}
int pollKey() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{0, 0};
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
        unsigned char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
    return 0;
}
#endif

struct TerminalGuard {
    TerminalGuard() { setupTerminal(); }
    ~TerminalGuard() { restoreTerminal(); }
};

// ---------- formatting ----------

std::tm localNow() {
    const std::time_t t = std::time(nullptr);
    std::tm out{};
#ifdef _WIN32
    localtime_s(&out, &t);
#else
    localtime_r(&t, &out);
#endif
    return out;
}

std::string formatTime(const std::tm& tm, bool twelveHour) {
    std::ostringstream os;
    int hour = tm.tm_hour;
    std::string suffix;
    if (twelveHour) {
        suffix = hour >= 12 ? " PM" : " AM";
        hour %= 12;
        if (hour == 0) hour = 12;
    }
    os << std::setfill('0') << std::setw(2) << hour << ':' << std::setw(2) << tm.tm_min << ':' << std::setw(2) << tm.tm_sec << suffix;
    return os.str();
}

std::string formatDate(const std::tm& tm) {
    static const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    static const char* months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    std::ostringstream os;
    os << days[tm.tm_wday] << ", " << tm.tm_mday << ' ' << months[tm.tm_mon] << ' ' << (tm.tm_year + 1900);
    return os.str();
}

std::string formatDuration(std::chrono::milliseconds ms) {
    using namespace std::chrono;
    const auto h = duration_cast<hours>(ms); ms -= h;
    const auto m = duration_cast<minutes>(ms); ms -= m;
    const auto s = duration_cast<seconds>(ms); ms -= s;
    std::ostringstream os;
    os << std::setfill('0') << std::setw(2) << h.count() << ':' << std::setw(2) << m.count() << ':' << std::setw(2) << s.count() << '.' << std::setw(1) << ms.count() / 100;
    return os.str();
}

// Big 5x3 digits for a nicer display.
const char* const kGlyphs[11][3] = {
    {" _ ", "| |", "|_|"}, {"   ", "  |", "  |"}, {" _ ", " _|", "|_ "}, {" _ ", " _|", " _|"}, {"   ", "|_|", "  |"},
    {" _ ", "|_ ", " _|"}, {" _ ", "|_ ", "|_|"}, {" _ ", "  |", "  |"}, {" _ ", "|_|", "|_|"}, {" _ ", "|_|", " _|"},
    {"   ", " . ", " . "}};  // index 10 = colon

std::string bigText(const std::string& digits) {
    std::string rows[3];
    for (char c : digits) {
        int idx = -1;
        if (c >= '0' && c <= '9') idx = c - '0';
        else if (c == ':') idx = 10;
        for (int r = 0; r < 3; ++r) {
            rows[r] += (idx >= 0) ? kGlyphs[idx][r] : std::string(1, c) + "  ";
            rows[r] += ' ';
        }
    }
    return rows[0] + '\n' + rows[1] + '\n' + rows[2] + '\n';
}

void clearScreen() {
    if (g_ansi) std::cout << "\x1b[2J\x1b[H";
}

void drawFrame(const std::string& title, const std::string& body, const std::string& footer) {
    if (g_ansi) {
        std::cout << "\x1b[H";  // home, redraw in place
        std::cout << title << "\x1b[K\n\n" << body << "\x1b[K\n" << footer << "\x1b[K\n";
        std::cout << "\x1b[J";  // clear remainder
    } else {
        std::cout << title << "  " << footer << '\n';
    }
    std::cout.flush();
}

// ---------- modes ----------

void runClock() {
    bool twelveHour = false;
    clearScreen();
    while (true) {
        const std::tm tm = localNow();
        const std::string time = formatTime(tm, twelveHour);
        const std::string body = g_ansi ? bigText(time.substr(0, 8)) + (twelveHour ? time.substr(8) : "") + "\n" + formatDate(tm) : time + "  " + formatDate(tm);
        drawFrame("=== Digital Clock ===", body, "[h] toggle 12/24h   [q] back to menu");
        for (int i = 0; i < 10; ++i) {  // poll keys 10x per second, redraw once per second
            const int key = pollKey();
            if (key == 'q' || key == 'Q') { clearScreen(); return; }
            if (key == 'h' || key == 'H') { twelveHour = !twelveHour; break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void runStopwatch() {
    using clock = std::chrono::steady_clock;
    auto start = clock::now();
    std::chrono::milliseconds accumulated{0};
    bool running = true;
    clearScreen();
    while (true) {
        const auto elapsed = accumulated + (running ? std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start) : std::chrono::milliseconds{0});
        drawFrame("=== Stopwatch ===", formatDuration(elapsed) + (running ? "  (running)" : "  (paused)"), "[space] start/pause   [r] reset   [q] back");
        const int key = pollKey();
        if (key == 'q' || key == 'Q') { clearScreen(); return; }
        if (key == ' ') {
            if (running) { accumulated = elapsed; running = false; }
            else { start = clock::now(); running = true; }
        }
        if (key == 'r' || key == 'R') { accumulated = std::chrono::milliseconds{0}; start = clock::now(); }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void runCountdown(int seconds) {
    using clock = std::chrono::steady_clock;
    const auto end = clock::now() + std::chrono::seconds(seconds);
    clearScreen();
    while (true) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(end - clock::now());
        if (remaining.count() <= 0) {
            drawFrame("=== Countdown ===", "00:00:00.0\n\nTime's up!\a", "Press any key to return");
            while (pollKey() == 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            clearScreen();
            return;
        }
        drawFrame("=== Countdown ===", formatDuration(remaining), "[q] cancel");
        const int key = pollKey();
        if (key == 'q' || key == 'Q') { clearScreen(); return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Line-based input for the menu (terminal is restored temporarily on POSIX).
int readMenuChoice() {
    std::string line;
    restoreTerminal();
    std::cout << "\n1. Live clock\n2. Stopwatch\n3. Countdown timer\n0. Quit\nChoose: " << std::flush;
    if (!std::getline(std::cin, line)) { return -1; }
    setupTerminal();
    try {
        std::size_t used = 0;
        const int v = std::stoi(line, &used);
        if (used == line.size()) return v;
    } catch (const std::exception&) {
    }
    return 99;
}

int readSeconds() {
    std::string line;
    restoreTerminal();
    std::cout << "Countdown seconds (1-86400): " << std::flush;
    if (!std::getline(std::cin, line)) return -1;
    setupTerminal();
    try {
        std::size_t used = 0;
        const int v = std::stoi(line, &used);
        if (used == line.size() && v >= 1 && v <= 86400) return v;
    } catch (const std::exception&) {
    }
    return 0;
}

}  // namespace

int main() {
    TerminalGuard guard;
    std::cout << "=== Digital Clock ===\n";
    if (!g_ansi) std::cout << "(This console does not support in-place redraw; output will scroll.)\n";

    while (true) {
        const int choice = readMenuChoice();
        if (choice == 0 || choice == -1) break;
        if (choice == 1) runClock();
        else if (choice == 2) runStopwatch();
        else if (choice == 3) {
            const int s = readSeconds();
            if (s == -1) break;
            if (s == 0) { std::cout << "Please enter a whole number between 1 and 86400.\n"; continue; }
            runCountdown(s);
        } else std::cout << "Invalid choice.\n";
    }
    std::cout << "\nGoodbye.\n";
    return 0;
}

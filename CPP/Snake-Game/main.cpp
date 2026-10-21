// Snake Game (Intermediate C++)
//
// Classic grid-based snake: eat food to grow, avoid walls and your own tail.
// Speed increases as you score. Includes pause, restart and a high score
// saved to snake_highscore.txt.
//
// Terminal requirements:
//   - Windows 10 (1511+) / 11: cmd.exe, PowerShell or Windows Terminal.
//     Uses <conio.h> (_kbhit/_getch) and enables VT processing for redraws.
//   - Linux / macOS / WSL: any ANSI terminal; uses termios raw mode + select().
//   - Must be run in a real interactive terminal (not an IDE output pane).
//
// Standard: C++17 (<chrono>, <thread>, <deque>, <random>).

#include <cctype>
#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#  include <conio.h>
#  include <windows.h>
#else
#  include <sys/select.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace {

constexpr int kWidth = 30;
constexpr int kHeight = 18;
const char* const kHighScoreFile = "snake_highscore.txt";

// ---------- terminal abstraction ----------

class Terminal {
public:
    Terminal() {
#ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(out, &mode)) {
            original_ = mode;
            SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#else
        if (tcgetattr(STDIN_FILENO, &original_) == 0) {
            termios raw = original_;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            rawSet_ = true;
        }
#endif
        std::cout << "\x1b[?25l";  // hide cursor
    }
    ~Terminal() {
        std::cout << "\x1b[?25h\x1b[0m" << std::flush;  // show cursor, reset colours
#ifdef _WIN32
        if (original_) SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), original_);
#else
        if (rawSet_) tcsetattr(STDIN_FILENO, TCSANOW, &original_);
#endif
    }
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    // Returns a normalised key: 'w','a','s','d','p','q','r' or 0 if none.
    static int pollKey() {
#ifdef _WIN32
        if (!_kbhit()) return 0;
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            switch (_getch()) {
                case 72: return 'w';
                case 80: return 's';
                case 75: return 'a';
                case 77: return 'd';
                default: return 0;
            }
        }
        return std::tolower(c);
#else
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        timeval tv{0, 0};
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) return 0;
        unsigned char c = 0;
        if (read(STDIN_FILENO, &c, 1) != 1) return 0;
        if (c == 0x1b) {  // arrow key escape sequence: ESC [ A/B/C/D
            unsigned char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1 && seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return 'w';
                    case 'B': return 's';
                    case 'D': return 'a';
                    case 'C': return 'd';
                }
            }
            return 0;
        }
        return std::tolower(c);
#endif
    }

private:
#ifdef _WIN32
    DWORD original_ = 0;
#else
    termios original_{};
    bool rawSet_ = false;
#endif
};

// ---------- game ----------

struct Point {
    int x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

enum class Dir { Up, Down, Left, Right };

class SnakeGame {
public:
    SnakeGame() : rng_(std::random_device{}()) { reset(); }

    void reset() {
        snake_.clear();
        const int cx = kWidth / 2, cy = kHeight / 2;
        for (int i = 0; i < 3; ++i) snake_.push_back({cx - i, cy});
        dir_ = Dir::Right;
        pendingDir_ = Dir::Right;
        score_ = 0;
        alive_ = true;
        placeFood();
    }

    void setDirection(Dir d) {
        // Ignore reversing directly into yourself.
        if ((d == Dir::Up && dir_ == Dir::Down) || (d == Dir::Down && dir_ == Dir::Up) ||
            (d == Dir::Left && dir_ == Dir::Right) || (d == Dir::Right && dir_ == Dir::Left))
            return;
        pendingDir_ = d;
    }

    void step() {
        if (!alive_) return;
        dir_ = pendingDir_;
        Point head = snake_.front();
        switch (dir_) {
            case Dir::Up: --head.y; break;
            case Dir::Down: ++head.y; break;
            case Dir::Left: --head.x; break;
            case Dir::Right: ++head.x; break;
        }
        if (head.x < 0 || head.x >= kWidth || head.y < 0 || head.y >= kHeight) { alive_ = false; return; }
        const bool eating = head == food_;
        // The tail cell frees up this tick unless we are growing.
        if (!eating) snake_.pop_back();
        for (const Point& p : snake_)
            if (p == head) { alive_ = false; return; }
        snake_.push_front(head);
        if (eating) {
            score_ += 10;
            placeFood();
        }
    }

    bool alive() const { return alive_; }
    int score() const { return score_; }
    int length() const { return static_cast<int>(snake_.size()); }

    // Delay per tick shrinks as score grows (min 60 ms).
    std::chrono::milliseconds tickDelay() const {
        const int ms = 180 - (score_ / 50) * 15;
        return std::chrono::milliseconds(ms < 60 ? 60 : ms);
    }

    std::string render(int highScore, bool paused) const {
        std::vector<std::string> grid(kHeight, std::string(kWidth, ' '));
        grid[static_cast<std::size_t>(food_.y)][static_cast<std::size_t>(food_.x)] = '*';
        for (std::size_t i = 0; i < snake_.size(); ++i)
            grid[static_cast<std::size_t>(snake_[i].y)][static_cast<std::size_t>(snake_[i].x)] = (i == 0) ? '@' : 'o';

        std::string out = "\x1b[H";  // cursor home; redraw over previous frame
        out += " SNAKE   score: " + std::to_string(score_) + "   best: " + std::to_string(highScore) + "   length: " + std::to_string(length()) + "\x1b[K\n";
        out += '+' + std::string(kWidth, '-') + "+\n";
        for (const auto& row : grid) out += '|' + row + "|\n";
        out += '+' + std::string(kWidth, '-') + "+\n";
        out += paused ? " PAUSED - press P to resume\x1b[K\n" : " WASD/arrows move  P pause  Q quit\x1b[K\n";
        return out;
    }

private:
    std::deque<Point> snake_;
    Point food_{0, 0};
    Dir dir_ = Dir::Right, pendingDir_ = Dir::Right;
    int score_ = 0;
    bool alive_ = true;
    std::mt19937 rng_;

    void placeFood() {
        if (length() >= kWidth * kHeight) { alive_ = false; return; }  // board full: you win
        std::uniform_int_distribution<int> dx(0, kWidth - 1), dy(0, kHeight - 1);
        for (;;) {
            Point p{dx(rng_), dy(rng_)};
            bool onSnake = false;
            for (const Point& s : snake_) if (s == p) { onSnake = true; break; }
            if (!onSnake) { food_ = p; return; }
        }
    }
};

int loadHighScore() {
    std::ifstream in(kHighScoreFile);
    int v = 0;
    if (in && (in >> v) && v >= 0) return v;
    return 0;
}

void saveHighScore(int score) {
    std::ofstream out(kHighScoreFile, std::ios::trunc);
    if (!out) { std::cerr << "Warning: could not save high score.\n"; return; }
    out << score << '\n';
}

}  // namespace

int main() {
    int highScore = loadHighScore();
    std::cout << "Snake - use WASD or arrow keys. Press Enter to start..." << std::flush;
    std::string dummy;
    if (!std::getline(std::cin, dummy)) return 0;

    Terminal term;
    SnakeGame game;
    bool paused = false;
    bool quit = false;
    std::cout << "\x1b[2J";  // clear once

    while (!quit) {
        // Handle input
        for (int key = Terminal::pollKey(); key != 0; key = Terminal::pollKey()) {
            switch (key) {
                case 'w': game.setDirection(Dir::Up); break;
                case 's': game.setDirection(Dir::Down); break;
                case 'a': game.setDirection(Dir::Left); break;
                case 'd': game.setDirection(Dir::Right); break;
                case 'p': paused = !paused; break;
                case 'q': quit = true; break;
                case 'r': if (!game.alive()) { game.reset(); std::cout << "\x1b[2J"; } break;
                default: break;
            }
        }

        if (game.alive()) {
            if (!paused) game.step();
            std::cout << game.render(highScore, paused) << std::flush;
            if (!game.alive()) {
                if (game.score() > highScore) {
                    highScore = game.score();
                    saveHighScore(highScore);
                    std::cout << " GAME OVER - new high score " << highScore << "!";
                } else {
                    std::cout << " GAME OVER - score " << game.score() << ".";
                }
                std::cout << " Press R to restart or Q to quit.\x1b[K\n" << std::flush;
            }
            std::this_thread::sleep_for(game.tickDelay());
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    std::cout << "\nThanks for playing. Best score: " << highScore << "\n";
    return 0;
}

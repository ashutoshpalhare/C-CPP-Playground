// Tic-Tac-Toe with AI (Intermediate C++)
//
// Play against an unbeatable computer opponent that uses minimax with
// alpha-beta pruning, or choose an easier difficulty that mixes in random
// moves. Also supports two-player mode and a scoreboard.
//
// Concepts: recursion, game-tree search, std::array, enums, classes,
// <random>, input validation.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <string>

namespace {

enum class Cell : char { Empty = ' ', X = 'X', O = 'O' };

class Board {
public:
    Board() { cells_.fill(Cell::Empty); }

    bool place(int idx, Cell who) {
        if (idx < 0 || idx > 8 || cells_[static_cast<std::size_t>(idx)] != Cell::Empty) return false;
        cells_[static_cast<std::size_t>(idx)] = who;
        return true;
    }
    void clear(int idx) { cells_[static_cast<std::size_t>(idx)] = Cell::Empty; }
    Cell at(int idx) const { return cells_[static_cast<std::size_t>(idx)]; }
    bool full() const { return std::none_of(cells_.begin(), cells_.end(), [](Cell c) { return c == Cell::Empty; }); }

    std::optional<Cell> winner() const {
        static const int lines[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};
        for (const auto& l : lines) {
            const Cell a = at(l[0]);
            if (a != Cell::Empty && a == at(l[1]) && a == at(l[2])) return a;
        }
        return std::nullopt;
    }

    void print() const {
        std::cout << "\n";
        for (int r = 0; r < 3; ++r) {
            std::cout << " ";
            for (int c = 0; c < 3; ++c) {
                const int idx = r * 3 + c;
                const Cell cell = at(idx);
                if (cell == Cell::Empty) std::cout << idx + 1; else std::cout << static_cast<char>(cell);
                if (c < 2) std::cout << " | ";
            }
            std::cout << "\n";
            if (r < 2) std::cout << "---+---+---\n";
        }
        std::cout << "\n";
    }

private:
    std::array<Cell, 9> cells_;
};

Cell other(Cell c) { return c == Cell::X ? Cell::O : Cell::X; }

class AI {
public:
    enum class Level { Easy, Medium, Hard };

    AI(Cell mark, Level level) : mark_(mark), level_(level), rng_(std::random_device{}()) {}

    int chooseMove(Board& board) {
        // Easy: random. Medium: 50% best move / 50% random. Hard: always best.
        std::uniform_int_distribution<int> coin(0, 1);
        if (level_ == Level::Easy || (level_ == Level::Medium && coin(rng_) == 0)) return randomMove(board);
        return bestMove(board);
    }

private:
    Cell mark_;
    Level level_;
    std::mt19937 rng_;

    int randomMove(const Board& board) {
        std::array<int, 9> options{};
        int n = 0;
        for (int i = 0; i < 9; ++i) if (board.at(i) == Cell::Empty) options[static_cast<std::size_t>(n++)] = i;
        std::uniform_int_distribution<int> pick(0, n - 1);
        return options[static_cast<std::size_t>(pick(rng_))];
    }

    // Minimax with alpha-beta pruning. Scores: +10 win (minus depth to prefer
    // faster wins), -10 loss, 0 draw.
    int minimax(Board& board, int depth, bool maximising, int alpha, int beta) {
        if (auto w = board.winner()) return *w == mark_ ? 10 - depth : depth - 10;
        if (board.full()) return 0;
        int best = maximising ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
        for (int i = 0; i < 9; ++i) {
            if (board.at(i) != Cell::Empty) continue;
            board.place(i, maximising ? mark_ : other(mark_));
            const int score = minimax(board, depth + 1, !maximising, alpha, beta);
            board.clear(i);
            if (maximising) { best = std::max(best, score); alpha = std::max(alpha, best); }
            else { best = std::min(best, score); beta = std::min(beta, best); }
            if (beta <= alpha) break;
        }
        return best;
    }

    int bestMove(Board& board) {
        int bestScore = std::numeric_limits<int>::min();
        std::array<int, 9> ties{};
        int tieCount = 0;
        for (int i = 0; i < 9; ++i) {
            if (board.at(i) != Cell::Empty) continue;
            board.place(i, mark_);
            const int score = minimax(board, 0, false, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            board.clear(i);
            if (score > bestScore) { bestScore = score; tieCount = 0; }
            if (score == bestScore) ties[static_cast<std::size_t>(tieCount++)] = i;
        }
        // Pick randomly among equally good moves so games are not identical.
        std::uniform_int_distribution<int> pick(0, tieCount - 1);
        return ties[static_cast<std::size_t>(pick(rng_))];
    }
};

// Returns 1-9 as index 0-8, or -1 on EOF.
int readMove(const Board& board, const std::string& who) {
    std::string line;
    while (true) {
        std::cout << who << ", choose a square (1-9): ";
        if (!std::getline(std::cin, line)) return -1;
        try {
            std::size_t used = 0;
            const int v = std::stoi(line, &used);
            if (used == line.size() && v >= 1 && v <= 9) {
                if (board.at(v - 1) == Cell::Empty) return v - 1;
                std::cout << "That square is taken.\n";
                continue;
            }
        } catch (const std::exception&) {
        }
        std::cout << "Enter a number from 1 to 9.\n";
    }
}

std::optional<char> readOption(const std::string& prompt, const std::string& allowed) {
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) return std::nullopt;
        if (line.size() == 1) {
            const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
            if (allowed.find(c) != std::string::npos) return c;
        }
        std::cout << "Please enter one of: " << allowed << "\n";
    }
}

}  // namespace

int main() {
    std::cout << "=== Tic-Tac-Toe with AI ===\n";
    int winsX = 0, winsO = 0, draws = 0;

    while (true) {
        auto mode = readOption("\nMode: (1) vs computer  (2) two players  (q) quit: ", "12q");
        if (!mode || *mode == 'q') break;

        std::optional<AI> ai;
        Cell human = Cell::X;
        if (*mode == '1') {
            auto lvl = readOption("Difficulty: (e)asy  (m)edium  (h)ard/unbeatable: ", "emh");
            if (!lvl) break;
            auto side = readOption("Play as (x) first or (o) second: ", "xo");
            if (!side) break;
            human = *side == 'x' ? Cell::X : Cell::O;
            const AI::Level level = *lvl == 'e' ? AI::Level::Easy : *lvl == 'm' ? AI::Level::Medium : AI::Level::Hard;
            ai.emplace(other(human), level);
        }

        Board board;
        Cell turn = Cell::X;
        bool aborted = false;
        board.print();
        while (!board.winner() && !board.full()) {
            int move;
            const bool aiTurn = ai && turn != human;
            if (aiTurn) {
                move = ai->chooseMove(board);
                std::cout << "Computer (" << static_cast<char>(turn) << ") plays " << move + 1 << ".\n";
            } else {
                move = readMove(board, std::string("Player ") + static_cast<char>(turn));
                if (move < 0) { aborted = true; break; }
            }
            board.place(move, turn);
            board.print();
            turn = other(turn);
        }
        if (aborted) break;

        if (auto w = board.winner()) {
            std::cout << (ai && *w != human ? "Computer wins!" : std::string("Player ") + static_cast<char>(*w) + " wins!") << "\n";
            (*w == Cell::X ? winsX : winsO)++;
        } else {
            std::cout << "It's a draw.\n";
            ++draws;
        }
        std::cout << "Scoreboard - X: " << winsX << "  O: " << winsO << "  Draws: " << draws << "\n";
    }
    std::cout << "\nThanks for playing!\n";
    return 0;
}

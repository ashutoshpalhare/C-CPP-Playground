// Chess Game (Advanced C++)
//
// Two-player terminal chess with full legal-move validation:
//   - All piece movement rules, including castling, en passant and promotion
//   - Check, checkmate and stalemate detection
//   - Draw by 50-move rule and insufficient material (basic cases)
//   - Move input in coordinate notation (e2e4, e7e8q), undo, move list,
//     and an optional simple computer opponent (one-ply capture-greedy)
//
// Concepts: value-type board modelling, enums, std::vector move generation,
// pseudo-legal -> legal filtering by king safety, undo via state snapshots.
//
// Standard: C++17. Portable to Windows, Linux and macOS (ASCII output).

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

enum class Color { White, Black, None };
enum class Kind { None, Pawn, Knight, Bishop, Rook, Queen, King };

struct Piece {
    Kind kind = Kind::None;
    Color color = Color::None;
    bool empty() const { return kind == Kind::None; }
};

struct Square {
    int file, rank;  // 0-7, file a-h, rank 1-8 -> 0-7
    bool valid() const { return file >= 0 && file < 8 && rank >= 0 && rank < 8; }
    bool operator==(const Square& o) const { return file == o.file && rank == o.rank; }
    bool operator!=(const Square& o) const { return !(*this == o); }
};

struct Move {
    Square from{}, to{};
    Kind promotion = Kind::None;
    bool isCastle = false;
    bool isEnPassant = false;
};

Color opposite(Color c) { return c == Color::White ? Color::Black : Color::White; }

char pieceChar(const Piece& p) {
    char c = '.';
    switch (p.kind) {
        case Kind::Pawn: c = 'p'; break;
        case Kind::Knight: c = 'n'; break;
        case Kind::Bishop: c = 'b'; break;
        case Kind::Rook: c = 'r'; break;
        case Kind::Queen: c = 'q'; break;
        case Kind::King: c = 'k'; break;
        default: return '.';
    }
    return p.color == Color::White ? static_cast<char>(std::toupper(c)) : c;
}

std::string squareName(Square s) { return std::string(1, static_cast<char>('a' + s.file)) + static_cast<char>('1' + s.rank); }

std::string moveName(const Move& m) {
    std::string s = squareName(m.from) + squareName(m.to);
    if (m.promotion != Kind::None) s += pieceChar(Piece{m.promotion, Color::Black});
    return s;
}

class Board {
public:
    Board() { reset(); }

    void reset() {
        for (auto& row : grid_) row.fill(Piece{});
        const Kind back[8] = {Kind::Rook, Kind::Knight, Kind::Bishop, Kind::Queen, Kind::King, Kind::Bishop, Kind::Knight, Kind::Rook};
        for (int f = 0; f < 8; ++f) {
            set({f, 0}, {back[f], Color::White});
            set({f, 1}, {Kind::Pawn, Color::White});
            set({f, 6}, {Kind::Pawn, Color::Black});
            set({f, 7}, {back[f], Color::Black});
        }
        turn_ = Color::White;
        castle_ = {true, true, true, true};
        enPassant_.reset();
        halfmoveClock_ = 0;
        fullmove_ = 1;
        history_.clear();
    }

    Color turn() const { return turn_; }
    const Piece& at(Square s) const { return grid_[static_cast<std::size_t>(s.rank)][static_cast<std::size_t>(s.file)]; }
    int fullmoveNumber() const { return fullmove_; }
    int halfmoveClock() const { return halfmoveClock_; }
    const std::vector<std::string>& moveList() const { return moveNames_; }

    // ----- legality -----
    std::vector<Move> legalMoves() const {
        std::vector<Move> out;
        for (const Move& m : pseudoLegalMoves(turn_)) {
            Board copy = *this;
            copy.applyRaw(m);
            if (!copy.isAttacked(copy.kingSquare(turn_), opposite(turn_))) out.push_back(m);
        }
        return out;
    }

    bool inCheck() const { return isAttacked(kingSquare(turn_), opposite(turn_)); }

    bool makeMove(const Move& m) {
        const auto moves = legalMoves();
        const auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& x) {
            return x.from == m.from && x.to == m.to && x.promotion == m.promotion;
        });
        if (it == moves.end()) return false;
        history_.push_back(snapshot());
        moveNames_.push_back(moveName(*it));
        applyRaw(*it);
        return true;
    }

    bool undo() {
        if (history_.empty()) return false;
        restore(history_.back());
        history_.pop_back();
        moveNames_.pop_back();
        return true;
    }

    bool insufficientMaterial() const {
        int minor = 0;
        for (int r = 0; r < 8; ++r)
            for (int f = 0; f < 8; ++f) {
                const Piece& p = at({f, r});
                if (p.kind == Kind::Pawn || p.kind == Kind::Rook || p.kind == Kind::Queen) return false;
                if (p.kind == Kind::Knight || p.kind == Kind::Bishop) ++minor;
            }
        return minor <= 1;  // K vs K, K+N vs K, K+B vs K
    }

    void print() const {
        std::cout << "\n    a b c d e f g h\n   -----------------\n";
        for (int r = 7; r >= 0; --r) {
            std::cout << ' ' << r + 1 << " | ";
            for (int f = 0; f < 8; ++f) std::cout << pieceChar(at({f, r})) << ' ';
            std::cout << "| " << r + 1 << '\n';
        }
        std::cout << "   -----------------\n    a b c d e f g h\n\n";
    }

    // Simple material evaluation from White's point of view (for the AI).
    int material() const {
        int score = 0;
        for (int r = 0; r < 8; ++r)
            for (int f = 0; f < 8; ++f) {
                const Piece& p = at({f, r});
                int v = 0;
                switch (p.kind) {
                    case Kind::Pawn: v = 100; break;
                    case Kind::Knight: v = 320; break;
                    case Kind::Bishop: v = 330; break;
                    case Kind::Rook: v = 500; break;
                    case Kind::Queen: v = 900; break;
                    default: break;
                }
                score += p.color == Color::White ? v : -v;
            }
        return score;
    }

    void applyForSearch(const Move& m) { applyRaw(m); }

private:
    std::array<std::array<Piece, 8>, 8> grid_;
    Color turn_ = Color::White;
    std::array<bool, 4> castle_{};  // WK, WQ, BK, BQ
    std::optional<Square> enPassant_;
    int halfmoveClock_ = 0;
    int fullmove_ = 1;

    struct Snapshot {
        std::array<std::array<Piece, 8>, 8> grid;
        Color turn;
        std::array<bool, 4> castle;
        std::optional<Square> enPassant;
        int halfmove, fullmove;
    };
    std::vector<Snapshot> history_;
    std::vector<std::string> moveNames_;

    Snapshot snapshot() const { return {grid_, turn_, castle_, enPassant_, halfmoveClock_, fullmove_}; }
    void restore(const Snapshot& s) { grid_ = s.grid; turn_ = s.turn; castle_ = s.castle; enPassant_ = s.enPassant; halfmoveClock_ = s.halfmove; fullmove_ = s.fullmove; }

    void set(Square s, Piece p) { grid_[static_cast<std::size_t>(s.rank)][static_cast<std::size_t>(s.file)] = p; }

    Square kingSquare(Color c) const {
        for (int r = 0; r < 8; ++r)
            for (int f = 0; f < 8; ++f)
                if (at({f, r}).kind == Kind::King && at({f, r}).color == c) return {f, r};
        return {-1, -1};
    }

    bool isAttacked(Square target, Color by) const {
        if (!target.valid()) return false;
        // Pawns
        const int dir = by == Color::White ? 1 : -1;
        for (int df : {-1, 1}) {
            Square s{target.file + df, target.rank - dir};
            if (s.valid() && at(s).kind == Kind::Pawn && at(s).color == by) return true;
        }
        // Knights
        static const int kn[8][2] = {{1, 2}, {2, 1}, {-1, 2}, {-2, 1}, {1, -2}, {2, -1}, {-1, -2}, {-2, -1}};
        for (const auto& d : kn) {
            Square s{target.file + d[0], target.rank + d[1]};
            if (s.valid() && at(s).kind == Kind::Knight && at(s).color == by) return true;
        }
        // King
        for (int df = -1; df <= 1; ++df)
            for (int dr = -1; dr <= 1; ++dr) {
                if (!df && !dr) continue;
                Square s{target.file + df, target.rank + dr};
                if (s.valid() && at(s).kind == Kind::King && at(s).color == by) return true;
            }
        // Sliders
        static const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (int i = 0; i < 8; ++i) {
            Square s{target.file + dirs[i][0], target.rank + dirs[i][1]};
            while (s.valid()) {
                const Piece& p = at(s);
                if (!p.empty()) {
                    if (p.color == by) {
                        const bool straight = i < 4;
                        if (p.kind == Kind::Queen || (straight && p.kind == Kind::Rook) || (!straight && p.kind == Kind::Bishop)) return true;
                    }
                    break;
                }
                s.file += dirs[i][0];
                s.rank += dirs[i][1];
            }
        }
        return false;
    }

    void addPawnMoves(std::vector<Move>& out, Square from, Color c) const {
        const int dir = c == Color::White ? 1 : -1;
        const int startRank = c == Color::White ? 1 : 6;
        const int promoRank = c == Color::White ? 7 : 0;
        auto push = [&](Square to, bool ep) {
            if (to.rank == promoRank) {
                for (Kind k : {Kind::Queen, Kind::Rook, Kind::Bishop, Kind::Knight}) out.push_back({from, to, k, false, false});
            } else {
                out.push_back({from, to, Kind::None, false, ep});
            }
        };
        Square one{from.file, from.rank + dir};
        if (one.valid() && at(one).empty()) {
            push(one, false);
            Square two{from.file, from.rank + 2 * dir};
            if (from.rank == startRank && at(two).empty()) out.push_back({from, two, Kind::None, false, false});
        }
        for (int df : {-1, 1}) {
            Square cap{from.file + df, from.rank + dir};
            if (!cap.valid()) continue;
            if (!at(cap).empty() && at(cap).color != c) push(cap, false);
            else if (enPassant_ && cap == *enPassant_) push(cap, true);
        }
    }

    std::vector<Move> pseudoLegalMoves(Color c) const {
        std::vector<Move> out;
        static const int knightD[8][2] = {{1, 2}, {2, 1}, {-1, 2}, {-2, 1}, {1, -2}, {2, -1}, {-1, -2}, {-2, -1}};
        static const int rookD[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        static const int bishopD[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (int r = 0; r < 8; ++r)
            for (int f = 0; f < 8; ++f) {
                const Square from{f, r};
                const Piece& p = at(from);
                if (p.empty() || p.color != c) continue;
                auto slide = [&](const int (*dirs)[2], int n) {
                    for (int i = 0; i < n; ++i) {
                        Square s{from.file + dirs[i][0], from.rank + dirs[i][1]};
                        while (s.valid()) {
                            if (at(s).empty()) out.push_back({from, s});
                            else { if (at(s).color != c) out.push_back({from, s}); break; }
                            s.file += dirs[i][0];
                            s.rank += dirs[i][1];
                        }
                    }
                };
                switch (p.kind) {
                    case Kind::Pawn: addPawnMoves(out, from, c); break;
                    case Kind::Knight:
                        for (const auto& d : knightD) {
                            Square s{f + d[0], r + d[1]};
                            if (s.valid() && (at(s).empty() || at(s).color != c)) out.push_back({from, s});
                        }
                        break;
                    case Kind::Bishop: slide(bishopD, 4); break;
                    case Kind::Rook: slide(rookD, 4); break;
                    case Kind::Queen: slide(rookD, 4); slide(bishopD, 4); break;
                    case Kind::King: {
                        for (int df = -1; df <= 1; ++df)
                            for (int dr = -1; dr <= 1; ++dr) {
                                if (!df && !dr) continue;
                                Square s{f + df, r + dr};
                                if (s.valid() && (at(s).empty() || at(s).color != c)) out.push_back({from, s});
                            }
                        // Castling: king and rook unmoved, path empty, not in/through check.
                        const int home = c == Color::White ? 0 : 7;
                        const int ks = c == Color::White ? 0 : 2;
                        if (from == Square{4, home} && !isAttacked(from, opposite(c))) {
                            if (castle_[static_cast<std::size_t>(ks)] && at({5, home}).empty() && at({6, home}).empty() &&
                                at({7, home}).kind == Kind::Rook && at({7, home}).color == c &&
                                !isAttacked({5, home}, opposite(c)) && !isAttacked({6, home}, opposite(c)))
                                out.push_back({from, {6, home}, Kind::None, true, false});
                            if (castle_[static_cast<std::size_t>(ks + 1)] && at({3, home}).empty() && at({2, home}).empty() && at({1, home}).empty() &&
                                at({0, home}).kind == Kind::Rook && at({0, home}).color == c &&
                                !isAttacked({3, home}, opposite(c)) && !isAttacked({2, home}, opposite(c)))
                                out.push_back({from, {2, home}, Kind::None, true, false});
                        }
                        break;
                    }
                    default: break;
                }
            }
        return out;
    }

    // Applies a move assumed to be (pseudo-)legal, updating all state.
    void applyRaw(const Move& m) {
        Piece moving = at(m.from);
        const Piece captured = at(m.to);
        const bool isCapture = !captured.empty() || m.isEnPassant;

        set(m.from, Piece{});
        if (m.isEnPassant) set({m.to.file, m.from.rank}, Piece{});
        if (m.promotion != Kind::None) moving.kind = m.promotion;
        set(m.to, moving);

        if (m.isCastle) {
            const int home = m.from.rank;
            if (m.to.file == 6) { set({5, home}, at({7, home})); set({7, home}, Piece{}); }
            else { set({3, home}, at({0, home})); set({0, home}, Piece{}); }
        }

        // Update castling rights when king/rooks move or rooks are captured.
        auto revoke = [&](Square s) {
            if (s == Square{4, 0}) castle_[0] = castle_[1] = false;
            if (s == Square{4, 7}) castle_[2] = castle_[3] = false;
            if (s == Square{7, 0}) castle_[0] = false;
            if (s == Square{0, 0}) castle_[1] = false;
            if (s == Square{7, 7}) castle_[2] = false;
            if (s == Square{0, 7}) castle_[3] = false;
        };
        revoke(m.from);
        revoke(m.to);

        enPassant_.reset();
        if (moving.kind == Kind::Pawn && std::abs(m.to.rank - m.from.rank) == 2) enPassant_ = Square{m.from.file, (m.from.rank + m.to.rank) / 2};

        halfmoveClock_ = (moving.kind == Kind::Pawn || isCapture) ? 0 : halfmoveClock_ + 1;
        if (turn_ == Color::Black) ++fullmove_;
        turn_ = opposite(turn_);
    }
};

// ----- input parsing -----

std::optional<Move> parseMove(const std::string& text) {
    std::string s;
    for (char c : text) if (!std::isspace(static_cast<unsigned char>(c)) && c != '-') s += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (s.size() != 4 && s.size() != 5) return std::nullopt;
    Move m;
    m.from = {s[0] - 'a', s[1] - '1'};
    m.to = {s[2] - 'a', s[3] - '1'};
    if (!m.from.valid() || !m.to.valid()) return std::nullopt;
    if (s.size() == 5) {
        switch (s[4]) {
            case 'q': m.promotion = Kind::Queen; break;
            case 'r': m.promotion = Kind::Rook; break;
            case 'b': m.promotion = Kind::Bishop; break;
            case 'n': m.promotion = Kind::Knight; break;
            default: return std::nullopt;
        }
    }
    return m;
}

// Greedy one-ply AI: pick the move with the best material result; random among ties.
Move chooseComputerMove(const Board& board, std::mt19937& rng) {
    const auto moves = board.legalMoves();
    const int sign = board.turn() == Color::White ? 1 : -1;
    int best = -1000000;
    std::vector<Move> ties;
    for (const Move& m : moves) {
        Board copy = board;
        copy.applyForSearch(m);
        int score = sign * copy.material();
        // Avoid moves that give the opponent an immediate recapture of higher value: penalise leaving the moved piece attacked.
        const auto replies = copy.legalMoves();
        int worst = 0;
        for (const Move& r : replies) {
            Board c2 = copy;
            c2.applyForSearch(r);
            worst = std::min(worst, sign * c2.material() - score);
        }
        score += worst;
        if (replies.empty()) score += copy.inCheck() ? 100000 : -50;  // checkmate great, stalemate bad
        if (score > best) { best = score; ties.clear(); }
        if (score == best) ties.push_back(m);
    }
    std::uniform_int_distribution<std::size_t> pick(0, ties.size() - 1);
    return ties[pick(rng)];
}

void help() {
    std::cout << "Enter moves as from-square + to-square, e.g. e2e4, g1f3. Promotion: e7e8q (q/r/b/n).\n"
                 "Castling: move the king two squares (e1g1 or e1c1).\n"
                 "Commands: moves (list legal moves), undo, history, board, resign, help, quit\n";
}

}  // namespace

int main() {
    std::cout << "=== Chess ===\n";
    help();
    std::cout << "Play against the computer? (y/N): ";
    std::string line;
    if (!std::getline(std::cin, line)) return 0;
    const bool vsComputer = !line.empty() && std::tolower(static_cast<unsigned char>(line[0])) == 'y';
    Color humanColor = Color::White;
    if (vsComputer) {
        std::cout << "Play as (w)hite or (b)lack? [w]: ";
        if (!std::getline(std::cin, line)) return 0;
        if (!line.empty() && std::tolower(static_cast<unsigned char>(line[0])) == 'b') humanColor = Color::Black;
    }

    std::mt19937 rng{std::random_device{}()};
    Board board;
    board.print();

    while (true) {
        const auto legal = board.legalMoves();
        const char* side = board.turn() == Color::White ? "White" : "Black";
        if (legal.empty()) {
            if (board.inCheck()) std::cout << "Checkmate! " << (board.turn() == Color::White ? "Black" : "White") << " wins.\n";
            else std::cout << "Stalemate - draw.\n";
            break;
        }
        if (board.halfmoveClock() >= 100) { std::cout << "Draw by the 50-move rule.\n"; break; }
        if (board.insufficientMaterial()) { std::cout << "Draw by insufficient material.\n"; break; }
        if (board.inCheck()) std::cout << side << " is in check!\n";

        if (vsComputer && board.turn() != humanColor) {
            const Move m = chooseComputerMove(board, rng);
            board.makeMove(m);
            std::cout << "Computer plays " << moveName(m) << "\n";
            board.print();
            continue;
        }

        std::cout << board.fullmoveNumber() << ". " << side << " to move > ";
        if (!std::getline(std::cin, line)) { std::cout << "\nInput closed.\n"; break; }
        std::string cmd = line;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (cmd == "quit" || cmd == "exit") break;
        if (cmd == "help") { help(); continue; }
        if (cmd == "board") { board.print(); continue; }
        if (cmd == "resign") { std::cout << side << " resigns. " << (board.turn() == Color::White ? "Black" : "White") << " wins.\n"; break; }
        if (cmd == "history") {
            const auto& h = board.moveList();
            if (h.empty()) std::cout << "(no moves yet)\n";
            for (std::size_t i = 0; i < h.size(); ++i) std::cout << (i % 2 == 0 ? std::to_string(i / 2 + 1) + ". " : "") << h[i] << (i % 2 == 1 ? "\n" : " ");
            if (h.size() % 2 == 1) std::cout << "\n";
            continue;
        }
        if (cmd == "moves") {
            std::cout << legal.size() << " legal move(s):";
            for (const Move& m : legal) std::cout << ' ' << moveName(m);
            std::cout << "\n";
            continue;
        }
        if (cmd == "undo") {
            bool ok = board.undo();
            if (vsComputer && ok) board.undo();  // also take back the computer's reply
            std::cout << (ok ? "Move taken back.\n" : "Nothing to undo.\n");
            board.print();
            continue;
        }
        auto parsed = parseMove(cmd);
        if (!parsed) { std::cout << "Could not understand '" << line << "'. Type 'help'.\n"; continue; }
        // Default promotion to queen if the user omitted the piece.
        if (parsed->promotion == Kind::None && board.at(parsed->from).kind == Kind::Pawn && (parsed->to.rank == 7 || parsed->to.rank == 0)) parsed->promotion = Kind::Queen;
        if (!board.makeMove(*parsed)) { std::cout << "Illegal move. Type 'moves' to list legal moves.\n"; continue; }
        board.print();
    }
    std::cout << "Game over after " << board.moveList().size() << " half-moves. Thanks for playing.\n";
    return 0;
}

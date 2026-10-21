// Mini Database Engine (Advanced C++)
//
// A tiny SQL-like database with:
//   - Tables with typed columns (INT, TEXT) and an optional PRIMARY KEY
//   - CREATE TABLE, DROP TABLE, INSERT, SELECT (WHERE, ORDER BY, LIMIT),
//     UPDATE, DELETE, SHOW TABLES, DESCRIBE, SAVE, LOAD, HELP, EXIT
//   - A hand-written tokenizer and recursive-descent parser
//   - std::map-based indexes: primary key index + CREATE INDEX on any column
//   - Persistence to a simple text file (mini.db by default)
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace {

// ===================== values & schema =====================

using Value = std::variant<long long, std::string>;
enum class Type { Int, Text };

std::string typeName(Type t) { return t == Type::Int ? "INT" : "TEXT"; }

std::string toString(const Value& v) {
    if (std::holds_alternative<long long>(v)) return std::to_string(std::get<long long>(v));
    return std::get<std::string>(v);
}

int compareValues(const Value& a, const Value& b) {
    if (a.index() != b.index()) throw std::runtime_error("type mismatch comparing " + toString(a) + " and " + toString(b));
    if (std::holds_alternative<long long>(a)) {
        const auto x = std::get<long long>(a), y = std::get<long long>(b);
        return x < y ? -1 : x > y ? 1 : 0;
    }
    return std::get<std::string>(a).compare(std::get<std::string>(b));
}

struct Column {
    std::string name;
    Type type;
    bool primaryKey = false;
};

using Row = std::vector<Value>;

class Table {
public:
    std::string name;
    std::vector<Column> columns;
    std::vector<Row> rows;
    std::map<std::string, std::multimap<Value, std::size_t>> indexes;  // column -> (value -> row index)

    int columnIndex(const std::string& col) const {
        for (std::size_t i = 0; i < columns.size(); ++i)
            if (columns[i].name == col) return static_cast<int>(i);
        return -1;
    }

    int primaryKeyIndex() const {
        for (std::size_t i = 0; i < columns.size(); ++i) if (columns[i].primaryKey) return static_cast<int>(i);
        return -1;
    }

    void rebuildIndexes() {
        for (auto& [col, idx] : indexes) {
            idx.clear();
            const int c = columnIndex(col);
            for (std::size_t r = 0; r < rows.size(); ++r) idx.emplace(rows[r][static_cast<std::size_t>(c)], r);
        }
    }

    void insert(Row row) {
        const int pk = primaryKeyIndex();
        if (pk >= 0) {
            auto it = indexes.find(columns[static_cast<std::size_t>(pk)].name);
            if (it != indexes.end() && it->second.count(row[static_cast<std::size_t>(pk)]))
                throw std::runtime_error("duplicate primary key " + toString(row[static_cast<std::size_t>(pk)]));
        }
        rows.push_back(std::move(row));
        for (auto& [col, idx] : indexes) idx.emplace(rows.back()[static_cast<std::size_t>(columnIndex(col))], rows.size() - 1);
    }
};

// ===================== tokenizer =====================

enum class Tok { Ident, Number, String, Symbol, End };
struct Token {
    Tok kind;
    std::string text;
};

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> out;
    std::size_t i = 0;
    while (i < src.size()) {
        const char c = src[i];
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t j = i;
            while (j < src.size() && (std::isalnum(static_cast<unsigned char>(src[j])) || src[j] == '_')) ++j;
            std::string word = src.substr(i, j - i);
            out.push_back({Tok::Ident, word});
            i = j;
        } else if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && i + 1 < src.size() && std::isdigit(static_cast<unsigned char>(src[i + 1])))) {
            std::size_t j = i + 1;
            while (j < src.size() && std::isdigit(static_cast<unsigned char>(src[j]))) ++j;
            out.push_back({Tok::Number, src.substr(i, j - i)});
            i = j;
        } else if (c == '\'') {
            std::string s;
            ++i;
            bool closed = false;
            while (i < src.size()) {
                if (src[i] == '\'') {
                    if (i + 1 < src.size() && src[i + 1] == '\'') { s += '\''; i += 2; continue; }  // '' escapes a quote
                    closed = true;
                    ++i;
                    break;
                }
                s += src[i++];
            }
            if (!closed) throw std::runtime_error("unterminated string literal");
            out.push_back({Tok::String, s});
        } else if ((c == '<' || c == '>' || c == '!') && i + 1 < src.size() && src[i + 1] == '=') {
            out.push_back({Tok::Symbol, src.substr(i, 2)});
            i += 2;
        } else if (std::string("(),;*=<>").find(c) != std::string::npos) {
            out.push_back({Tok::Symbol, std::string(1, c)});
            ++i;
        } else {
            throw std::runtime_error(std::string("unexpected character '") + c + "'");
        }
    }
    out.push_back({Tok::End, ""});
    return out;
}

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

// ===================== parser helpers =====================

class Parser {
public:
    explicit Parser(std::vector<Token> toks) : toks_(std::move(toks)) {}

    const Token& peek() const { return toks_[pos_]; }
    Token next() { return toks_[pos_ < toks_.size() - 1 ? pos_++ : pos_]; }
    bool atEnd() const { return peek().kind == Tok::End || (peek().kind == Tok::Symbol && peek().text == ";"); }

    bool acceptKeyword(const std::string& kw) {
        if (peek().kind == Tok::Ident && upper(peek().text) == kw) { next(); return true; }
        return false;
    }
    void expectKeyword(const std::string& kw) {
        if (!acceptKeyword(kw)) throw std::runtime_error("expected " + kw + " near '" + peek().text + "'");
    }
    bool acceptSymbol(const std::string& s) {
        if (peek().kind == Tok::Symbol && peek().text == s) { next(); return true; }
        return false;
    }
    void expectSymbol(const std::string& s) {
        if (!acceptSymbol(s)) throw std::runtime_error("expected '" + s + "' near '" + peek().text + "'");
    }
    std::string expectIdent() {
        if (peek().kind != Tok::Ident) throw std::runtime_error("expected identifier near '" + peek().text + "'");
        return next().text;
    }
    Value expectValue() {
        const Token t = next();
        if (t.kind == Tok::Number) return std::stoll(t.text);
        if (t.kind == Tok::String) return t.text;
        throw std::runtime_error("expected a value near '" + t.text + "'");
    }

private:
    std::vector<Token> toks_;
    std::size_t pos_ = 0;
};

struct Condition {
    std::string column;
    std::string op;  // = != < <= > >=
    Value value;
};

std::optional<Condition> parseWhere(Parser& p) {
    if (!p.acceptKeyword("WHERE")) return std::nullopt;
    Condition c;
    c.column = p.expectIdent();
    const Token op = p.next();
    static const char* ops[] = {"=", "!=", "<", "<=", ">", ">="};
    if (op.kind != Tok::Symbol || std::find(std::begin(ops), std::end(ops), op.text) == std::end(ops))
        throw std::runtime_error("expected comparison operator near '" + op.text + "'");
    c.op = op.text;
    c.value = p.expectValue();
    return c;
}

bool evaluate(const Condition& c, const Value& cell) {
    const int cmp = compareValues(cell, c.value);
    if (c.op == "=") return cmp == 0;
    if (c.op == "!=") return cmp != 0;
    if (c.op == "<") return cmp < 0;
    if (c.op == "<=") return cmp <= 0;
    if (c.op == ">") return cmp > 0;
    return cmp >= 0;
}

// ===================== database =====================

class Database {
public:
    explicit Database(std::string file) : file_(std::move(file)) {}

    void execute(const std::string& sql) {
        Parser p(tokenize(sql));
        if (p.atEnd()) return;
        if (p.acceptKeyword("CREATE")) {
            if (p.acceptKeyword("TABLE")) createTable(p);
            else if (p.acceptKeyword("INDEX")) createIndex(p);
            else throw std::runtime_error("expected TABLE or INDEX after CREATE");
        } else if (p.acceptKeyword("DROP")) { p.expectKeyword("TABLE"); dropTable(p.expectIdent()); }
        else if (p.acceptKeyword("INSERT")) insert(p);
        else if (p.acceptKeyword("SELECT")) select(p);
        else if (p.acceptKeyword("UPDATE")) update(p);
        else if (p.acceptKeyword("DELETE")) remove(p);
        else if (p.acceptKeyword("SHOW")) { p.expectKeyword("TABLES"); showTables(); }
        else if (p.acceptKeyword("DESCRIBE") || p.acceptKeyword("DESC")) describe(p.expectIdent());
        else if (p.acceptKeyword("SAVE")) { save(); std::cout << "Saved to " << file_ << ".\n"; }
        else if (p.acceptKeyword("LOAD")) { load(); std::cout << "Loaded " << tables_.size() << " table(s) from " << file_ << ".\n"; }
        else if (p.acceptKeyword("HELP")) help();
        else throw std::runtime_error("unknown statement near '" + p.peek().text + "'");
        if (!p.atEnd()) throw std::runtime_error("unexpected trailing input near '" + p.peek().text + "'");
    }

    bool dirty() const { return dirty_; }

    void save() {
        std::ofstream out(file_, std::ios::trunc);
        if (!out) throw std::runtime_error("cannot write " + file_);
        for (const auto& [name, t] : tables_) {
            out << "TABLE " << name << " " << t->columns.size() << "\n";
            for (const Column& c : t->columns) out << c.name << " " << typeName(c.type) << " " << (c.primaryKey ? 1 : 0) << "\n";
            out << "INDEXES " << t->indexes.size();
            for (const auto& [col, idx] : t->indexes) out << " " << col;
            out << "\nROWS " << t->rows.size() << "\n";
            for (const Row& r : t->rows) {
                for (std::size_t i = 0; i < r.size(); ++i) {
                    if (i) out << '\t';
                    out << escape(toString(r[i]));
                }
                out << "\n";
            }
        }
        if (!out) throw std::runtime_error("write failed for " + file_);
        dirty_ = false;
    }

    void load() {
        std::ifstream in(file_);
        if (!in) throw std::runtime_error("cannot open " + file_);
        std::map<std::string, std::unique_ptr<Table>> loaded;
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream hs(line);
            std::string kw, name;
            std::size_t ncols = 0;
            if (!(hs >> kw >> name >> ncols) || kw != "TABLE") throw std::runtime_error("corrupt file: expected TABLE header");
            auto t = std::make_unique<Table>();
            t->name = name;
            for (std::size_t i = 0; i < ncols; ++i) {
                std::string cname, ctype;
                int pk = 0;
                if (!std::getline(in, line)) throw std::runtime_error("corrupt file: truncated columns");
                std::istringstream cs(line);
                if (!(cs >> cname >> ctype >> pk)) throw std::runtime_error("corrupt file: bad column line");
                t->columns.push_back({cname, ctype == "INT" ? Type::Int : Type::Text, pk != 0});
            }
            std::size_t nidx = 0, nrows = 0;
            std::getline(in, line);
            std::istringstream is(line);
            if (!(is >> kw >> nidx) || kw != "INDEXES") throw std::runtime_error("corrupt file: expected INDEXES");
            for (std::size_t i = 0; i < nidx; ++i) { std::string col; is >> col; t->indexes[col]; }
            std::getline(in, line);
            std::istringstream rs(line);
            if (!(rs >> kw >> nrows) || kw != "ROWS") throw std::runtime_error("corrupt file: expected ROWS");
            for (std::size_t r = 0; r < nrows; ++r) {
                if (!std::getline(in, line)) throw std::runtime_error("corrupt file: truncated rows");
                Row row;
                std::size_t start = 0;
                for (std::size_t c = 0; c < ncols; ++c) {
                    const auto tab = c + 1 < ncols ? line.find('\t', start) : std::string::npos;
                    const std::string field = unescape(line.substr(start, tab == std::string::npos ? std::string::npos : tab - start));
                    if (t->columns[c].type == Type::Int) row.emplace_back(std::stoll(field));
                    else row.emplace_back(field);
                    start = tab == std::string::npos ? line.size() : tab + 1;
                }
                t->rows.push_back(std::move(row));
            }
            t->rebuildIndexes();
            loaded[name] = std::move(t);
        }
        tables_ = std::move(loaded);
        dirty_ = false;
    }

    bool tryLoadIfExists() {
        std::ifstream probe(file_);
        if (!probe) return false;
        probe.close();
        load();
        return true;
    }

private:
    std::string file_;
    std::map<std::string, std::unique_ptr<Table>> tables_;
    bool dirty_ = false;

    static std::string escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '\\') out += "\\\\";
            else if (c == '\t') out += "\\t";
            else if (c == '\n') out += "\\n";
            else out += c;
        }
        return out;
    }
    static std::string unescape(const std::string& s) {
        std::string out;
        for (std::size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                const char n = s[++i];
                out += n == 't' ? '\t' : n == 'n' ? '\n' : n;
            } else out += s[i];
        }
        return out;
    }

    Table& table(const std::string& name) {
        auto it = tables_.find(name);
        if (it == tables_.end()) throw std::runtime_error("no such table '" + name + "'");
        return *it->second;
    }

    static int requireColumn(const Table& t, const std::string& col) {
        const int i = t.columnIndex(col);
        if (i < 0) throw std::runtime_error("no such column '" + col + "' in " + t.name);
        return i;
    }

    static void checkType(const Column& c, const Value& v) {
        if ((c.type == Type::Int) != std::holds_alternative<long long>(v))
            throw std::runtime_error("column '" + c.name + "' expects " + typeName(c.type));
    }

    // Returns indices of rows matching the optional condition, using an index when possible.
    std::vector<std::size_t> match(Table& t, const std::optional<Condition>& where) {
        std::vector<std::size_t> out;
        if (!where) {
            for (std::size_t i = 0; i < t.rows.size(); ++i) out.push_back(i);
            return out;
        }
        const int ci = requireColumn(t, where->column);
        checkType(t.columns[static_cast<std::size_t>(ci)], where->value);
        auto idx = t.indexes.find(where->column);
        if (idx != t.indexes.end() && where->op == "=") {  // index lookup
            auto range = idx->second.equal_range(where->value);
            for (auto it = range.first; it != range.second; ++it) out.push_back(it->second);
            std::sort(out.begin(), out.end());
            return out;
        }
        for (std::size_t i = 0; i < t.rows.size(); ++i)
            if (evaluate(*where, t.rows[i][static_cast<std::size_t>(ci)])) out.push_back(i);
        return out;
    }

    // ----- statements -----
    void createTable(Parser& p) {
        auto t = std::make_unique<Table>();
        t->name = p.expectIdent();
        if (tables_.count(t->name)) throw std::runtime_error("table '" + t->name + "' already exists");
        p.expectSymbol("(");
        do {
            Column c;
            c.name = p.expectIdent();
            if (t->columnIndex(c.name) >= 0) throw std::runtime_error("duplicate column '" + c.name + "'");
            const std::string ty = upper(p.expectIdent());
            if (ty == "INT" || ty == "INTEGER") c.type = Type::Int;
            else if (ty == "TEXT" || ty == "STRING" || ty == "VARCHAR") c.type = Type::Text;
            else throw std::runtime_error("unknown type '" + ty + "' (use INT or TEXT)");
            if (p.acceptKeyword("PRIMARY")) {
                p.expectKeyword("KEY");
                if (t->primaryKeyIndex() >= 0) throw std::runtime_error("only one PRIMARY KEY allowed");
                c.primaryKey = true;
            }
            t->columns.push_back(c);
        } while (p.acceptSymbol(","));
        p.expectSymbol(")");
        const int pk = t->primaryKeyIndex();
        if (pk >= 0) t->indexes[t->columns[static_cast<std::size_t>(pk)].name];
        std::cout << "Table " << t->name << " created with " << t->columns.size() << " column(s).\n";
        tables_[t->name] = std::move(t);
        dirty_ = true;
    }

    void createIndex(Parser& p) {
        p.expectKeyword("ON");
        Table& t = table(p.expectIdent());
        p.expectSymbol("(");
        const std::string col = p.expectIdent();
        p.expectSymbol(")");
        requireColumn(t, col);
        t.indexes[col];
        t.rebuildIndexes();
        std::cout << "Index created on " << t.name << "(" << col << ").\n";
        dirty_ = true;
    }

    void dropTable(const std::string& name) {
        if (!tables_.erase(name)) throw std::runtime_error("no such table '" + name + "'");
        std::cout << "Table " << name << " dropped.\n";
        dirty_ = true;
    }

    void insert(Parser& p) {
        p.expectKeyword("INTO");
        Table& t = table(p.expectIdent());
        std::vector<int> targets;
        if (p.acceptSymbol("(")) {
            do targets.push_back(requireColumn(t, p.expectIdent())); while (p.acceptSymbol(","));
            p.expectSymbol(")");
        } else {
            for (std::size_t i = 0; i < t.columns.size(); ++i) targets.push_back(static_cast<int>(i));
        }
        p.expectKeyword("VALUES");
        int inserted = 0;
        do {
            p.expectSymbol("(");
            std::vector<Value> vals;
            do vals.push_back(p.expectValue()); while (p.acceptSymbol(","));
            p.expectSymbol(")");
            if (vals.size() != targets.size()) throw std::runtime_error("expected " + std::to_string(targets.size()) + " values, got " + std::to_string(vals.size()));
            Row row(t.columns.size());
            std::vector<bool> filled(t.columns.size(), false);
            for (std::size_t i = 0; i < targets.size(); ++i) {
                const Column& c = t.columns[static_cast<std::size_t>(targets[i])];
                checkType(c, vals[i]);
                row[static_cast<std::size_t>(targets[i])] = vals[i];
                filled[static_cast<std::size_t>(targets[i])] = true;
            }
            for (std::size_t i = 0; i < t.columns.size(); ++i) {
                if (filled[i]) continue;
                if (t.columns[i].primaryKey) throw std::runtime_error("primary key '" + t.columns[i].name + "' must be provided");
                row[i] = t.columns[i].type == Type::Int ? Value{0LL} : Value{std::string()};
            }
            t.insert(std::move(row));
            ++inserted;
        } while (p.acceptSymbol(","));
        std::cout << inserted << " row(s) inserted.\n";
        dirty_ = true;
    }

    void select(Parser& p) {
        std::vector<std::string> cols;
        bool star = p.acceptSymbol("*");
        if (!star) { do cols.push_back(p.expectIdent()); while (p.acceptSymbol(",")); }
        p.expectKeyword("FROM");
        Table& t = table(p.expectIdent());
        std::vector<int> proj;
        if (star) for (std::size_t i = 0; i < t.columns.size(); ++i) proj.push_back(static_cast<int>(i));
        else for (const auto& c : cols) proj.push_back(requireColumn(t, c));

        auto where = parseWhere(p);
        std::vector<std::size_t> rows = match(t, where);

        if (p.acceptKeyword("ORDER")) {
            p.expectKeyword("BY");
            const int oc = requireColumn(t, p.expectIdent());
            const bool desc = p.acceptKeyword("DESC") || (p.acceptKeyword("ASC"), false);
            std::stable_sort(rows.begin(), rows.end(), [&](std::size_t a, std::size_t b) {
                const int c = compareValues(t.rows[a][static_cast<std::size_t>(oc)], t.rows[b][static_cast<std::size_t>(oc)]);
                return desc ? c > 0 : c < 0;
            });
        }
        if (p.acceptKeyword("LIMIT")) {
            const Value v = p.expectValue();
            if (!std::holds_alternative<long long>(v) || std::get<long long>(v) < 0) throw std::runtime_error("LIMIT needs a non-negative integer");
            const auto limit = static_cast<std::size_t>(std::get<long long>(v));
            if (rows.size() > limit) rows.resize(limit);
        }

        // Pretty print with column widths.
        std::vector<std::size_t> width;
        for (int c : proj) width.push_back(t.columns[static_cast<std::size_t>(c)].name.size());
        for (std::size_t r : rows)
            for (std::size_t i = 0; i < proj.size(); ++i) width[i] = std::max(width[i], toString(t.rows[r][static_cast<std::size_t>(proj[i])]).size());
        auto line = [&]() {
            std::cout << '+';
            for (std::size_t w : width) std::cout << std::string(w + 2, '-') << '+';
            std::cout << "\n";
        };
        line();
        std::cout << '|';
        for (std::size_t i = 0; i < proj.size(); ++i) std::cout << ' ' << std::left << std::setw(static_cast<int>(width[i])) << t.columns[static_cast<std::size_t>(proj[i])].name << " |";
        std::cout << "\n";
        line();
        for (std::size_t r : rows) {
            std::cout << '|';
            for (std::size_t i = 0; i < proj.size(); ++i) {
                const Value& v = t.rows[r][static_cast<std::size_t>(proj[i])];
                const bool num = std::holds_alternative<long long>(v);
                std::cout << ' ' << (num ? std::right : std::left) << std::setw(static_cast<int>(width[i])) << toString(v) << " |";
            }
            std::cout << "\n";
        }
        line();
        std::cout << rows.size() << " row(s)" << (where && t.indexes.count(where->column) && where->op == "=" ? " (index used)" : "") << "\n";
    }

    void update(Parser& p) {
        Table& t = table(p.expectIdent());
        p.expectKeyword("SET");
        std::vector<std::pair<int, Value>> sets;
        do {
            const int ci = requireColumn(t, p.expectIdent());
            p.expectSymbol("=");
            Value v = p.expectValue();
            checkType(t.columns[static_cast<std::size_t>(ci)], v);
            sets.emplace_back(ci, std::move(v));
        } while (p.acceptSymbol(","));
        auto where = parseWhere(p);
        const auto rows = match(t, where);
        const int pk = t.primaryKeyIndex();
        for (const auto& [ci, v] : sets) {
            if (ci == pk && rows.size() > 1) throw std::runtime_error("cannot set the primary key on multiple rows");
            if (ci == pk && rows.size() == 1) {
                for (std::size_t r = 0; r < t.rows.size(); ++r)
                    if (r != rows[0] && compareValues(t.rows[r][static_cast<std::size_t>(pk)], v) == 0) throw std::runtime_error("duplicate primary key " + toString(v));
            }
        }
        for (std::size_t r : rows) for (const auto& [ci, v] : sets) t.rows[r][static_cast<std::size_t>(ci)] = v;
        t.rebuildIndexes();
        std::cout << rows.size() << " row(s) updated.\n";
        dirty_ = true;
    }

    void remove(Parser& p) {
        p.expectKeyword("FROM");
        Table& t = table(p.expectIdent());
        auto where = parseWhere(p);
        const auto rows = match(t, where);
        std::vector<bool> kill(t.rows.size(), false);
        for (std::size_t r : rows) kill[r] = true;
        std::vector<Row> kept;
        for (std::size_t r = 0; r < t.rows.size(); ++r) if (!kill[r]) kept.push_back(std::move(t.rows[r]));
        t.rows = std::move(kept);
        t.rebuildIndexes();
        std::cout << rows.size() << " row(s) deleted.\n";
        dirty_ = true;
    }

    void showTables() {
        if (tables_.empty()) { std::cout << "(no tables)\n"; return; }
        for (const auto& [name, t] : tables_) std::cout << name << "  (" << t->rows.size() << " rows, " << t->indexes.size() << " index(es))\n";
    }

    void describe(const std::string& name) {
        const Table& t = table(name);
        for (const Column& c : t.columns)
            std::cout << std::left << std::setw(20) << c.name << typeName(c.type) << (c.primaryKey ? "  PRIMARY KEY" : "") << (t.indexes.count(c.name) ? "  INDEXED" : "") << "\n";
    }

    static void help() {
        std::cout << "Statements (end each with ; or newline):\n"
                     "  CREATE TABLE users (id INT PRIMARY KEY, name TEXT, age INT)\n"
                     "  CREATE INDEX ON users (name)\n"
                     "  INSERT INTO users VALUES (1, 'Ada', 36), (2, 'Linus', 28)\n"
                     "  INSERT INTO users (id, name) VALUES (3, 'Grace')\n"
                     "  SELECT * FROM users WHERE age > 30 ORDER BY name DESC LIMIT 10\n"
                     "  SELECT name, age FROM users\n"
                     "  UPDATE users SET age = 37 WHERE id = 1\n"
                     "  DELETE FROM users WHERE name = 'Grace'\n"
                     "  SHOW TABLES | DESCRIBE users | DROP TABLE users\n"
                     "  SAVE | LOAD | HELP | EXIT\n"
                     "Strings use single quotes ('it''s'). Operators: = != < <= > >=\n";
    }
};

}  // namespace

int main(int argc, char** argv) {
    const std::string file = argc > 1 ? argv[1] : "mini.db";
    Database db(file);
    try {
        if (db.tryLoadIfExists()) std::cout << "Loaded database from " << file << ".\n";
        else std::cout << "Starting a new database (" << file << " will be created on SAVE).\n";
    } catch (const std::exception& e) {
        std::cerr << "Could not load " << file << ": " << e.what() << "\n";
        return 1;
    }
    std::cout << "Mini Database Engine. Type HELP for syntax, EXIT to quit.\n";

    std::string line;
    while (true) {
        std::cout << "db> ";
        if (!std::getline(std::cin, line)) { std::cout << "\n"; break; }
        // Each input line holds one or more ';'-separated statements.
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos) continue;
        std::string sql = line.substr(first);
        const std::string word = upper(sql.substr(0, sql.find_first_of(" ;\t")));
        if (word == "EXIT" || word == "QUIT") break;
        try {
            // Allow several ';'-separated statements per line.
            std::size_t start = 0;
            while (start < sql.size()) {
                const auto end = sql.find(';', start);
                db.execute(sql.substr(start, end == std::string::npos ? std::string::npos : end - start));
                if (end == std::string::npos) break;
                start = end + 1;
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }

    if (db.dirty()) {
        std::cout << "Unsaved changes. Save to " << file << "? (Y/n): ";
        if (std::getline(std::cin, line) && !(line.size() && std::tolower(static_cast<unsigned char>(line[0])) == 'n')) {
            try { db.save(); std::cout << "Saved.\n"; } catch (const std::exception& e) { std::cerr << "Save failed: " << e.what() << "\n"; return 1; }
        }
    }
    return 0;
}

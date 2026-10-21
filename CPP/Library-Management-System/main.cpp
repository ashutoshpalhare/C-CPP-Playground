// Library Management System (Intermediate C++)
//
// Manage books, members, loans and returns with persistent CSV-style
// storage. Demonstrates OOP, std::map / std::vector, std::optional,
// file streams, date arithmetic with <chrono>, and defensive input parsing.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char* const kBooksFile = "books.txt";
const char* const kMembersFile = "members.txt";
const char* const kLoansFile = "loans.txt";
constexpr int kLoanDays = 14;
constexpr int kFinePerDayCents = 25;
constexpr int kMaxLoansPerMember = 3;

// ---------- helpers ----------

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::vector<std::string> split(const std::string& line, char sep) {
    std::vector<std::string> out;
    std::string field;
    std::stringstream ss(line);
    while (std::getline(ss, field, sep)) out.push_back(field);
    if (!line.empty() && line.back() == sep) out.emplace_back();
    return out;
}

// Days since epoch (local date) - good enough for due-date arithmetic.
long today() {
    using namespace std::chrono;
    return duration_cast<hours>(system_clock::now().time_since_epoch()).count() / 24;
}

std::string dateString(long days) {
    const std::time_t t = static_cast<std::time_t>(days) * 86400;
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream os;
    os << (tm.tm_year + 1900) << '-' << std::setw(2) << std::setfill('0') << tm.tm_mon + 1 << '-' << std::setw(2) << tm.tm_mday;
    return os.str();
}

std::optional<int> toInt(const std::string& s) {
    try {
        std::size_t used = 0;
        const int v = std::stoi(s, &used);
        if (used == s.size()) return v;
    } catch (const std::exception&) {
    }
    return std::nullopt;
}

std::optional<long> toLong(const std::string& s) {
    try {
        std::size_t used = 0;
        const long v = std::stol(s, &used);
        if (used == s.size()) return v;
    } catch (const std::exception&) {
    }
    return std::nullopt;
}

// ---------- domain ----------

struct Book {
    int id = 0;
    std::string title, author, isbn;
    int copiesTotal = 1;
    int copiesAvailable = 1;
};

struct Member {
    int id = 0;
    std::string name, email;
};

struct Loan {
    int id = 0;
    int bookId = 0;
    int memberId = 0;
    long issuedDay = 0;
    long dueDay = 0;
    long returnedDay = -1;  // -1 = still out
};

class Library {
public:
    // ----- persistence -----
    bool loadAll() {
        bool ok = true;
        ok &= loadFile(kBooksFile, [this](const std::vector<std::string>& f) {
            if (f.size() < 6) return false;
            Book b;
            auto id = toInt(f[0]), total = toInt(f[4]), avail = toInt(f[5]);
            if (!id || !total || !avail || *id <= 0 || *total < 0 || *avail < 0 || *avail > *total) return false;
            b.id = *id; b.title = f[1]; b.author = f[2]; b.isbn = f[3]; b.copiesTotal = *total; b.copiesAvailable = *avail;
            books_[b.id] = b;
            nextBookId_ = std::max(nextBookId_, b.id + 1);
            return true;
        });
        ok &= loadFile(kMembersFile, [this](const std::vector<std::string>& f) {
            if (f.size() < 3) return false;
            auto id = toInt(f[0]);
            if (!id || *id <= 0 || f[1].empty()) return false;
            members_[*id] = Member{*id, f[1], f[2]};
            nextMemberId_ = std::max(nextMemberId_, *id + 1);
            return true;
        });
        ok &= loadFile(kLoansFile, [this](const std::vector<std::string>& f) {
            if (f.size() < 6) return false;
            auto id = toInt(f[0]), b = toInt(f[1]), m = toInt(f[2]);
            auto issued = toLong(f[3]), due = toLong(f[4]), ret = toLong(f[5]);
            if (!id || !b || !m || !issued || !due || !ret) return false;
            loans_.push_back(Loan{*id, *b, *m, *issued, *due, *ret});
            nextLoanId_ = std::max(nextLoanId_, *id + 1);
            return true;
        });
        return ok;
    }

    bool saveAll() const {
        std::ofstream b(kBooksFile, std::ios::trunc), m(kMembersFile, std::ios::trunc), l(kLoansFile, std::ios::trunc);
        if (!b || !m || !l) {
            std::cerr << "Error: cannot open data files for writing.\n";
            return false;
        }
        for (const auto& [id, bk] : books_)
            b << bk.id << '|' << bk.title << '|' << bk.author << '|' << bk.isbn << '|' << bk.copiesTotal << '|' << bk.copiesAvailable << '\n';
        for (const auto& [id, mb] : members_) m << mb.id << '|' << mb.name << '|' << mb.email << '\n';
        for (const auto& ln : loans_)
            l << ln.id << '|' << ln.bookId << '|' << ln.memberId << '|' << ln.issuedDay << '|' << ln.dueDay << '|' << ln.returnedDay << '\n';
        return b.good() && m.good() && l.good();
    }

    // ----- books -----
    int addBook(const std::string& title, const std::string& author, const std::string& isbn, int copies) {
        Book b;
        b.id = nextBookId_++;
        b.title = title; b.author = author; b.isbn = isbn;
        b.copiesTotal = b.copiesAvailable = copies;
        books_[b.id] = b;
        return b.id;
    }

    bool removeBook(int id, std::string& error) {
        auto it = books_.find(id);
        if (it == books_.end()) { error = "No such book."; return false; }
        if (it->second.copiesAvailable != it->second.copiesTotal) { error = "Book has copies on loan."; return false; }
        books_.erase(it);
        return true;
    }

    std::vector<const Book*> searchBooks(const std::string& term) const {
        std::vector<const Book*> out;
        const std::string t = lower(term);
        for (const auto& [id, b] : books_) {
            if (t.empty() || lower(b.title).find(t) != std::string::npos || lower(b.author).find(t) != std::string::npos || b.isbn.find(term) != std::string::npos)
                out.push_back(&b);
        }
        return out;
    }

    // ----- members -----
    int addMember(const std::string& name, const std::string& email) {
        const int id = nextMemberId_++;
        members_[id] = Member{id, name, email};
        return id;
    }

    const std::map<int, Member>& members() const { return members_; }
    const std::map<int, Book>& books() const { return books_; }

    // ----- loans -----
    bool issue(int bookId, int memberId, std::string& error) {
        auto b = books_.find(bookId);
        if (b == books_.end()) { error = "No such book."; return false; }
        if (!members_.count(memberId)) { error = "No such member."; return false; }
        if (b->second.copiesAvailable <= 0) { error = "No copies available."; return false; }
        int active = 0;
        for (const Loan& l : loans_) {
            if (l.returnedDay >= 0) continue;
            if (l.memberId == memberId) ++active;
            if (l.memberId == memberId && l.dueDay < today()) { error = "Member has overdue books."; return false; }
        }
        if (active >= kMaxLoansPerMember) { error = "Member already has " + std::to_string(kMaxLoansPerMember) + " books."; return false; }
        Loan l;
        l.id = nextLoanId_++;
        l.bookId = bookId; l.memberId = memberId;
        l.issuedDay = today(); l.dueDay = l.issuedDay + kLoanDays;
        loans_.push_back(l);
        --b->second.copiesAvailable;
        return true;
    }

    // Returns fine in cents via out param.
    bool returnBook(int loanId, int& fineCents, std::string& error) {
        auto it = std::find_if(loans_.begin(), loans_.end(), [loanId](const Loan& l) { return l.id == loanId; });
        if (it == loans_.end()) { error = "No such loan."; return false; }
        if (it->returnedDay >= 0) { error = "Loan already returned."; return false; }
        it->returnedDay = today();
        const long late = it->returnedDay - it->dueDay;
        fineCents = late > 0 ? static_cast<int>(late) * kFinePerDayCents : 0;
        auto b = books_.find(it->bookId);
        if (b != books_.end()) ++b->second.copiesAvailable;
        return true;
    }

    std::vector<const Loan*> activeLoans(bool overdueOnly) const {
        std::vector<const Loan*> out;
        for (const Loan& l : loans_)
            if (l.returnedDay < 0 && (!overdueOnly || l.dueDay < today())) out.push_back(&l);
        return out;
    }

    std::string bookTitle(int id) const { auto it = books_.find(id); return it == books_.end() ? "(deleted)" : it->second.title; }
    std::string memberName(int id) const { auto it = members_.find(id); return it == members_.end() ? "(deleted)" : it->second.name; }

private:
    std::map<int, Book> books_;
    std::map<int, Member> members_;
    std::vector<Loan> loans_;
    int nextBookId_ = 1, nextMemberId_ = 1, nextLoanId_ = 1;

    template <typename Fn>
    static bool loadFile(const char* path, Fn parse) {
        std::ifstream in(path);
        if (!in) return true;  // missing file = empty on first run
        std::string line;
        int n = 0;
        bool ok = true;
        while (std::getline(in, line)) {
            ++n;
            if (trim(line).empty()) continue;
            if (!parse(split(line, '|'))) {
                std::cerr << "Warning: skipped malformed line " << n << " in " << path << "\n";
                ok = false;
            }
        }
        return ok;
    }
};

// ---------- UI ----------

bool ask(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    out = trim(out);
    return true;
}

bool askInt(const std::string& prompt, int lo, int hi, int& out) {
    std::string line;
    while (ask(prompt, line)) {
        auto v = toInt(line);
        if (v && *v >= lo && *v <= hi) { out = *v; return true; }
        std::cout << "Enter a whole number between " << lo << " and " << hi << ".\n";
    }
    return false;
}

bool noPipe(const std::string& s) { return s.find('|') == std::string::npos; }

void printBooks(const std::vector<const Book*>& list) {
    if (list.empty()) { std::cout << "No books found.\n"; return; }
    std::cout << std::left << std::setw(5) << "ID" << std::setw(32) << "Title" << std::setw(22) << "Author" << std::setw(15) << "ISBN" << "Avail/Total\n";
    for (const Book* b : list)
        std::cout << std::left << std::setw(5) << b->id << std::setw(32) << b->title.substr(0, 30) << std::setw(22) << b->author.substr(0, 20)
                  << std::setw(15) << b->isbn << b->copiesAvailable << '/' << b->copiesTotal << '\n';
}

void menu() {
    std::cout << "\n=== Library Management ===\n"
                 " 1. List/search books      2. Add book          3. Remove book\n"
                 " 4. List members           5. Add member\n"
                 " 6. Issue book             7. Return book       8. Active loans\n"
                 " 9. Overdue loans         10. Save              0. Quit\n";
}

}  // namespace

int main() {
    Library lib;
    lib.loadAll();
    std::cout << "Loaded " << lib.books().size() << " books, " << lib.members().size() << " members, "
              << lib.activeLoans(false).size() << " active loans.\n";

    int choice = 0;
    std::string s1, s2, s3, err;
    while (true) {
        menu();
        if (!askInt("Choose: ", 0, 10, choice) || choice == 0) break;
        switch (choice) {
            case 1:
                if (!ask("Search (empty for all): ", s1)) return 0;
                printBooks(lib.searchBooks(s1));
                break;
            case 2: {
                int copies;
                if (!ask("Title: ", s1) || !ask("Author: ", s2) || !ask("ISBN: ", s3) || !askInt("Copies (1-100): ", 1, 100, copies)) return 0;
                if (s1.empty() || !noPipe(s1) || !noPipe(s2) || !noPipe(s3)) { std::cout << "Title is required; fields may not contain '|'.\n"; break; }
                std::cout << "Added book #" << lib.addBook(s1, s2, s3, copies) << ".\n";
                break;
            }
            case 3: {
                int id;
                if (!askInt("Book ID: ", 1, 1000000, id)) return 0;
                std::cout << (lib.removeBook(id, err) ? "Removed." : err) << "\n";
                break;
            }
            case 4:
                if (lib.members().empty()) std::cout << "No members.\n";
                for (const auto& [id, m] : lib.members()) std::cout << std::setw(4) << id << "  " << m.name << "  <" << m.email << ">\n";
                break;
            case 5:
                if (!ask("Name: ", s1) || !ask("Email: ", s2)) return 0;
                if (s1.empty() || !noPipe(s1) || !noPipe(s2)) { std::cout << "Name is required; no '|' allowed.\n"; break; }
                std::cout << "Added member #" << lib.addMember(s1, s2) << ".\n";
                break;
            case 6: {
                int b, m;
                if (!askInt("Book ID: ", 1, 1000000, b) || !askInt("Member ID: ", 1, 1000000, m)) return 0;
                if (lib.issue(b, m, err)) std::cout << "Issued. Due on " << dateString(today() + kLoanDays) << ".\n";
                else std::cout << "Cannot issue: " << err << "\n";
                break;
            }
            case 7: {
                int id, fine = 0;
                if (!askInt("Loan ID: ", 1, 1000000, id)) return 0;
                if (lib.returnBook(id, fine, err)) {
                    std::cout << "Returned.";
                    if (fine > 0) std::cout << " Late fine: $" << fine / 100 << '.' << std::setw(2) << std::setfill('0') << fine % 100 << std::setfill(' ');
                    std::cout << "\n";
                } else std::cout << "Cannot return: " << err << "\n";
                break;
            }
            case 8:
            case 9: {
                const auto loans = lib.activeLoans(choice == 9);
                if (loans.empty()) { std::cout << "None.\n"; break; }
                std::cout << std::left << std::setw(6) << "Loan" << std::setw(30) << "Book" << std::setw(20) << "Member" << std::setw(12) << "Due" << "Status\n";
                for (const Loan* l : loans) {
                    const long late = today() - l->dueDay;
                    std::cout << std::left << std::setw(6) << l->id << std::setw(30) << lib.bookTitle(l->bookId).substr(0, 28) << std::setw(20)
                              << lib.memberName(l->memberId).substr(0, 18) << std::setw(12) << dateString(l->dueDay)
                              << (late > 0 ? "OVERDUE " + std::to_string(late) + "d" : "on time") << "\n";
                }
                break;
            }
            case 10:
                std::cout << (lib.saveAll() ? "Saved." : "Save failed.") << "\n";
                break;
        }
    }
    std::cout << (lib.saveAll() ? "Data saved. Goodbye.\n" : "WARNING: data could not be saved.\n");
    return 0;
}

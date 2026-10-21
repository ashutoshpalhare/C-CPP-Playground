// Bank Management System (Intermediate C++)
//
// Multiple accounts (savings / current), deposits, withdrawals, transfers,
// an append-only transaction ledger, PIN verification, and persistence.
// Money is kept in integer cents. Demonstrates inheritance and polymorphism
// (Account -> SavingsAccount / CurrentAccount), smart pointers, std::map,
// exceptions and file I/O.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Cents = long long;
const char* const kAccountsFile = "accounts.txt";
const char* const kLedgerFile = "ledger.txt";

std::string money(Cents c) {
    std::ostringstream os;
    if (c < 0) { os << '-'; c = -c; }
    os << c / 100 << '.' << std::setw(2) << std::setfill('0') << c % 100;
    return os.str();
}

bool parseMoney(const std::string& text, Cents& out) {
    std::size_t i = 0;
    Cents dollars = 0, cents = 0;
    int cd = 0;
    bool digit = false;
    for (; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])); ++i) {
        dollars = dollars * 10 + (text[i] - '0');
        digit = true;
        if (dollars > 1'000'000'000LL) return false;
    }
    if (i < text.size() && text[i] == '.') {
        for (++i; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])); ++i, ++cd) {
            if (cd < 2) cents = cents * 10 + (text[i] - '0');
            else if (text[i] != '0') return false;
            digit = true;
        }
        if (cd == 1) cents *= 10;
    }
    if (i != text.size() || !digit) return false;
    out = dollars * 100 + cents;
    return true;
}

std::string timestamp() {
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

// ---------- accounts ----------

class Account {
public:
    Account(int number, std::string holder, std::string pin, Cents balance)
        : number_(number), holder_(std::move(holder)), pin_(std::move(pin)), balance_(balance) {}
    virtual ~Account() = default;

    int number() const { return number_; }
    const std::string& holder() const { return holder_; }
    Cents balance() const { return balance_; }
    bool checkPin(const std::string& pin) const { return pin == pin_; }
    const std::string& pin() const { return pin_; }

    virtual std::string type() const = 0;
    virtual Cents minimumBalance() const = 0;  // lowest balance allowed after a withdrawal
    virtual void monthEnd() = 0;               // interest or fees

    void deposit(Cents amount) {
        if (amount <= 0) throw std::invalid_argument("Amount must be positive");
        balance_ += amount;
    }
    void withdraw(Cents amount) {
        if (amount <= 0) throw std::invalid_argument("Amount must be positive");
        if (balance_ - amount < minimumBalance())
            throw std::runtime_error("Insufficient funds (minimum balance " + money(minimumBalance()) + ")");
        balance_ -= amount;
    }

protected:
    void adjust(Cents delta) { balance_ += delta; }

private:
    int number_;
    std::string holder_;
    std::string pin_;
    Cents balance_;
};

class SavingsAccount : public Account {
public:
    using Account::Account;
    std::string type() const override { return "Savings"; }
    Cents minimumBalance() const override { return 0; }
    void monthEnd() override {  // 0.3% monthly interest (~3.6% p.a.)
        const Cents interest = static_cast<Cents>(std::llround(balance() * 0.003));
        if (interest > 0) adjust(interest);
    }
};

class CurrentAccount : public Account {
public:
    using Account::Account;
    std::string type() const override { return "Current"; }
    Cents minimumBalance() const override { return -50000; }  // $500 overdraft
    void monthEnd() override {  // flat fee, plus overdraft interest 1% of negative balance
        adjust(-500);
        if (balance() < 0) adjust(static_cast<Cents>(std::llround(balance() * 0.01)));
    }
};

// ---------- bank ----------

class Bank {
public:
    bool load() {
        std::ifstream in(kAccountsFile);
        if (!in) return true;
        std::string line;
        int n = 0;
        while (std::getline(in, line)) {
            ++n;
            if (line.empty()) continue;
            std::vector<std::string> f;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, '|')) f.push_back(part);
            try {
                if (f.size() != 5) throw std::runtime_error("field count");
                const int number = std::stoi(f[0]);
                const Cents bal = std::stoll(f[4]);
                std::unique_ptr<Account> acct;
                if (f[1] == "S") acct = std::make_unique<SavingsAccount>(number, f[2], f[3], bal);
                else if (f[1] == "C") acct = std::make_unique<CurrentAccount>(number, f[2], f[3], bal);
                else throw std::runtime_error("type");
                nextNumber_ = std::max(nextNumber_, number + 1);
                accounts_[number] = std::move(acct);
            } catch (const std::exception&) {
                std::cerr << "Warning: skipping malformed line " << n << " of " << kAccountsFile << "\n";
            }
        }
        return true;
    }

    bool save() const {
        std::ofstream out(kAccountsFile, std::ios::trunc);
        if (!out) { std::cerr << "Error: cannot write " << kAccountsFile << "\n"; return false; }
        for (const auto& [num, a] : accounts_)
            out << num << '|' << (a->type() == "Savings" ? 'S' : 'C') << '|' << a->holder() << '|' << a->pin() << '|' << a->balance() << '\n';
        return out.good();
    }

    Account& open(bool savings, const std::string& holder, const std::string& pin, Cents initial) {
        const int number = nextNumber_++;
        std::unique_ptr<Account> acct;
        if (savings) acct = std::make_unique<SavingsAccount>(number, holder, pin, 0);
        else acct = std::make_unique<CurrentAccount>(number, holder, pin, 0);
        if (initial > 0) acct->deposit(initial);
        Account& ref = *acct;
        accounts_[number] = std::move(acct);
        log(number, "OPEN", initial, ref.balance(), "");
        return ref;
    }

    Account* find(int number) {
        auto it = accounts_.find(number);
        return it == accounts_.end() ? nullptr : it->second.get();
    }

    Account& authenticate(int number, const std::string& pin) {
        Account* a = find(number);
        if (!a) throw std::runtime_error("Account not found");
        if (!a->checkPin(pin)) throw std::runtime_error("Incorrect PIN");
        return *a;
    }

    void deposit(Account& a, Cents amt) { a.deposit(amt); log(a.number(), "DEPOSIT", amt, a.balance(), ""); }
    void withdraw(Account& a, Cents amt) { a.withdraw(amt); log(a.number(), "WITHDRAW", -amt, a.balance(), ""); }

    void transfer(Account& from, int toNumber, Cents amt) {
        Account* to = find(toNumber);
        if (!to) throw std::runtime_error("Destination account not found");
        if (to == &from) throw std::invalid_argument("Cannot transfer to the same account");
        from.withdraw(amt);  // may throw; nothing changed yet on 'to'
        to->deposit(amt);
        log(from.number(), "TRANSFER_OUT", -amt, from.balance(), "to " + std::to_string(toNumber));
        log(to->number(), "TRANSFER_IN", amt, to->balance(), "from " + std::to_string(from.number()));
    }

    bool close(int number, const std::string& pin, std::string& err) {
        Account* a = find(number);
        if (!a || !a->checkPin(pin)) { err = "Account not found or wrong PIN."; return false; }
        if (a->balance() != 0) { err = "Balance must be exactly 0.00 to close (current " + money(a->balance()) + ")."; return false; }
        log(number, "CLOSE", 0, 0, "");
        accounts_.erase(number);
        return true;
    }

    void monthEnd() {
        for (auto& [num, a] : accounts_) {
            const Cents before = a->balance();
            a->monthEnd();
            log(num, "MONTH_END", a->balance() - before, a->balance(), a->type());
        }
    }

    void listAccounts() const {
        if (accounts_.empty()) { std::cout << "No accounts.\n"; return; }
        std::cout << std::left << std::setw(8) << "Number" << std::setw(10) << "Type" << std::setw(24) << "Holder" << std::right << std::setw(14) << "Balance\n";
        for (const auto& [num, a] : accounts_)
            std::cout << std::left << std::setw(8) << num << std::setw(10) << a->type() << std::setw(24) << a->holder().substr(0, 22) << std::right << std::setw(14) << money(a->balance()) << "\n";
    }

    void printHistory(int number) const {
        std::ifstream in(kLedgerFile);
        if (!in) { std::cout << "No transactions recorded yet.\n"; return; }
        std::string line;
        const std::string prefix = std::to_string(number) + "|";
        int shown = 0;
        while (std::getline(in, line)) {
            if (line.compare(0, prefix.size(), prefix) != 0) continue;
            std::vector<std::string> f;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, '|')) f.push_back(part);
            if (f.size() < 5) continue;
            std::cout << f[1] << "  " << std::left << std::setw(13) << f[2] << std::right << std::setw(12) << money(std::stoll(f[3])) << std::setw(14) << money(std::stoll(f[4])) << "  " << (f.size() > 5 ? f[5] : "") << "\n";
            ++shown;
        }
        if (shown == 0) std::cout << "No transactions for account " << number << ".\n";
    }

private:
    std::map<int, std::unique_ptr<Account>> accounts_;
    int nextNumber_ = 1001;

    static void log(int number, const std::string& kind, Cents delta, Cents after, const std::string& note) {
        std::ofstream out(kLedgerFile, std::ios::app);
        if (!out) { std::cerr << "Warning: could not append to ledger.\n"; return; }
        out << number << '|' << timestamp() << '|' << kind << '|' << delta << '|' << after << '|' << note << '\n';
    }
};

// ---------- UI ----------

bool ask(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    const auto a = out.find_first_not_of(" \t\r\n");
    out = (a == std::string::npos) ? "" : out.substr(a, out.find_last_not_of(" \t\r\n") - a + 1);
    return true;
}

bool askInt(const std::string& prompt, int& out) {
    std::string s;
    if (!ask(prompt, s)) return false;
    try {
        std::size_t used = 0;
        out = std::stoi(s, &used);
        if (used != s.size()) throw std::invalid_argument("trailing");
    } catch (const std::exception&) {
        throw std::invalid_argument("Please enter a whole number");
    }
    return true;
}

bool askMoney(const std::string& prompt, Cents& out) {
    std::string s;
    if (!ask(prompt, s)) return false;
    if (!parseMoney(s, out) || out <= 0) throw std::invalid_argument("Enter a positive amount like 100 or 99.95");
    return true;
}

bool validPin(const std::string& pin) {
    return pin.size() == 4 && std::all_of(pin.begin(), pin.end(), [](unsigned char c) { return std::isdigit(c); });
}

}  // namespace

int main() {
    Bank bank;
    bank.load();
    std::cout << "=== Bank Management System ===\n";

    std::string line, holder, pin;
    while (true) {
        std::cout << "\n1. Open account   2. Deposit   3. Withdraw   4. Transfer   5. Balance\n"
                     "6. History        7. List all  8. Close      9. Month-end  0. Quit\nChoose: ";
        if (!std::getline(std::cin, line)) break;
        try {
            if (line == "0") break;
            if (line == "1") {
                if (!ask("Type (s=savings, c=current): ", line) || !ask("Holder name: ", holder) || !ask("4-digit PIN: ", pin)) break;
                if (holder.empty() || holder.find('|') != std::string::npos) throw std::invalid_argument("Holder name is required and may not contain '|'");
                if (!validPin(pin)) throw std::invalid_argument("PIN must be exactly 4 digits");
                Cents initial = 0;
                std::string s;
                if (!ask("Initial deposit (0 for none): ", s)) break;
                if (!parseMoney(s, initial)) throw std::invalid_argument("Bad amount");
                const bool savings = !line.empty() && std::tolower(static_cast<unsigned char>(line[0])) == 's';
                Account& a = bank.open(savings, holder, pin, initial);
                std::cout << a.type() << " account " << a.number() << " opened with balance " << money(a.balance()) << ".\n";
            } else if (line == "2" || line == "3" || line == "4" || line == "5" || line == "6") {
                int number;
                if (!askInt("Account number: ", number) || !ask("PIN: ", pin)) break;
                Account& a = bank.authenticate(number, pin);
                if (line == "5") { std::cout << a.type() << " account " << a.number() << " (" << a.holder() << "): " << money(a.balance()) << "\n"; continue; }
                if (line == "6") { bank.printHistory(number); continue; }
                Cents amt;
                if (!askMoney("Amount: ", amt)) break;
                if (line == "2") bank.deposit(a, amt);
                else if (line == "3") bank.withdraw(a, amt);
                else {
                    int to;
                    if (!askInt("Destination account: ", to)) break;
                    bank.transfer(a, to, amt);
                }
                std::cout << "OK. New balance: " << money(a.balance()) << "\n";
            } else if (line == "7") {
                bank.listAccounts();
            } else if (line == "8") {
                int number;
                std::string err;
                if (!askInt("Account number: ", number) || !ask("PIN: ", pin)) break;
                std::cout << (bank.close(number, pin, err) ? "Account closed." : err) << "\n";
            } else if (line == "9") {
                bank.monthEnd();
                std::cout << "Month-end processing complete (interest and fees applied).\n";
            } else {
                std::cout << "Unknown option.\n";
                continue;
            }
            bank.save();
        } catch (const std::invalid_argument& e) {
            std::cout << "Invalid input: " << e.what() << "\n";
        } catch (const std::runtime_error& e) {
            std::cout << "Declined: " << e.what() << "\n";
        }
    }
    std::cout << (bank.save() ? "Data saved. Goodbye.\n" : "WARNING: data could not be saved.\n");
    return 0;
}

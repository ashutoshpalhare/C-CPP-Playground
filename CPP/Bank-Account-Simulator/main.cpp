// Bank Account Simulator (Beginner C++)
//
// Models a single bank account with deposits, withdrawals, interest,
// an overdraft limit and a transaction history. Focuses on encapsulation,
// validation and clear error reporting. Money is stored in integer cents
// to avoid floating-point rounding errors.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Cents = long long;

std::string money(Cents c) {
    std::ostringstream os;
    const bool negative = c < 0;
    if (negative) c = -c;
    os << (negative ? "-" : "") << "$" << c / 100 << '.' << std::setw(2) << std::setfill('0') << c % 100;
    return os.str();
}

// Parses "12.34" / "12" / "0.5" into cents. Returns false on bad input.
bool parseMoney(const std::string& text, Cents& out) {
    if (text.empty()) return false;
    std::size_t i = 0;
    Cents dollars = 0, cents = 0;
    int centDigits = 0;
    bool anyDigit = false;
    for (; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])); ++i) {
        dollars = dollars * 10 + (text[i] - '0');
        anyDigit = true;
        if (dollars > 1'000'000'000LL) return false;  // sanity cap: 1 billion dollars
    }
    if (i < text.size() && text[i] == '.') {
        ++i;
        for (; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])); ++i) {
            if (centDigits < 2) cents = cents * 10 + (text[i] - '0');
            else if (text[i] != '0') return false;  // more than 2 decimals
            ++centDigits;
            anyDigit = true;
        }
        if (centDigits == 1) cents *= 10;
    }
    if (i != text.size() || !anyDigit) return false;
    out = dollars * 100 + cents;
    return true;
}

struct Transaction {
    std::string type;
    Cents amount;
    Cents balanceAfter;
    std::string note;
};

class BankAccount {
public:
    BankAccount(std::string owner, Cents openingBalance, Cents overdraftLimit)
        : owner_(std::move(owner)), balance_(0), overdraftLimit_(overdraftLimit) {
        if (openingBalance < 0) throw std::invalid_argument("Opening balance cannot be negative");
        if (overdraftLimit < 0) throw std::invalid_argument("Overdraft limit cannot be negative");
        if (openingBalance > 0) record("Open", openingBalance, "Opening deposit");
    }

    const std::string& owner() const { return owner_; }
    Cents balance() const { return balance_; }
    Cents overdraftLimit() const { return overdraftLimit_; }
    const std::vector<Transaction>& history() const { return history_; }

    void deposit(Cents amount) {
        if (amount <= 0) throw std::invalid_argument("Deposit must be greater than zero");
        if (amount > 1'000'000'00LL) throw std::invalid_argument("Single deposits are limited to $1,000,000.00");
        record("Deposit", amount, "");
    }

    void withdraw(Cents amount) {
        if (amount <= 0) throw std::invalid_argument("Withdrawal must be greater than zero");
        if (balance_ - amount < -overdraftLimit_) {
            throw std::runtime_error("Insufficient funds: available " + money(balance_ + overdraftLimit_) +
                                     " (including overdraft)");
        }
        record("Withdraw", -amount, balance_ - amount < 0 ? "Overdraft used" : "");
    }

    // Applies annual interest for a number of months (simple, pro-rata).
    void applyInterest(double annualRatePercent, int months) {
        if (annualRatePercent < 0 || annualRatePercent > 100) throw std::invalid_argument("Rate must be 0-100%");
        if (months < 1 || months > 600) throw std::invalid_argument("Months must be 1-600");
        if (balance_ <= 0) throw std::runtime_error("No positive balance to earn interest on");
        const double interest = static_cast<double>(balance_) * annualRatePercent / 100.0 * months / 12.0;
        const Cents rounded = static_cast<Cents>(std::llround(interest));
        if (rounded <= 0) throw std::runtime_error("Interest rounds to zero");
        std::ostringstream note;
        note << std::fixed << std::setprecision(2) << annualRatePercent << "% for " << months << " month(s)";
        record("Interest", rounded, note.str());
    }

    void setOverdraftLimit(Cents limit) {
        if (limit < 0) throw std::invalid_argument("Overdraft limit cannot be negative");
        if (balance_ < -limit) throw std::runtime_error("Current balance already exceeds the new limit");
        overdraftLimit_ = limit;
    }

private:
    std::string owner_;
    Cents balance_;
    Cents overdraftLimit_;
    std::vector<Transaction> history_;

    void record(const std::string& type, Cents delta, const std::string& note) {
        balance_ += delta;
        history_.push_back({type, delta, balance_, note});
    }
};

// ---------- UI helpers ----------

bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    const auto a = out.find_first_not_of(" \t\r\n");
    out = (a == std::string::npos) ? "" : out.substr(a, out.find_last_not_of(" \t\r\n") - a + 1);
    return true;
}

// Returns false on EOF; sets ok=false on invalid value.
bool readMoney(const std::string& prompt, Cents& out, bool& ok) {
    std::string line;
    if (!readLine(prompt, line)) return false;
    ok = parseMoney(line, out);
    if (!ok) std::cout << "Enter an amount like 25 or 25.50 (no sign or currency symbol).\n";
    return true;
}

void printStatement(const BankAccount& acct) {
    std::cout << "\nStatement for " << acct.owner() << "\n";
    std::cout << std::left << std::setw(10) << "Type" << std::right << std::setw(14) << "Amount" << std::setw(16) << "Balance" << "  Note\n";
    std::cout << std::string(60, '-') << "\n";
    for (const auto& t : acct.history()) {
        std::cout << std::left << std::setw(10) << t.type << std::right << std::setw(14) << money(t.amount) << std::setw(16) << money(t.balanceAfter) << "  " << t.note << "\n";
    }
    if (acct.history().empty()) std::cout << "(no transactions yet)\n";
    std::cout << "Current balance: " << money(acct.balance()) << "   Overdraft limit: " << money(acct.overdraftLimit()) << "\n";
}

}  // namespace

int main() {
    std::cout << "=== Bank Account Simulator ===\n";
    std::string owner;
    if (!readLine("Account holder name: ", owner)) return 0;
    if (owner.empty()) owner = "Customer";

    Cents opening = 0;
    bool ok = false;
    while (!ok) {
        if (!readMoney("Opening deposit (0 for none): ", opening, ok)) return 0;
    }

    BankAccount account(owner, opening, 10000);  // $100.00 default overdraft
    std::cout << "Account opened for " << owner << " with " << money(account.balance()) << ".\n";

    std::string line;
    while (true) {
        std::cout << "\n1. Deposit\n2. Withdraw\n3. Balance\n4. Apply interest\n5. Set overdraft limit\n6. Statement\n0. Quit\nChoose: ";
        if (!std::getline(std::cin, line)) break;
        try {
            if (line == "0") break;
            if (line == "1" || line == "2") {
                Cents amount;
                if (!readMoney("Amount: ", amount, ok)) break;
                if (!ok) continue;
                if (line == "1") account.deposit(amount);
                else account.withdraw(amount);
                std::cout << "Done. Balance: " << money(account.balance()) << "\n";
            } else if (line == "3") {
                std::cout << "Balance: " << money(account.balance()) << " (available incl. overdraft: "
                          << money(account.balance() + account.overdraftLimit()) << ")\n";
            } else if (line == "4") {
                std::string rate, months;
                if (!readLine("Annual rate % (e.g. 3.5): ", rate) || !readLine("Months: ", months)) break;
                account.applyInterest(std::stod(rate), std::stoi(months));
                std::cout << "Interest applied. Balance: " << money(account.balance()) << "\n";
            } else if (line == "5") {
                Cents limit;
                if (!readMoney("New overdraft limit: ", limit, ok)) break;
                if (!ok) continue;
                account.setOverdraftLimit(limit);
                std::cout << "Overdraft limit is now " << money(limit) << ".\n";
            } else if (line == "6") {
                printStatement(account);
            } else {
                std::cout << "Unknown option.\n";
            }
        } catch (const std::invalid_argument& e) {
            std::cout << "Invalid input: " << e.what() << "\n";
        } catch (const std::out_of_range&) {
            std::cout << "Invalid input: number out of range.\n";
        } catch (const std::runtime_error& e) {
            std::cout << "Declined: " << e.what() << "\n";
        }
    }

    printStatement(account);
    std::cout << "Thank you for banking with us.\n";
    return 0;
}

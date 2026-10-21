// Employee Payroll System (Intermediate C++)
//
// Models different employee types with an abstract base class and
// polymorphic pay calculation, computes progressive tax, prints payslips
// and summary reports, and persists records to employees.txt.
//
// Concepts: inheritance, pure virtual functions, std::unique_ptr, factory
// function, std::vector, sorting, file I/O, exceptions. Money is handled in
// integer cents.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Cents = long long;
const char* const kFile = "employees.txt";

std::string money(Cents c) {
    std::ostringstream os;
    if (c < 0) { os << '-'; c = -c; }
    os << c / 100 << '.' << std::setw(2) << std::setfill('0') << c % 100;
    return os.str();
}

Cents toCents(double amount) { return static_cast<Cents>(std::llround(amount * 100.0)); }

// Simple progressive monthly tax: 0% up to 1000, 10% to 3000, 20% to 6000, 30% above.
Cents monthlyTax(Cents gross) {
    struct Band { Cents upTo; double rate; };
    static const Band bands[] = {{100000, 0.0}, {300000, 0.10}, {600000, 0.20}, {-1, 0.30}};
    Cents tax = 0, lower = 0;
    for (const Band& b : bands) {
        const Cents upper = b.upTo < 0 ? gross : std::min(gross, b.upTo);
        if (gross > lower) tax += static_cast<Cents>(std::llround((upper - lower) * b.rate));
        lower = b.upTo;
        if (b.upTo < 0 || gross <= b.upTo) break;
    }
    return tax;
}

// ---------- employee hierarchy ----------

class Employee {
public:
    Employee(int id, std::string name, std::string department) : id_(id), name_(std::move(name)), dept_(std::move(department)) {}
    virtual ~Employee() = default;

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& department() const { return dept_; }

    virtual std::string type() const = 0;
    virtual Cents grossPay() const = 0;        // monthly gross
    virtual std::string payDetails() const = 0;
    virtual std::string serialize() const = 0;  // extra fields for the data file

    Cents tax() const { return monthlyTax(grossPay()); }
    Cents netPay() const { return grossPay() - tax(); }

private:
    int id_;
    std::string name_, dept_;
};

class SalariedEmployee : public Employee {
public:
    SalariedEmployee(int id, std::string n, std::string d, Cents annual) : Employee(id, std::move(n), std::move(d)), annual_(annual) {
        if (annual < 0) throw std::invalid_argument("Salary cannot be negative");
    }
    std::string type() const override { return "Salaried"; }
    Cents grossPay() const override { return annual_ / 12; }
    std::string payDetails() const override { return "annual salary " + money(annual_); }
    std::string serialize() const override { return "S|" + std::to_string(annual_); }
private:
    Cents annual_;
};

class HourlyEmployee : public Employee {
public:
    HourlyEmployee(int id, std::string n, std::string d, Cents rate, double hours)
        : Employee(id, std::move(n), std::move(d)), rate_(rate), hours_(hours) {
        if (rate < 0 || hours < 0 || hours > 744) throw std::invalid_argument("Invalid rate or hours");
    }
    std::string type() const override { return "Hourly"; }
    Cents grossPay() const override {  // overtime (>160 h) paid at 1.5x
        const double regular = std::min(hours_, 160.0), overtime = std::max(0.0, hours_ - 160.0);
        return static_cast<Cents>(std::llround(regular * rate_ + overtime * rate_ * 1.5));
    }
    std::string payDetails() const override {
        std::ostringstream os;
        os << std::fixed << std::setprecision(1) << hours_ << " h @ " << money(rate_) << "/h";
        if (hours_ > 160.0) os << " (incl. " << hours_ - 160.0 << " h overtime at 1.5x)";
        return os.str();
    }
    std::string serialize() const override { return "H|" + std::to_string(rate_) + "|" + std::to_string(hours_); }
    void setHours(double h) { if (h < 0 || h > 744) throw std::invalid_argument("Hours must be 0-744"); hours_ = h; }
private:
    Cents rate_;
    double hours_;
};

class CommissionEmployee : public Employee {
public:
    CommissionEmployee(int id, std::string n, std::string d, Cents base, Cents sales, double pct)
        : Employee(id, std::move(n), std::move(d)), base_(base), sales_(sales), pct_(pct) {
        if (base < 0 || sales < 0 || pct < 0 || pct > 100) throw std::invalid_argument("Invalid base, sales or commission %");
    }
    std::string type() const override { return "Commission"; }
    Cents grossPay() const override { return base_ + static_cast<Cents>(std::llround(sales_ * pct_ / 100.0)); }
    std::string payDetails() const override {
        std::ostringstream os;
        os << "base " << money(base_) << " + " << std::fixed << std::setprecision(1) << pct_ << "% of sales " << money(sales_);
        return os.str();
    }
    std::string serialize() const override { return "C|" + std::to_string(base_) + "|" + std::to_string(sales_) + "|" + std::to_string(pct_); }
    void setSales(Cents s) { if (s < 0) throw std::invalid_argument("Sales cannot be negative"); sales_ = s; }
private:
    Cents base_, sales_;
    double pct_;
};

// Factory from a data-file line: id|name|dept|TYPE|fields...
std::unique_ptr<Employee> fromLine(const std::string& line) {
    std::vector<std::string> f;
    std::stringstream ss(line);
    std::string part;
    while (std::getline(ss, part, '|')) f.push_back(part);
    if (f.size() < 5) throw std::runtime_error("too few fields");
    const int id = std::stoi(f[0]);
    if (f[3] == "S" && f.size() == 5) return std::make_unique<SalariedEmployee>(id, f[1], f[2], std::stoll(f[4]));
    if (f[3] == "H" && f.size() == 6) return std::make_unique<HourlyEmployee>(id, f[1], f[2], std::stoll(f[4]), std::stod(f[5]));
    if (f[3] == "C" && f.size() == 7) return std::make_unique<CommissionEmployee>(id, f[1], f[2], std::stoll(f[4]), std::stoll(f[5]), std::stod(f[6]));
    throw std::runtime_error("unknown type");
}

// ---------- payroll ----------

class Payroll {
public:
    void load() {
        std::ifstream in(kFile);
        if (!in) return;
        std::string line;
        int n = 0;
        while (std::getline(in, line)) {
            ++n;
            if (line.empty()) continue;
            try {
                auto e = fromLine(line);
                nextId_ = std::max(nextId_, e->id() + 1);
                staff_.push_back(std::move(e));
            } catch (const std::exception& ex) {
                std::cerr << "Warning: skipping line " << n << " (" << ex.what() << ")\n";
            }
        }
    }

    bool save() const {
        std::ofstream out(kFile, std::ios::trunc);
        if (!out) { std::cerr << "Error: cannot write " << kFile << "\n"; return false; }
        for (const auto& e : staff_) out << e->id() << '|' << e->name() << '|' << e->department() << '|' << e->serialize() << '\n';
        return out.good();
    }

    int nextId() { return nextId_++; }
    void add(std::unique_ptr<Employee> e) { staff_.push_back(std::move(e)); }

    Employee* find(int id) {
        for (auto& e : staff_) if (e->id() == id) return e.get();
        return nullptr;
    }

    bool remove(int id) {
        auto it = std::find_if(staff_.begin(), staff_.end(), [id](const auto& e) { return e->id() == id; });
        if (it == staff_.end()) return false;
        staff_.erase(it);
        return true;
    }

    void list() const {
        if (staff_.empty()) { std::cout << "No employees.\n"; return; }
        std::cout << std::left << std::setw(5) << "ID" << std::setw(22) << "Name" << std::setw(14) << "Dept" << std::setw(12) << "Type"
                  << std::right << std::setw(12) << "Gross" << std::setw(10) << "Tax" << std::setw(12) << "Net" << "\n" << std::string(87, '-') << "\n";
        Cents totalGross = 0, totalNet = 0;
        for (const auto& e : staff_) {
            std::cout << std::left << std::setw(5) << e->id() << std::setw(22) << e->name().substr(0, 20) << std::setw(14) << e->department().substr(0, 12)
                      << std::setw(12) << e->type() << std::right << std::setw(12) << money(e->grossPay()) << std::setw(10) << money(e->tax()) << std::setw(12) << money(e->netPay()) << "\n";
            totalGross += e->grossPay();
            totalNet += e->netPay();
        }
        std::cout << std::string(87, '-') << "\n" << std::left << std::setw(53) << "Totals" << std::right << std::setw(12) << money(totalGross) << std::setw(10) << money(totalGross - totalNet) << std::setw(12) << money(totalNet) << "\n";
    }

    void payslip(const Employee& e) const {
        std::cout << "\n------------ PAYSLIP ------------\n"
                  << "Employee : " << e.name() << " (#" << e.id() << ")\n"
                  << "Dept     : " << e.department() << "\n"
                  << "Type     : " << e.type() << "\n"
                  << "Basis    : " << e.payDetails() << "\n"
                  << "Gross    : " << std::setw(14) << money(e.grossPay()) << "\n"
                  << "Tax      : " << std::setw(14) << money(e.tax()) << "\n"
                  << "Net pay  : " << std::setw(14) << money(e.netPay()) << "\n"
                  << "---------------------------------\n";
    }

    void departmentReport() const {
        std::vector<std::pair<std::string, Cents>> totals;
        for (const auto& e : staff_) {
            auto it = std::find_if(totals.begin(), totals.end(), [&](const auto& p) { return p.first == e->department(); });
            if (it == totals.end()) totals.push_back({e->department(), e->grossPay()});
            else it->second += e->grossPay();
        }
        std::sort(totals.begin(), totals.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        std::cout << "Monthly gross by department:\n";
        for (const auto& [dept, total] : totals) std::cout << "  " << std::left << std::setw(16) << dept << std::right << std::setw(14) << money(total) << "\n";
        if (totals.empty()) std::cout << "  (none)\n";
    }

    void sortByNet() {
        std::sort(staff_.begin(), staff_.end(), [](const auto& a, const auto& b) { return a->netPay() > b->netPay(); });
    }

private:
    std::vector<std::unique_ptr<Employee>> staff_;
    int nextId_ = 1;
};

// ---------- input ----------

bool ask(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    const auto a = out.find_first_not_of(" \t\r\n");
    out = (a == std::string::npos) ? "" : out.substr(a, out.find_last_not_of(" \t\r\n") - a + 1);
    if (out.find('|') != std::string::npos) throw std::invalid_argument("The '|' character is not allowed");
    return true;
}

bool askDouble(const std::string& prompt, double lo, double hi, double& out) {
    std::string s;
    if (!ask(prompt, s)) return false;
    try {
        std::size_t used = 0;
        out = std::stod(s, &used);
        if (used != s.size() || out < lo || out > hi) throw std::invalid_argument("range");
    } catch (const std::exception&) {
        std::ostringstream os;
        os << "Enter a number between " << lo << " and " << hi;
        throw std::invalid_argument(os.str());
    }
    return true;
}

bool askInt(const std::string& prompt, int& out) {
    double d;
    if (!askDouble(prompt, 1, 1e7, d)) return false;
    out = static_cast<int>(d);
    return true;
}

}  // namespace

int main() {
    Payroll payroll;
    payroll.load();
    std::cout << "=== Employee Payroll System ===\n";

    std::string line, name, dept;
    while (true) {
        std::cout << "\n1. List payroll   2. Add employee   3. Payslip   4. Update hours/sales\n"
                     "5. Remove         6. Department report   7. Sort by net pay   0. Quit\nChoose: ";
        if (!std::getline(std::cin, line)) break;
        try {
            if (line == "0") break;
            if (line == "1") payroll.list();
            else if (line == "2") {
                std::string kind;
                if (!ask("Type (s=salaried, h=hourly, c=commission): ", kind) || !ask("Name: ", name) || !ask("Department: ", dept)) break;
                if (name.empty()) throw std::invalid_argument("Name is required");
                if (dept.empty()) dept = "General";
                const char k = kind.empty() ? '?' : static_cast<char>(std::tolower(static_cast<unsigned char>(kind[0])));
                const int id = payroll.nextId();
                double a, b, c;
                if (k == 's') {
                    if (!askDouble("Annual salary: ", 0, 1e8, a)) break;
                    payroll.add(std::make_unique<SalariedEmployee>(id, name, dept, toCents(a)));
                } else if (k == 'h') {
                    if (!askDouble("Hourly rate: ", 0, 1e5, a) || !askDouble("Hours this month: ", 0, 744, b)) break;
                    payroll.add(std::make_unique<HourlyEmployee>(id, name, dept, toCents(a), b));
                } else if (k == 'c') {
                    if (!askDouble("Monthly base: ", 0, 1e7, a) || !askDouble("Sales this month: ", 0, 1e9, b) || !askDouble("Commission %: ", 0, 100, c)) break;
                    payroll.add(std::make_unique<CommissionEmployee>(id, name, dept, toCents(a), toCents(b), c));
                } else throw std::invalid_argument("Unknown employee type");
                std::cout << "Added employee #" << id << ".\n";
                payroll.save();
            } else if (line == "3" || line == "4" || line == "5") {
                int id;
                if (!askInt("Employee ID: ", id)) break;
                Employee* e = payroll.find(id);
                if (!e) { std::cout << "No employee with ID " << id << ".\n"; continue; }
                if (line == "3") payroll.payslip(*e);
                else if (line == "5") { payroll.remove(id); payroll.save(); std::cout << "Removed.\n"; }
                else {
                    double v;
                    if (auto* h = dynamic_cast<HourlyEmployee*>(e)) {
                        if (!askDouble("Hours this month: ", 0, 744, v)) break;
                        h->setHours(v);
                    } else if (auto* c = dynamic_cast<CommissionEmployee*>(e)) {
                        if (!askDouble("Sales this month: ", 0, 1e9, v)) break;
                        c->setSales(toCents(v));
                    } else {
                        std::cout << "Salaried employees have no monthly variables.\n";
                        continue;
                    }
                    payroll.save();
                    std::cout << "Updated. New net pay: " << money(e->netPay()) << "\n";
                }
            } else if (line == "6") payroll.departmentReport();
            else if (line == "7") { payroll.sortByNet(); payroll.list(); payroll.save(); }
            else std::cout << "Unknown option.\n";
        } catch (const std::invalid_argument& e) {
            std::cout << "Invalid input: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    std::cout << (payroll.save() ? "Saved. Goodbye.\n" : "WARNING: save failed.\n");
    return 0;
}

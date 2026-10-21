// Parking Lot Management System (Intermediate C++)
//
// Tracks slots of different sizes, vehicle entry/exit, time-based fees and
// daily revenue, with persistence so a running lot survives restarts.
//
// Concepts: enums, structs/classes, std::map and std::vector, std::optional,
// <chrono> timestamps, file I/O, validation.
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

const char* const kStateFile = "parking_state.txt";
const char* const kLogFile = "parking_log.txt";

enum class VehicleType { Motorcycle, Car, Van };
enum class SlotSize { Small, Medium, Large };

const char* name(VehicleType t) {
    switch (t) { case VehicleType::Motorcycle: return "Motorcycle"; case VehicleType::Car: return "Car"; case VehicleType::Van: return "Van"; }
    return "?";
}
const char* name(SlotSize s) {
    switch (s) { case SlotSize::Small: return "Small"; case SlotSize::Medium: return "Medium"; case SlotSize::Large: return "Large"; }
    return "?";
}

// Hourly rates in cents by vehicle type; first 15 minutes free; max daily cap.
long long hourlyRate(VehicleType t) {
    switch (t) { case VehicleType::Motorcycle: return 100; case VehicleType::Car: return 250; case VehicleType::Van: return 400; }
    return 0;
}
constexpr long long kDailyCapCents = 2500;
constexpr long kGraceSeconds = 15 * 60;

std::string money(long long c) {
    std::ostringstream os;
    os << c / 100 << '.' << std::setw(2) << std::setfill('0') << c % 100;
    return os.str();
}

long long now() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

std::string formatTime(long long epoch) {
    const std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tm);
    return buf;
}

std::string formatDuration(long long seconds) {
    std::ostringstream os;
    os << seconds / 3600 << "h " << (seconds % 3600) / 60 << "m";
    return os.str();
}

// Fee: free within grace period; otherwise charged per started hour; capped per 24 h.
long long computeFee(VehicleType type, long long seconds) {
    if (seconds <= kGraceSeconds) return 0;
    const long long days = seconds / 86400;
    const long long remainder = seconds % 86400;
    const long long hours = (remainder + 3599) / 3600;
    const long long partial = std::min(hours * hourlyRate(type), kDailyCapCents);
    return days * kDailyCapCents + partial;
}

bool fits(VehicleType v, SlotSize s) {
    switch (v) {
        case VehicleType::Motorcycle: return true;
        case VehicleType::Car: return s != SlotSize::Small;
        case VehicleType::Van: return s == SlotSize::Large;
    }
    return false;
}

std::string normalisePlate(std::string p) {
    p.erase(std::remove_if(p.begin(), p.end(), [](unsigned char c) { return std::isspace(c) || c == '-'; }), p.end());
    std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return p;
}

bool validPlate(const std::string& p) {
    return p.size() >= 2 && p.size() <= 10 && std::all_of(p.begin(), p.end(), [](unsigned char c) { return std::isalnum(c); });
}

struct Ticket {
    int slotId;
    std::string plate;
    VehicleType type;
    long long entryTime;
};

struct Slot {
    int id;
    SlotSize size;
    std::optional<Ticket> ticket;  // empty when the slot is free
};

class ParkingLot {
public:
    ParkingLot() {
        int id = 1;
        for (int i = 0; i < 4; ++i) slots_.push_back({id++, SlotSize::Small, std::nullopt});
        for (int i = 0; i < 8; ++i) slots_.push_back({id++, SlotSize::Medium, std::nullopt});
        for (int i = 0; i < 3; ++i) slots_.push_back({id++, SlotSize::Large, std::nullopt});
    }

    // ----- persistence -----
    void load() {
        std::ifstream in(kStateFile);
        if (!in) return;
        std::string line;
        int n = 0;
        while (std::getline(in, line)) {
            ++n;
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string slot, plate, type, time;
            if (!std::getline(ss, slot, '|') || !std::getline(ss, plate, '|') || !std::getline(ss, type, '|') || !std::getline(ss, time)) {
                std::cerr << "Warning: skipping malformed line " << n << "\n";
                continue;
            }
            try {
                const int slotId = std::stoi(slot);
                const int t = std::stoi(type);
                const long long entry = std::stoll(time);
                Slot* s = findSlot(slotId);
                if (!s || s->ticket || t < 0 || t > 2 || !validPlate(plate)) throw std::runtime_error("bad");
                s->ticket = Ticket{slotId, plate, static_cast<VehicleType>(t), entry};
                plates_[plate] = slotId;
            } catch (const std::exception&) {
                std::cerr << "Warning: skipping invalid record on line " << n << "\n";
            }
        }
    }

    bool save() const {
        std::ofstream out(kStateFile, std::ios::trunc);
        if (!out) { std::cerr << "Error: cannot write " << kStateFile << "\n"; return false; }
        for (const Slot& s : slots_)
            if (s.ticket) out << s.id << '|' << s.ticket->plate << '|' << static_cast<int>(s.ticket->type) << '|' << s.ticket->entryTime << '\n';
        return out.good();
    }

    // ----- operations -----
    std::optional<int> park(const std::string& plate, VehicleType type, std::string& error) {
        if (plates_.count(plate)) { error = "Vehicle " + plate + " is already parked in slot " + std::to_string(plates_[plate]) + "."; return std::nullopt; }
        // Prefer the smallest slot the vehicle fits in to keep big slots free.
        for (SlotSize size : {SlotSize::Small, SlotSize::Medium, SlotSize::Large}) {
            if (!fits(type, size)) continue;
            for (Slot& s : slots_) {
                if (s.size == size && !s.ticket) {
                    s.ticket = Ticket{s.id, plate, type, now()};
                    plates_[plate] = s.id;
                    appendLog("ENTRY", *s.ticket, 0, 0);
                    return s.id;
                }
            }
        }
        error = std::string("No free slot suitable for a ") + name(type) + ".";
        return std::nullopt;
    }

    struct Receipt { Ticket ticket; long long seconds; long long fee; };

    std::optional<Receipt> leave(const std::string& plate, std::string& error) {
        auto it = plates_.find(plate);
        if (it == plates_.end()) { error = "No parked vehicle with plate " + plate + "."; return std::nullopt; }
        Slot* s = findSlot(it->second);
        if (!s || !s->ticket) { error = "Internal inconsistency; slot is empty."; plates_.erase(it); return std::nullopt; }
        Receipt r{*s->ticket, std::max(0LL, now() - s->ticket->entryTime), 0};
        r.fee = computeFee(r.ticket.type, r.seconds);
        appendLog("EXIT", r.ticket, r.seconds, r.fee);
        revenueToday_ += r.fee;
        s->ticket.reset();
        plates_.erase(it);
        return r;
    }

    void status() const {
        int freeCount = 0;
        std::cout << std::left << std::setw(6) << "Slot" << std::setw(8) << "Size" << std::setw(12) << "Plate" << std::setw(12) << "Vehicle" << "Since\n";
        for (const Slot& s : slots_) {
            std::cout << std::left << std::setw(6) << s.id << std::setw(8) << name(s.size);
            if (s.ticket) std::cout << std::setw(12) << s.ticket->plate << std::setw(12) << name(s.ticket->type) << formatTime(s.ticket->entryTime) << " (" << formatDuration(now() - s.ticket->entryTime) << ")";
            else { std::cout << "-- free --"; ++freeCount; }
            std::cout << "\n";
        }
        std::cout << freeCount << " of " << slots_.size() << " slots free. Revenue this session: " << money(revenueToday_) << "\n";
    }

    void find(const std::string& plate) const {
        auto it = plates_.find(plate);
        if (it == plates_.end()) { std::cout << plate << " is not in the lot.\n"; return; }
        const Slot* s = const_cast<ParkingLot*>(this)->findSlot(it->second);
        std::cout << plate << " is in slot " << it->second << " since " << formatTime(s->ticket->entryTime)
                  << "; current fee would be " << money(computeFee(s->ticket->type, now() - s->ticket->entryTime)) << "\n";
    }

    void showRates() const {
        std::cout << "Rates per started hour: Motorcycle " << money(hourlyRate(VehicleType::Motorcycle)) << ", Car " << money(hourlyRate(VehicleType::Car))
                  << ", Van " << money(hourlyRate(VehicleType::Van)) << ". First 15 minutes free. Daily cap " << money(kDailyCapCents) << ".\n"
                  << "Slot sizes: Small (motorcycles), Medium (cars + motorcycles), Large (any vehicle).\n";
    }

private:
    std::vector<Slot> slots_;
    std::map<std::string, int> plates_;
    long long revenueToday_ = 0;

    Slot* findSlot(int id) {
        for (Slot& s : slots_) if (s.id == id) return &s;
        return nullptr;
    }

    static void appendLog(const char* event, const Ticket& t, long long seconds, long long fee) {
        std::ofstream out(kLogFile, std::ios::app);
        if (!out) return;
        out << formatTime(now()) << '|' << event << '|' << t.plate << '|' << name(t.type) << '|' << t.slotId << '|' << seconds << '|' << fee << '\n';
    }
};

bool ask(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    const auto a = out.find_first_not_of(" \t\r\n");
    out = (a == std::string::npos) ? "" : out.substr(a, out.find_last_not_of(" \t\r\n") - a + 1);
    return true;
}

std::optional<VehicleType> askType() {
    std::string s;
    while (ask("Vehicle type (m=motorcycle, c=car, v=van): ", s)) {
        if (s.empty()) continue;
        switch (std::tolower(static_cast<unsigned char>(s[0]))) {
            case 'm': return VehicleType::Motorcycle;
            case 'c': return VehicleType::Car;
            case 'v': return VehicleType::Van;
        }
        std::cout << "Please enter m, c or v.\n";
    }
    return std::nullopt;
}

}  // namespace

int main() {
    ParkingLot lot;
    lot.load();
    std::cout << "=== Parking Lot Management ===\n";
    lot.showRates();

    std::string choice, plate;
    while (true) {
        std::cout << "\n1. Vehicle entry   2. Vehicle exit   3. Lot status   4. Find vehicle   5. Rates   0. Quit\nChoose: ";
        if (!std::getline(std::cin, choice)) break;
        if (choice == "0") break;
        if (choice == "3") { lot.status(); continue; }
        if (choice == "5") { lot.showRates(); continue; }
        if (choice != "1" && choice != "2" && choice != "4") { std::cout << "Unknown option.\n"; continue; }

        if (!ask("Licence plate: ", plate)) break;
        plate = normalisePlate(plate);
        if (!validPlate(plate)) { std::cout << "Plate must be 2-10 letters/digits.\n"; continue; }

        std::string err;
        if (choice == "1") {
            auto type = askType();
            if (!type) break;
            if (auto slot = lot.park(plate, *type, err)) {
                std::cout << "Ticket issued: " << plate << " -> slot " << *slot << " at " << formatTime(now()) << ".\n";
                lot.save();
            } else std::cout << err << "\n";
        } else if (choice == "2") {
            if (auto r = lot.leave(plate, err)) {
                std::cout << "Exit: " << plate << " (" << name(r->ticket.type) << ") from slot " << r->ticket.slotId << "\n"
                          << "Parked " << formatDuration(r->seconds) << ", fee due: " << money(r->fee) << "\n";
                lot.save();
            } else std::cout << err << "\n";
        } else {
            lot.find(plate);
        }
    }
    std::cout << (lot.save() ? "State saved. Goodbye.\n" : "WARNING: state could not be saved.\n");
    return 0;
}

// Task Manager App (Beginner C++)
//
// Organise tasks with priorities, due dates and completion state. Tasks are
// persisted to a simple text file so they survive between runs.
//
// Concepts: classes, enums, std::vector, sorting with lambdas, file streams,
// string parsing and validation.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char* const kDataFile = "tasks.txt";

enum class Priority { Low = 1, Medium = 2, High = 3 };

std::string toString(Priority p) {
    switch (p) {
        case Priority::Low: return "Low";
        case Priority::Medium: return "Medium";
        case Priority::High: return "High";
    }
    return "?";
}

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// Validates YYYY-MM-DD (basic calendar checks, including leap years).
bool isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (std::size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u}) {
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    const int y = std::stoi(date.substr(0, 4));
    const int m = std::stoi(date.substr(5, 2));
    const int d = std::stoi(date.substr(8, 2));
    if (m < 1 || m > 12 || d < 1) return false;
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max = days[m - 1];
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    if (m == 2 && leap) max = 29;
    return d <= max;
}

struct Task {
    int id = 0;
    std::string title;
    Priority priority = Priority::Medium;
    std::string dueDate;  // YYYY-MM-DD or empty
    bool done = false;
};

class TaskManager {
public:
    bool load() {
        std::ifstream in(kDataFile);
        if (!in) return true;  // first run: nothing to load
        std::string line;
        int lineNo = 0;
        while (std::getline(in, line)) {
            ++lineNo;
            if (line.empty() || line[0] == '#') continue;
            Task t;
            if (!parseLine(line, t)) {
                std::cerr << "Warning: skipping malformed line " << lineNo << " in " << kDataFile << "\n";
                continue;
            }
            nextId_ = std::max(nextId_, t.id + 1);
            tasks_.push_back(t);
        }
        return true;
    }

    bool save() const {
        std::ofstream out(kDataFile, std::ios::trunc);
        if (!out) {
            std::cerr << "Error: cannot write " << kDataFile << "\n";
            return false;
        }
        out << "# id|done|priority|due|title\n";
        for (const Task& t : tasks_) {
            out << t.id << '|' << (t.done ? 1 : 0) << '|' << static_cast<int>(t.priority) << '|' << t.dueDate << '|' << t.title << '\n';
        }
        if (!out) {
            std::cerr << "Error: write failed for " << kDataFile << "\n";
            return false;
        }
        return true;
    }

    void add(const std::string& title, Priority p, const std::string& due) {
        Task t;
        t.id = nextId_++;
        t.title = title;
        t.priority = p;
        t.dueDate = due;
        tasks_.push_back(t);
        std::cout << "Added task #" << t.id << ".\n";
    }

    bool setDone(int id, bool done) {
        Task* t = find(id);
        if (!t) return false;
        t->done = done;
        return true;
    }

    bool remove(int id) {
        const auto it = std::find_if(tasks_.begin(), tasks_.end(), [id](const Task& t) { return t.id == id; });
        if (it == tasks_.end()) return false;
        tasks_.erase(it);
        return true;
    }

    bool edit(int id, const std::string& title, Priority p, const std::string& due) {
        Task* t = find(id);
        if (!t) return false;
        if (!title.empty()) t->title = title;
        t->priority = p;
        t->dueDate = due;
        return true;
    }

    void sortBy(const std::string& key) {
        if (key == "priority") {
            std::stable_sort(tasks_.begin(), tasks_.end(), [](const Task& a, const Task& b) {
                return static_cast<int>(a.priority) > static_cast<int>(b.priority);
            });
        } else if (key == "due") {
            std::stable_sort(tasks_.begin(), tasks_.end(), [](const Task& a, const Task& b) {
                if (a.dueDate.empty() != b.dueDate.empty()) return !a.dueDate.empty();  // dated first
                return a.dueDate < b.dueDate;  // ISO dates sort lexicographically
            });
        } else if (key == "status") {
            std::stable_sort(tasks_.begin(), tasks_.end(), [](const Task& a, const Task& b) { return !a.done && b.done; });
        } else {
            std::stable_sort(tasks_.begin(), tasks_.end(), [](const Task& a, const Task& b) { return a.id < b.id; });
        }
    }

    void list(bool pendingOnly) const {
        std::size_t shown = 0;
        std::cout << std::left << std::setw(4) << "ID" << std::setw(6) << "Done" << std::setw(8) << "Prio"
                  << std::setw(12) << "Due" << "Title\n";
        std::cout << std::string(60, '-') << "\n";
        for (const Task& t : tasks_) {
            if (pendingOnly && t.done) continue;
            std::cout << std::left << std::setw(4) << t.id << std::setw(6) << (t.done ? "[x]" : "[ ]") << std::setw(8)
                      << toString(t.priority) << std::setw(12) << (t.dueDate.empty() ? "-" : t.dueDate) << t.title << "\n";
            ++shown;
        }
        if (shown == 0) std::cout << (pendingOnly ? "No pending tasks." : "No tasks yet.") << "\n";
        const auto pending = std::count_if(tasks_.begin(), tasks_.end(), [](const Task& t) { return !t.done; });
        std::cout << tasks_.size() << " task(s), " << pending << " pending.\n";
    }

private:
    std::vector<Task> tasks_;
    int nextId_ = 1;

    Task* find(int id) {
        for (Task& t : tasks_)
            if (t.id == id) return &t;
        return nullptr;
    }

    static bool parseLine(const std::string& line, Task& t) {
        std::vector<std::string> parts;
        std::string field;
        std::stringstream ss(line);
        // split into 5 parts, title may contain '|' so stop splitting after 4 separators
        for (int i = 0; i < 4; ++i) {
            if (!std::getline(ss, field, '|')) return false;
            parts.push_back(field);
        }
        std::getline(ss, field);
        parts.push_back(field);
        try {
            t.id = std::stoi(parts[0]);
            t.done = std::stoi(parts[1]) != 0;
            const int p = std::stoi(parts[2]);
            if (p < 1 || p > 3 || t.id <= 0) return false;
            t.priority = static_cast<Priority>(p);
        } catch (const std::exception&) {
            return false;
        }
        t.dueDate = parts[3];
        if (!t.dueDate.empty() && !isValidDate(t.dueDate)) return false;
        t.title = trim(parts[4]);
        return !t.title.empty();
    }
};

// ---------- input helpers ----------

bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    out = trim(out);
    return true;
}

int readInt(const std::string& prompt, int lo, int hi, int fallback = -1) {
    std::string line;
    while (readLine(prompt, line)) {
        if (line.empty() && fallback != -1) return fallback;
        try {
            std::size_t used = 0;
            const int v = std::stoi(line, &used);
            if (used == line.size() && v >= lo && v <= hi) return v;
        } catch (const std::exception&) {
        }
        std::cout << "Enter a number between " << lo << " and " << hi << ".\n";
    }
    return -1;  // EOF
}

std::string readDate(const std::string& prompt) {
    std::string line;
    while (readLine(prompt, line)) {
        if (line.empty() || isValidDate(line)) return line;
        std::cout << "Use the format YYYY-MM-DD or leave empty.\n";
    }
    return "";
}

void showMenu() {
    std::cout << "\n=== Task Manager ===\n"
                 "1. List all tasks\n"
                 "2. List pending tasks\n"
                 "3. Add task\n"
                 "4. Mark task done\n"
                 "5. Mark task not done\n"
                 "6. Edit task\n"
                 "7. Delete task\n"
                 "8. Sort tasks (priority / due / status / id)\n"
                 "9. Save\n"
                 "0. Quit\n";
}

}  // namespace

int main() {
    TaskManager manager;
    manager.load();
    bool dirty = false;

    while (true) {
        showMenu();
        const int choice = readInt("Choose: ", 0, 9);
        if (choice <= 0) break;

        std::string text;
        switch (choice) {
            case 1: manager.list(false); break;
            case 2: manager.list(true); break;
            case 3: {
                if (!readLine("Title: ", text) || text.empty()) { std::cout << "Title is required.\n"; break; }
                const int p = readInt("Priority (1=Low, 2=Medium, 3=High) [2]: ", 1, 3, 2);
                if (p < 0) break;
                manager.add(text, static_cast<Priority>(p), readDate("Due date YYYY-MM-DD (optional): "));
                dirty = true;
                break;
            }
            case 4:
            case 5: {
                const int id = readInt("Task ID: ", 1, 1000000);
                if (id < 0) break;
                if (manager.setDone(id, choice == 4)) { std::cout << "Updated.\n"; dirty = true; }
                else std::cout << "No task with ID " << id << ".\n";
                break;
            }
            case 6: {
                const int id = readInt("Task ID: ", 1, 1000000);
                if (id < 0) break;
                if (!readLine("New title (empty to keep): ", text)) break;
                const int p = readInt("Priority (1-3): ", 1, 3);
                if (p < 0) break;
                if (manager.edit(id, text, static_cast<Priority>(p), readDate("Due date (optional): "))) { std::cout << "Updated.\n"; dirty = true; }
                else std::cout << "No task with ID " << id << ".\n";
                break;
            }
            case 7: {
                const int id = readInt("Task ID to delete: ", 1, 1000000);
                if (id < 0) break;
                if (manager.remove(id)) { std::cout << "Deleted.\n"; dirty = true; }
                else std::cout << "No task with ID " << id << ".\n";
                break;
            }
            case 8:
                if (!readLine("Sort key (priority/due/status/id): ", text)) break;
                manager.sortBy(text);
                manager.list(false);
                dirty = true;
                break;
            case 9:
                if (manager.save()) { std::cout << "Saved to " << kDataFile << ".\n"; dirty = false; }
                break;
        }
    }

    if (dirty) {
        std::cout << "Saving changes... " << (manager.save() ? "done." : "FAILED.") << "\n";
    }
    std::cout << "Bye.\n";
    return 0;
}

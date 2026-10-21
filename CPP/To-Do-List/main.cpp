// To-Do List (Beginner C++)
//
// A compact command-driven to-do list: add, list, done, undo, remove, clear,
// with automatic persistence to todo.txt. Demonstrates std::vector,
// std::string parsing, file streams and simple functions.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char* const kFile = "todo.txt";

struct Item {
    std::string text;
    bool done = false;
};

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    const auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<Item> load() {
    std::vector<Item> items;
    std::ifstream in(kFile);
    if (!in) return items;  // no file yet is fine
    std::string line;
    while (std::getline(in, line)) {
        if (line.size() < 4 || line[0] != '[' || line[2] != ']') continue;  // malformed, skip
        Item it;
        it.done = (line[1] == 'x' || line[1] == 'X');
        it.text = trim(line.substr(3));
        if (!it.text.empty()) items.push_back(it);
    }
    return items;
}

bool save(const std::vector<Item>& items) {
    std::ofstream out(kFile, std::ios::trunc);
    if (!out) {
        std::cerr << "Error: could not open " << kFile << " for writing.\n";
        return false;
    }
    for (const Item& it : items) out << (it.done ? "[x] " : "[ ] ") << it.text << '\n';
    if (!out) {
        std::cerr << "Error: writing " << kFile << " failed.\n";
        return false;
    }
    return true;
}

void list(const std::vector<Item>& items) {
    if (items.empty()) {
        std::cout << "Nothing to do. Add something with: add <text>\n";
        return;
    }
    std::size_t pending = 0;
    for (std::size_t i = 0; i < items.size(); ++i) {
        std::cout << (i + 1) << ". " << (items[i].done ? "[x] " : "[ ] ") << items[i].text << '\n';
        if (!items[i].done) ++pending;
    }
    std::cout << items.size() << " item(s), " << pending << " pending.\n";
}

// Parses a 1-based index from text; returns false if invalid.
bool parseIndex(const std::string& arg, std::size_t size, std::size_t& index) {
    if (arg.empty()) return false;
    try {
        std::size_t used = 0;
        const long v = std::stol(arg, &used);
        if (used != arg.size() || v < 1 || static_cast<std::size_t>(v) > size) return false;
        index = static_cast<std::size_t>(v - 1);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void help() {
    std::cout << "Commands:\n"
                 "  add <text>      add a new item\n"
                 "  list            show all items\n"
                 "  done <n>        mark item n as completed\n"
                 "  undo <n>        mark item n as not completed\n"
                 "  edit <n> <text> replace the text of item n\n"
                 "  remove <n>      delete item n\n"
                 "  clear           remove all completed items\n"
                 "  help            show this help\n"
                 "  quit            save and exit\n";
}

}  // namespace

int main() {
    std::vector<Item> items = load();
    std::cout << "To-Do List (" << items.size() << " item(s) loaded from " << kFile << "). Type 'help' for commands.\n";

    std::string line;
    while (true) {
        std::cout << "todo> ";
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }
        std::istringstream ss(line);
        std::string cmd, arg, rest;
        ss >> cmd;
        if (cmd.empty()) continue;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (cmd == "quit" || cmd == "exit") break;
        if (cmd == "help") { help(); continue; }
        if (cmd == "list" || cmd == "ls") { list(items); continue; }

        if (cmd == "add") {
            std::getline(ss, rest);
            rest = trim(rest);
            if (rest.empty()) { std::cout << "Usage: add <text>\n"; continue; }
            items.push_back({rest, false});
            std::cout << "Added item " << items.size() << ".\n";
        } else if (cmd == "done" || cmd == "undo" || cmd == "remove" || cmd == "rm" || cmd == "edit") {
            ss >> arg;
            std::size_t idx = 0;
            if (!parseIndex(arg, items.size(), idx)) {
                std::cout << "Please give a valid item number (1-" << items.size() << ").\n";
                continue;
            }
            if (cmd == "done") { items[idx].done = true; std::cout << "Completed: " << items[idx].text << '\n'; }
            else if (cmd == "undo") { items[idx].done = false; std::cout << "Reopened: " << items[idx].text << '\n'; }
            else if (cmd == "edit") {
                std::getline(ss, rest);
                rest = trim(rest);
                if (rest.empty()) { std::cout << "Usage: edit <n> <new text>\n"; continue; }
                items[idx].text = rest;
                std::cout << "Updated item " << idx + 1 << ".\n";
            } else {
                std::cout << "Removed: " << items[idx].text << '\n';
                items.erase(items.begin() + static_cast<std::ptrdiff_t>(idx));
            }
        } else if (cmd == "clear") {
            const auto before = items.size();
            items.erase(std::remove_if(items.begin(), items.end(), [](const Item& i) { return i.done; }), items.end());
            std::cout << "Removed " << before - items.size() << " completed item(s).\n";
        } else {
            std::cout << "Unknown command '" << cmd << "'. Type 'help'.\n";
            continue;
        }
        save(items);  // persist after every change so nothing is lost
    }

    if (save(items)) std::cout << "Saved " << items.size() << " item(s) to " << kFile << ". Bye!\n";
    return 0;
}

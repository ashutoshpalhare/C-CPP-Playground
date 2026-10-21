// Password Generator (Beginner C++)
//
// Generates random passwords from configurable character sets, guarantees
// at least one character from each enabled set, estimates entropy, and can
// also produce memorable word-based passphrases.
//
// Uses std::random_device + std::mt19937 and std::uniform_int_distribution
// (never rand() % n). Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

struct Options {
    int length = 16;
    int count = 1;
    bool lower = true;
    bool upper = true;
    bool digits = true;
    bool symbols = true;
    bool excludeAmbiguous = false;  // drop 0 O o 1 l I |
};

const std::string kLower = "abcdefghijklmnopqrstuvwxyz";
const std::string kUpper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string kDigits = "0123456789";
const std::string kSymbols = "!@#$%^&*()-_=+[]{};:,.<>?/";
const std::string kAmbiguous = "0Oo1lI|";

const std::vector<std::string> kWords = {
    "apple", "river", "stone", "cloud", "tiger", "piano", "lemon", "maple", "ocean", "robot",
    "candle", "forest", "garden", "meadow", "pepper", "silver", "velvet", "window", "yellow", "zebra",
    "anchor", "breeze", "copper", "dragon", "ember", "falcon", "glacier", "harbor", "island", "jungle",
    "kettle", "lantern", "marble", "nectar", "orbit", "planet", "quartz", "rocket", "saddle", "timber"};

std::string removeAmbiguous(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), [](char c) { return kAmbiguous.find(c) != std::string::npos; }), s.end());
    return s;
}

class Generator {
public:
    Generator() : rng_(std::random_device{}()) {}

    std::string password(const Options& opt) {
        std::vector<std::string> sets;
        if (opt.lower) sets.push_back(opt.excludeAmbiguous ? removeAmbiguous(kLower) : kLower);
        if (opt.upper) sets.push_back(opt.excludeAmbiguous ? removeAmbiguous(kUpper) : kUpper);
        if (opt.digits) sets.push_back(opt.excludeAmbiguous ? removeAmbiguous(kDigits) : kDigits);
        if (opt.symbols) sets.push_back(opt.excludeAmbiguous ? removeAmbiguous(kSymbols) : kSymbols);
        if (sets.empty()) return "";

        std::string pool;
        for (const auto& s : sets) pool += s;

        std::string out;
        // Guarantee one character from each enabled set.
        for (const auto& s : sets) out += pick(s);
        while (static_cast<int>(out.size()) < opt.length) out += pick(pool);
        std::shuffle(out.begin(), out.end(), rng_);
        return out;
    }

    std::string passphrase(int words, char separator, bool capitalise, bool addNumber) {
        std::string out;
        for (int i = 0; i < words; ++i) {
            std::string w = kWords[index(kWords.size())];
            if (capitalise) w[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(w[0])));
            if (!out.empty()) out += separator;
            out += w;
        }
        if (addNumber) out += separator + std::to_string(index(100));
        return out;
    }

    static double entropyBits(const Options& opt) {
        std::size_t pool = 0;
        if (opt.lower) pool += (opt.excludeAmbiguous ? removeAmbiguous(kLower) : kLower).size();
        if (opt.upper) pool += (opt.excludeAmbiguous ? removeAmbiguous(kUpper) : kUpper).size();
        if (opt.digits) pool += (opt.excludeAmbiguous ? removeAmbiguous(kDigits) : kDigits).size();
        if (opt.symbols) pool += (opt.excludeAmbiguous ? removeAmbiguous(kSymbols) : kSymbols).size();
        return pool ? opt.length * std::log2(static_cast<double>(pool)) : 0.0;
    }

    static double passphraseEntropyBits(int words, bool addNumber) {
        return words * std::log2(static_cast<double>(kWords.size())) + (addNumber ? std::log2(100.0) : 0.0);
    }

private:
    std::mt19937 rng_;

    std::size_t index(std::size_t n) {
        std::uniform_int_distribution<std::size_t> dist(0, n - 1);
        return dist(rng_);
    }
    char pick(const std::string& s) { return s[index(s.size())]; }
};

const char* strengthLabel(double bits) {
    if (bits < 40) return "weak";
    if (bits < 60) return "fair";
    if (bits < 80) return "strong";
    return "very strong";
}

// ---------- input ----------

bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) return false;
    const auto a = out.find_first_not_of(" \t\r\n");
    out = (a == std::string::npos) ? "" : out.substr(a, out.find_last_not_of(" \t\r\n") - a + 1);
    return true;
}

// Returns -1 on EOF.
int readInt(const std::string& prompt, int lo, int hi, int fallback) {
    std::string line;
    while (readLine(prompt + " [" + std::to_string(fallback) + "]: ", line)) {
        if (line.empty()) return fallback;
        try {
            std::size_t used = 0;
            const int v = std::stoi(line, &used);
            if (used == line.size() && v >= lo && v <= hi) return v;
        } catch (const std::exception&) {
        }
        std::cout << "Enter a number from " << lo << " to " << hi << ".\n";
    }
    return -1;
}

// Returns 1 yes, 0 no, -1 EOF.
int readYesNo(const std::string& prompt, bool fallback) {
    std::string line;
    while (readLine(prompt + (fallback ? " [Y/n]: " : " [y/N]: "), line)) {
        if (line.empty()) return fallback ? 1 : 0;
        const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        if (c == 'y') return 1;
        if (c == 'n') return 0;
        std::cout << "Please answer y or n.\n";
    }
    return -1;
}

}  // namespace

int main() {
    Generator gen;
    std::cout << "=== Password Generator ===\n";

    while (true) {
        std::cout << "\n1. Random password\n2. Memorable passphrase\n0. Quit\n";
        const int mode = readInt("Choose", 0, 2, 1);
        if (mode <= 0) break;

        if (mode == 1) {
            Options opt;
            opt.length = readInt("Length (4-128)", 4, 128, opt.length);
            if (opt.length < 0) break;
            int r;
            if ((r = readYesNo("Include lowercase letters?", true)) < 0) break; opt.lower = r;
            if ((r = readYesNo("Include uppercase letters?", true)) < 0) break; opt.upper = r;
            if ((r = readYesNo("Include digits?", true)) < 0) break; opt.digits = r;
            if ((r = readYesNo("Include symbols?", true)) < 0) break; opt.symbols = r;
            if ((r = readYesNo("Exclude ambiguous characters (0 O 1 l I |)?", false)) < 0) break; opt.excludeAmbiguous = r;
            opt.count = readInt("How many passwords?", 1, 50, 1);
            if (opt.count < 0) break;

            if (!opt.lower && !opt.upper && !opt.digits && !opt.symbols) {
                std::cout << "At least one character set must be enabled.\n";
                continue;
            }
            const int enabledSets = opt.lower + opt.upper + opt.digits + opt.symbols;
            if (opt.length < enabledSets) {
                std::cout << "Length must be at least " << enabledSets << " to include one of each selected set.\n";
                continue;
            }
            const double bits = Generator::entropyBits(opt);
            std::cout << "\nEntropy: ~" << static_cast<int>(bits) << " bits (" << strengthLabel(bits) << ")\n";
            for (int i = 0; i < opt.count; ++i) std::cout << "  " << gen.password(opt) << "\n";
        } else {
            const int words = readInt("Number of words (3-10)", 3, 10, 4);
            if (words < 0) break;
            std::string sep;
            if (!readLine("Separator character [-]: ", sep)) break;
            const char separator = sep.empty() ? '-' : sep[0];
            int cap, num;
            if ((cap = readYesNo("Capitalise words?", true)) < 0) break;
            if ((num = readYesNo("Append a number?", true)) < 0) break;
            const double bits = Generator::passphraseEntropyBits(words, num != 0);
            std::cout << "\nEntropy: ~" << static_cast<int>(bits) << " bits (" << strengthLabel(bits) << ")\n";
            std::cout << "  " << gen.passphrase(words, separator, cap != 0, num != 0) << "\n";
        }
    }
    std::cout << "\nGoodbye.\n";
    return 0;
}

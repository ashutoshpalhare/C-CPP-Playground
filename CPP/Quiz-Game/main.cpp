// Quiz Game (Beginner C++)
//
// A terminal quiz with several categories, shuffled questions, immediate
// feedback, a final summary, and replay support. Demonstrates classes,
// std::vector, <random>, and basic input validation.
//
// Standard: C++17. Portable to Windows, Linux and macOS.

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

struct Question {
    std::string text;
    std::vector<std::string> options;  // exactly 4
    int correct;                       // index 0-3
    std::string explanation;
};

class QuestionBank {
public:
    QuestionBank() { populate(); }

    std::vector<std::string> categories() const {
        std::vector<std::string> names;
        for (const auto& c : bank_) names.push_back(c.first);
        return names;
    }

    const std::vector<Question>& questionsFor(std::size_t index) const { return bank_.at(index).second; }

private:
    std::vector<std::pair<std::string, std::vector<Question>>> bank_;

    void populate() {
        bank_.push_back({"C++ Basics", {
            {"Which header provides std::cout?", {"<stdio.h>", "<iostream>", "<stream>", "<cstdout>"}, 1, "std::cout lives in <iostream>."},
            {"What does 'auto' do in C++11 and later?", {"Allocates memory automatically", "Deduces a variable's type", "Makes a variable global", "Declares a constant"}, 1, "auto lets the compiler deduce the type from the initialiser."},
            {"Which container stores elements in contiguous memory?", {"std::list", "std::map", "std::vector", "std::set"}, 2, "std::vector is a dynamic array with contiguous storage."},
            {"What is the size of a bool guaranteed to be?", {"1 bit", "1 byte", "4 bytes", "Implementation-defined"}, 3, "The standard only guarantees sizeof(bool) >= 1; most compilers use 1 byte."},
            {"Which keyword prevents a class from being derived from?", {"sealed", "static", "final", "const"}, 2, "'final' (C++11) stops further inheritance or overriding."},
        }});
        bank_.push_back({"General Knowledge", {
            {"What is the largest planet in our solar system?", {"Earth", "Saturn", "Jupiter", "Neptune"}, 2, "Jupiter is more than twice the mass of all other planets combined."},
            {"How many continents are there?", {"5", "6", "7", "8"}, 2, "Seven: Africa, Antarctica, Asia, Australia, Europe, North and South America."},
            {"Which gas do plants absorb from the air?", {"Oxygen", "Nitrogen", "Carbon dioxide", "Hydrogen"}, 2, "Photosynthesis uses CO2 and water to create sugar and oxygen."},
            {"Who painted the Mona Lisa?", {"Michelangelo", "Leonardo da Vinci", "Raphael", "Donatello"}, 1, "Leonardo painted it in the early 16th century."},
            {"What is the boiling point of water at sea level (Celsius)?", {"90", "100", "110", "120"}, 1, "100 °C at standard atmospheric pressure."},
        }});
        bank_.push_back({"Mathematics", {
            {"What is 12 x 12?", {"124", "144", "132", "148"}, 1, "12 squared is 144."},
            {"What is the square root of 81?", {"7", "8", "9", "10"}, 2, "9 x 9 = 81."},
            {"Which number is prime?", {"21", "27", "29", "33"}, 2, "29 has no divisors other than 1 and itself."},
            {"What is 15% of 200?", {"20", "25", "30", "35"}, 2, "0.15 x 200 = 30."},
            {"How many degrees are in a triangle's interior angles?", {"90", "180", "270", "360"}, 1, "Always 180 degrees in Euclidean geometry."},
        }});
    }
};

class Quiz {
public:
    Quiz(std::vector<Question> questions, std::mt19937& rng) : questions_(std::move(questions)), rng_(rng) {
        std::shuffle(questions_.begin(), questions_.end(), rng_);
    }

    void play() {
        score_ = 0;
        missed_.clear();
        for (std::size_t i = 0; i < questions_.size(); ++i) {
            ask(i);
        }
        summary();
    }

private:
    std::vector<Question> questions_;
    std::mt19937& rng_;
    int score_ = 0;
    std::vector<const Question*> missed_;

    void ask(std::size_t index) {
        const Question& q = questions_[index];
        std::cout << "\nQuestion " << index + 1 << " of " << questions_.size() << ": " << q.text << "\n";
        for (std::size_t i = 0; i < q.options.size(); ++i) {
            std::cout << "  " << static_cast<char>('A' + i) << ") " << q.options[i] << "\n";
        }
        int answer = readAnswer();
        if (answer < 0) {
            std::cout << "(input closed - counting as skipped)\n";
            missed_.push_back(&q);
            return;
        }
        if (answer == q.correct) {
            std::cout << "Correct!\n";
            ++score_;
        } else {
            std::cout << "Wrong. The answer was " << static_cast<char>('A' + q.correct) << ") " << q.options[q.correct] << ".\n";
            missed_.push_back(&q);
        }
    }

    // Returns 0-3, or -1 if input is closed.
    int readAnswer() {
        while (true) {
            std::cout << "Your answer (A-D): ";
            std::string line;
            if (!std::getline(std::cin, line)) return -1;
            line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); }), line.end());
            if (line.size() == 1) {
                char c = static_cast<char>(std::toupper(static_cast<unsigned char>(line[0])));
                if (c >= 'A' && c <= 'D') return c - 'A';
            }
            std::cout << "Please type a single letter A, B, C or D.\n";
        }
    }

    void summary() const {
        const double pct = questions_.empty() ? 0.0 : 100.0 * score_ / static_cast<double>(questions_.size());
        std::cout << "\n=== Results ===\n";
        std::cout << "Score: " << score_ << "/" << questions_.size() << " (" << static_cast<int>(pct + 0.5) << "%)\n";
        if (pct == 100.0) std::cout << "Perfect score!\n";
        else if (pct >= 80.0) std::cout << "Great job!\n";
        else if (pct >= 50.0) std::cout << "Not bad - keep practising.\n";
        else std::cout << "Review the explanations below and try again.\n";
        if (!missed_.empty()) {
            std::cout << "\nReview:\n";
            for (const Question* q : missed_) {
                std::cout << "- " << q->text << "\n    Answer: " << q->options[static_cast<std::size_t>(q->correct)]
                          << "\n    Why: " << q->explanation << "\n";
            }
        }
    }
};

// Reads an integer in [lo, hi]; returns -1 if input closed.
int readChoice(int lo, int hi) {
    while (true) {
        std::cout << "Choice (" << lo << "-" << hi << "): ";
        std::string line;
        if (!std::getline(std::cin, line)) return -1;
        try {
            std::size_t used = 0;
            int value = std::stoi(line, &used);
            if (used == line.size() && value >= lo && value <= hi) return value;
        } catch (const std::exception&) {
            // fall through to error message
        }
        std::cout << "Invalid choice.\n";
    }
}

bool askYesNo(const std::string& prompt) {
    while (true) {
        std::cout << prompt << " (y/n): ";
        std::string line;
        if (!std::getline(std::cin, line)) return false;
        if (!line.empty()) {
            char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
            if (c == 'y') return true;
            if (c == 'n') return false;
        }
        std::cout << "Please answer y or n.\n";
    }
}

int main() {
    std::mt19937 rng{std::random_device{}()};
    QuestionBank bank;
    std::cout << "=== Quiz Game ===\n";

    do {
        const auto categories = bank.categories();
        std::cout << "\nCategories:\n";
        for (std::size_t i = 0; i < categories.size(); ++i) {
            std::cout << "  " << i + 1 << ") " << categories[i] << "\n";
        }
        std::cout << "  " << categories.size() + 1 << ") All categories\n";
        int choice = readChoice(1, static_cast<int>(categories.size()) + 1);
        if (choice < 0) {
            std::cout << "\nGoodbye.\n";
            return 0;
        }

        std::vector<Question> selected;
        if (choice == static_cast<int>(categories.size()) + 1) {
            for (std::size_t i = 0; i < categories.size(); ++i) {
                const auto& qs = bank.questionsFor(i);
                selected.insert(selected.end(), qs.begin(), qs.end());
            }
        } else {
            selected = bank.questionsFor(static_cast<std::size_t>(choice - 1));
        }

        Quiz quiz(std::move(selected), rng);
        quiz.play();
    } while (askYesNo("\nPlay again?"));

    std::cout << "Thanks for playing!\n";
    return 0;
}

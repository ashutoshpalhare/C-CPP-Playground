// Custom String Class (Advanced C++)
//
// `MyString` is a from-scratch string type that owns a heap buffer and
// demonstrates:
//   - RAII and the Rule of Five (destructor, copy/move ctor, copy/move assign)
//   - Small-buffer optimisation (SSO) for strings up to 15 chars
//   - Strong exception safety via copy-and-swap
//   - Operator overloading: + += == != < [] << >>
//   - Iterators compatible with range-for and <algorithm>
//   - A built-in self-test suite that doubles as usage documentation
//
// Standard: C++17. No dependencies beyond the standard library.

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

class MyString {
public:
    using size_type = std::size_t;
    static constexpr size_type npos = static_cast<size_type>(-1);

    // ----- construction / destruction (Rule of Five) -----
    MyString() noexcept : size_(0), capacity_(kSsoCapacity), data_(sso_) { sso_[0] = '\0'; }

    MyString(const char* s) : MyString() {  // NOLINT: implicit conversion is intentional for convenience
        if (s == nullptr) throw std::invalid_argument("MyString: null pointer");
        assign(s, std::strlen(s));
    }

    MyString(const char* s, size_type n) : MyString() {
        if (s == nullptr && n != 0) throw std::invalid_argument("MyString: null pointer");
        assign(s, n);
    }

    MyString(size_type n, char fill) : MyString() {
        reserve(n);
        std::memset(data_, fill, n);
        setSize(n);
    }

    MyString(const MyString& other) : MyString() { assign(other.data_, other.size_); }

    MyString(MyString&& other) noexcept : MyString() { moveFrom(std::move(other)); }

    // Copy-and-swap gives the strong exception guarantee for both assignments.
    MyString& operator=(MyString other) noexcept {
        swap(other);
        return *this;
    }

    ~MyString() { release(); }

    void swap(MyString& other) noexcept {
        // Both SSO: swap buffers byte-wise. Otherwise handle pointer ownership.
        MyString tmp(std::move(other));
        other.moveFrom(std::move(*this));
        moveFrom(std::move(tmp));
    }

    // ----- capacity -----
    size_type size() const noexcept { return size_; }
    size_type length() const noexcept { return size_; }
    size_type capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }
    bool usesSmallBuffer() const noexcept { return data_ == sso_; }

    void reserve(size_type newCap) {
        if (newCap <= capacity_) return;
        char* fresh = new char[newCap + 1];
        std::memcpy(fresh, data_, size_ + 1);
        release();
        data_ = fresh;
        capacity_ = newCap;
    }

    void clear() noexcept { setSize(0); }

    // ----- element access -----
    char& operator[](size_type i) noexcept { return data_[i]; }
    const char& operator[](size_type i) const noexcept { return data_[i]; }
    char& at(size_type i) {
        if (i >= size_) throw std::out_of_range("MyString::at index " + std::to_string(i) + " >= size " + std::to_string(size_));
        return data_[i];
    }
    const char& at(size_type i) const {
        if (i >= size_) throw std::out_of_range("MyString::at index " + std::to_string(i) + " >= size " + std::to_string(size_));
        return data_[i];
    }
    char& front() { return at(0); }
    char& back() { return at(size_ ? size_ - 1 : 0); }
    const char* c_str() const noexcept { return data_; }
    const char* data() const noexcept { return data_; }

    // ----- iterators -----
    char* begin() noexcept { return data_; }
    char* end() noexcept { return data_ + size_; }
    const char* begin() const noexcept { return data_; }
    const char* end() const noexcept { return data_ + size_; }

    // ----- modifiers -----
    MyString& append(const char* s, size_type n) {
        if (n == 0) return *this;
        if (s == nullptr) throw std::invalid_argument("MyString::append null");
        // s may alias our own buffer; compute offset before any reallocation.
        const bool aliases = s >= data_ && s < data_ + size_ + 1;
        const size_type offset = aliases ? static_cast<size_type>(s - data_) : 0;
        growTo(size_ + n);
        if (aliases) s = data_ + offset;
        std::memcpy(data_ + size_, s, n);
        setSize(size_ + n);
        return *this;
    }
    MyString& append(const MyString& s) { return append(s.data_, s.size_); }
    MyString& append(const char* s) { return append(s, s ? std::strlen(s) : 0); }
    void push_back(char c) { append(&c, 1); }
    void pop_back() {
        if (empty()) throw std::out_of_range("MyString::pop_back on empty string");
        setSize(size_ - 1);
    }

    MyString& operator+=(const MyString& s) { return append(s); }
    MyString& operator+=(const char* s) { return append(s); }
    MyString& operator+=(char c) { push_back(c); return *this; }

    MyString& insert(size_type pos, const MyString& s) {
        if (pos > size_) throw std::out_of_range("MyString::insert position out of range");
        MyString copy(s);  // protects against self-insert
        growTo(size_ + copy.size_);
        std::memmove(data_ + pos + copy.size_, data_ + pos, size_ - pos);
        std::memcpy(data_ + pos, copy.data_, copy.size_);
        setSize(size_ + copy.size_);
        return *this;
    }

    MyString& erase(size_type pos, size_type n = npos) {
        if (pos > size_) throw std::out_of_range("MyString::erase position out of range");
        n = std::min(n, size_ - pos);
        std::memmove(data_ + pos, data_ + pos + n, size_ - pos - n);
        setSize(size_ - n);
        return *this;
    }

    // ----- operations -----
    MyString substr(size_type pos, size_type n = npos) const {
        if (pos > size_) throw std::out_of_range("MyString::substr position out of range");
        return MyString(data_ + pos, std::min(n, size_ - pos));
    }

    size_type find(const MyString& needle, size_type from = 0) const noexcept {
        if (needle.size_ == 0) return from <= size_ ? from : npos;
        if (needle.size_ > size_) return npos;
        for (size_type i = from; i + needle.size_ <= size_; ++i)
            if (std::memcmp(data_ + i, needle.data_, needle.size_) == 0) return i;
        return npos;
    }
    size_type find(char c, size_type from = 0) const noexcept {
        for (size_type i = from; i < size_; ++i) if (data_[i] == c) return i;
        return npos;
    }

    int compare(const MyString& other) const noexcept {
        const int r = std::memcmp(data_, other.data_, std::min(size_, other.size_));
        if (r != 0) return r;
        return size_ < other.size_ ? -1 : size_ > other.size_ ? 1 : 0;
    }

    MyString toUpper() const {
        MyString out(*this);
        for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return out;
    }
    MyString toLower() const {
        MyString out(*this);
        for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return out;
    }
    MyString trimmed() const {
        size_type a = 0, b = size_;
        while (a < b && std::isspace(static_cast<unsigned char>(data_[a]))) ++a;
        while (b > a && std::isspace(static_cast<unsigned char>(data_[b - 1]))) --b;
        return substr(a, b - a);
    }
    MyString reversed() const {
        MyString out(*this);
        std::reverse(out.begin(), out.end());
        return out;
    }

private:
    static constexpr size_type kSsoCapacity = 15;

    size_type size_;
    size_type capacity_;
    char* data_;
    char sso_[kSsoCapacity + 1];

    void setSize(size_type n) noexcept {
        size_ = n;
        data_[n] = '\0';
    }

    void release() noexcept {
        if (!usesSmallBuffer()) delete[] data_;
        data_ = sso_;
        capacity_ = kSsoCapacity;
        size_ = 0;
        sso_[0] = '\0';
    }

    void growTo(size_type needed) {
        if (needed <= capacity_) return;
        size_type newCap = std::max(needed, capacity_ * 2);
        reserve(newCap);
    }

    void assign(const char* s, size_type n) {
        release();
        reserve(n);
        if (n) std::memcpy(data_, s, n);
        setSize(n);
    }

    // Takes ownership from `other`, leaving it empty. *this must own nothing heap-allocated.
    void moveFrom(MyString&& other) noexcept {
        release();
        if (other.usesSmallBuffer()) {
            std::memcpy(sso_, other.sso_, other.size_ + 1);
            data_ = sso_;
            capacity_ = kSsoCapacity;
            size_ = other.size_;
        } else {
            data_ = other.data_;
            capacity_ = other.capacity_;
            size_ = other.size_;
            other.data_ = other.sso_;
            other.capacity_ = kSsoCapacity;
        }
        other.size_ = 0;
        other.sso_[0] = '\0';
    }
};

// ----- non-member operators -----
inline MyString operator+(MyString lhs, const MyString& rhs) { lhs += rhs; return lhs; }
inline MyString operator+(MyString lhs, const char* rhs) { lhs += rhs; return lhs; }
inline MyString operator+(const char* lhs, const MyString& rhs) { MyString out(lhs); out += rhs; return out; }
inline bool operator==(const MyString& a, const MyString& b) noexcept { return a.compare(b) == 0; }
inline bool operator!=(const MyString& a, const MyString& b) noexcept { return !(a == b); }
inline bool operator<(const MyString& a, const MyString& b) noexcept { return a.compare(b) < 0; }
inline bool operator>(const MyString& a, const MyString& b) noexcept { return b < a; }
inline bool operator<=(const MyString& a, const MyString& b) noexcept { return !(b < a); }
inline bool operator>=(const MyString& a, const MyString& b) noexcept { return !(a < b); }
inline std::ostream& operator<<(std::ostream& os, const MyString& s) { return os.write(s.data(), static_cast<std::streamsize>(s.size())); }
inline std::istream& operator>>(std::istream& is, MyString& s) {
    std::string tmp;
    if (is >> tmp) s = MyString(tmp.c_str(), tmp.size());
    return is;
}
inline void swap(MyString& a, MyString& b) noexcept { a.swap(b); }

// ===================== self-test / demo =====================

namespace {
int g_failed = 0, g_passed = 0;
void check(bool cond, const char* what) {
    if (cond) ++g_passed;
    else { ++g_failed; std::cout << "  FAIL: " << what << "\n"; }
}
}  // namespace

static void runTests() {
    std::cout << "Running MyString self-tests...\n";

    // construction & SSO
    MyString empty;
    check(empty.empty() && empty.size() == 0 && std::strcmp(empty.c_str(), "") == 0, "default ctor is empty");
    MyString small("hello");
    check(small.size() == 5 && small.usesSmallBuffer(), "short string uses small buffer");
    MyString big("this string is definitely longer than fifteen characters");
    check(big.size() == 56 && !big.usesSmallBuffer(), "long string uses heap buffer");
    MyString filled(4, 'x');
    check(filled == "xxxx", "fill ctor");

    // copy / move
    MyString copy(big);
    check(copy == big && copy.c_str() != big.c_str(), "copy ctor deep-copies");
    MyString moved(std::move(copy));
    check(moved == big && copy.empty(), "move ctor steals buffer and empties source");
    MyString assigned;
    assigned = small;
    check(assigned == "hello", "copy assignment");
    assigned = std::move(moved);
    check(assigned == big, "move assignment");
    assigned = assigned;  // self-assignment must be safe
    check(assigned == big, "self-assignment");

    // growth crossing SSO boundary
    MyString grow("abcdefghijklmn");  // 14 chars
    grow += 'o';
    check(grow.size() == 15 && grow.usesSmallBuffer(), "15 chars still SSO");
    grow += 'p';
    check(grow.size() == 16 && !grow.usesSmallBuffer() && grow == "abcdefghijklmnop", "16th char moves to heap");

    // append aliasing itself
    MyString alias("abc");
    alias += alias;
    check(alias == "abcabc", "self-append");
    alias.append(alias.c_str() + 1);
    check(alias == "abcabcbcabc", "append from own buffer offset");

    // operators
    check(MyString("a") + "b" + MyString("c") == "abc", "operator+ chaining");
    check("x" + MyString("y") == "xy", "const char* + MyString");
    check(MyString("apple") < MyString("banana") && MyString("abc") < MyString("abcd"), "ordering");
    check(MyString("abc") != MyString("abd"), "inequality");

    // access & exceptions
    bool threw = false;
    try { (void)small.at(10); } catch (const std::out_of_range&) { threw = true; }
    check(threw, "at() throws out_of_range");
    threw = false;
    try { MyString nullStr(nullptr); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "null pointer ctor throws");
    check(small[0] == 'h' && small.front() == 'h' && small.back() == 'o', "indexing/front/back");

    // modifiers
    MyString m("hello world");
    m.insert(5, ",");
    check(m == "hello, world", "insert");
    m.erase(5, 1);
    check(m == "hello world", "erase");
    m.pop_back();
    check(m == "hello worl", "pop_back");
    m.clear();
    check(m.empty() && m.capacity() >= 10, "clear keeps capacity");

    // operations
    MyString s("  The Quick Brown Fox  ");
    check(s.trimmed() == "The Quick Brown Fox", "trimmed");
    check(s.trimmed().toUpper() == "THE QUICK BROWN FOX", "toUpper");
    check(s.trimmed().toLower() == "the quick brown fox", "toLower");
    check(MyString("abc").reversed() == "cba", "reversed");
    check(s.find("Quick") == 6 && s.find("zzz") == MyString::npos && s.find('F') == 16, "find");
    check(s.substr(6, 5) == "Quick" && s.substr(21) == "  ", "substr");

    // iterators with <algorithm>
    MyString letters("dcba");
    std::sort(letters.begin(), letters.end());
    check(letters == "abcd", "std::sort via iterators");
    int vowels = 0;
    for (char c : MyString("education")) if (MyString("aeiou").find(c) != MyString::npos) ++vowels;
    check(vowels == 5, "range-for");

    // streams
    std::ostringstream os;
    os << MyString("stream") << '!' << MyString();
    check(os.str() == "stream!", "operator<<");
    std::istringstream is("word1 word2");
    MyString w1, w2;
    is >> w1 >> w2;
    check(w1 == "word1" && w2 == "word2", "operator>>");

    // swap
    MyString a("short"), b("a much longer string that lives on the heap");
    swap(a, b);
    check(a == "a much longer string that lives on the heap" && b == "short" && b.usesSmallBuffer(), "swap mixed SSO/heap");

    std::cout << g_passed << " passed, " << g_failed << " failed.\n";
}

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--test") == 0) {
        runTests();
        return g_failed == 0 ? 0 : 1;
    }

    runTests();
    std::cout << "\nInteractive demo - type words (Ctrl+D / Ctrl+Z to finish):\n";
    MyString joined;
    MyString word;
    std::size_t count = 0;
    while (std::cin >> word) {
        if (!joined.empty()) joined += ' ';
        joined += word;
        ++count;
    }
    if (count > 0) {
        std::cout << "\nYou entered " << count << " word(s): \"" << joined << "\"\n";
        std::cout << "Upper:    " << joined.toUpper() << "\n";
        std::cout << "Reversed: " << joined.reversed() << "\n";
        std::cout << "Length " << joined.size() << ", capacity " << joined.capacity() << ", storage: " << (joined.usesSmallBuffer() ? "small buffer" : "heap") << "\n";
    }
    return g_failed == 0 ? 0 : 1;
}

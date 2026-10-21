// Memory Pool Allocator (Advanced C++)
//
// Two production-style allocators plus a benchmark and self-tests:
//
//   1. FixedPool<BlockSize, Alignment>
//      A free-list pool of fixed-size blocks carved from large chunks.
//      O(1) allocate/deallocate, no per-allocation heap calls, detects
//      double frees and foreign pointers in debug checks.
//
//   2. PoolAllocator<T>
//      A C++17 Allocator adapter so STL containers (std::list, std::map,
//      std::unordered_map...) can use the pool transparently.
//
//   3. ArenaAllocator
//      A bump/linear allocator with alignment support and a reset()
//      that frees everything at once (great for per-frame scratch memory).
//
// Also shows: placement new, std::aligned_storage-free alignment math,
// RAII ownership, thread-safety option via std::mutex, and a benchmark
// against the global operator new/delete.
//
// Standard: C++17. Add -pthread on Linux. Portable to Windows, Linux, macOS.

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace mem {

constexpr bool isPowerOfTwo(std::size_t n) { return n && !(n & (n - 1)); }
constexpr std::size_t alignUp(std::size_t n, std::size_t a) { return (n + a - 1) & ~(a - 1); }

// ---------------------------------------------------------------------
// FixedPool: fixed-size blocks, intrusive singly linked free list.
// ---------------------------------------------------------------------
template <std::size_t BlockSize, std::size_t Alignment = alignof(std::max_align_t)>
class FixedPool {
    static_assert(isPowerOfTwo(Alignment), "Alignment must be a power of two");

public:
    static constexpr std::size_t kBlockSize = alignUp(BlockSize < sizeof(void*) ? sizeof(void*) : BlockSize, Alignment);

    explicit FixedPool(std::size_t blocksPerChunk = 256) : blocksPerChunk_(blocksPerChunk) {
        if (blocksPerChunk == 0) throw std::invalid_argument("blocksPerChunk must be > 0");
    }
    ~FixedPool() {
        for (void* chunk : chunks_) ::operator delete(chunk, std::align_val_t{Alignment});
    }
    FixedPool(const FixedPool&) = delete;
    FixedPool& operator=(const FixedPool&) = delete;

    void* allocate() {
        if (freeList_ == nullptr) addChunk();
        FreeNode* node = freeList_;
        freeList_ = node->next;
        ++liveBlocks_;
        return node;
    }

    void deallocate(void* p) {
        if (p == nullptr) return;
        if (!owns(p)) throw std::logic_error("FixedPool::deallocate: pointer not from this pool");
        if (isFree(p)) throw std::logic_error("FixedPool::deallocate: double free detected");
        auto* node = static_cast<FreeNode*>(p);
        node->next = freeList_;
        freeList_ = node;
        --liveBlocks_;
    }

    bool owns(const void* p) const noexcept {
        const auto addr = reinterpret_cast<std::uintptr_t>(p);
        for (void* chunk : chunks_) {
            const auto base = reinterpret_cast<std::uintptr_t>(chunk);
            if (addr >= base && addr < base + chunkBytes()) return (addr - base) % kBlockSize == 0;
        }
        return false;
    }

    std::size_t liveBlocks() const noexcept { return liveBlocks_; }
    std::size_t capacity() const noexcept { return chunks_.size() * blocksPerChunk_; }
    std::size_t chunks() const noexcept { return chunks_.size(); }
    std::size_t bytesReserved() const noexcept { return chunks_.size() * chunkBytes(); }

private:
    struct FreeNode { FreeNode* next; };

    std::size_t blocksPerChunk_;
    std::vector<void*> chunks_;
    FreeNode* freeList_ = nullptr;
    std::size_t liveBlocks_ = 0;

    std::size_t chunkBytes() const noexcept { return blocksPerChunk_ * kBlockSize; }

    void addChunk() {
        void* raw = ::operator new(chunkBytes(), std::align_val_t{Alignment});
        chunks_.push_back(raw);
        // Thread the new blocks onto the free list, last to first so the list is in address order.
        auto* bytes = static_cast<unsigned char*>(raw);
        for (std::size_t i = blocksPerChunk_; i-- > 0;) {
            auto* node = reinterpret_cast<FreeNode*>(bytes + i * kBlockSize);
            node->next = freeList_;
            freeList_ = node;
        }
    }

    // O(free list length) - only used in the debug double-free check.
    bool isFree(const void* p) const noexcept {
        for (const FreeNode* n = freeList_; n; n = n->next) if (n == p) return true;
        return false;
    }
};

// Optional thread-safe wrapper.
template <typename Pool>
class LockedPool {
public:
    template <typename... Args>
    explicit LockedPool(Args&&... args) : pool_(std::forward<Args>(args)...) {}
    void* allocate() { std::lock_guard<std::mutex> g(m_); return pool_.allocate(); }
    void deallocate(void* p) { std::lock_guard<std::mutex> g(m_); pool_.deallocate(p); }
    std::size_t liveBlocks() { std::lock_guard<std::mutex> g(m_); return pool_.liveBlocks(); }
private:
    std::mutex m_;
    Pool pool_;
};

// ---------------------------------------------------------------------
// PoolAllocator<T>: STL-compatible allocator backed by a shared FixedPool.
// Containers like std::list allocate nodes larger than T, so we size the
// pool for sizeof(T) + node overhead and fall back to new for larger asks.
// ---------------------------------------------------------------------
template <typename T, std::size_t NodeBytes = sizeof(T) + 4 * sizeof(void*)>
class PoolAllocator {
public:
    using value_type = T;
    using Pool = FixedPool<NodeBytes, alignof(std::max_align_t)>;

    PoolAllocator() : pool_(std::make_shared<Pool>()) {}
    template <typename U, std::size_t N>
    PoolAllocator(const PoolAllocator<U, N>& other) noexcept : pool_(other.pool_) {}

    T* allocate(std::size_t n) {
        if (n == 1 && sizeof(T) <= Pool::kBlockSize) return static_cast<T*>(pool_->allocate());
        return static_cast<T*>(::operator new(n * sizeof(T)));  // arrays: fall back
    }
    void deallocate(T* p, std::size_t n) noexcept {
        if (n == 1 && sizeof(T) <= Pool::kBlockSize && pool_->owns(p)) pool_->deallocate(p);
        else ::operator delete(p);
    }

    template <typename U>
    struct rebind { using other = PoolAllocator<U, NodeBytes>; };

    std::shared_ptr<Pool> pool_;  // shared so rebound copies use the same pool
};
template <typename T, std::size_t N, typename U, std::size_t M>
bool operator==(const PoolAllocator<T, N>& a, const PoolAllocator<U, M>& b) { return a.pool_ == b.pool_; }
template <typename T, std::size_t N, typename U, std::size_t M>
bool operator!=(const PoolAllocator<T, N>& a, const PoolAllocator<U, M>& b) { return !(a == b); }

// ---------------------------------------------------------------------
// ArenaAllocator: bump pointer with alignment; reset() frees all at once.
// ---------------------------------------------------------------------
class ArenaAllocator {
public:
    explicit ArenaAllocator(std::size_t bytes) : size_(bytes), buffer_(static_cast<unsigned char*>(std::malloc(bytes))) {
        if (buffer_ == nullptr) throw std::bad_alloc();
    }
    ~ArenaAllocator() { std::free(buffer_); }
    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t)) {
        if (!isPowerOfTwo(alignment)) throw std::invalid_argument("alignment must be a power of two");
        const std::size_t start = alignUp(offset_, alignment);
        if (bytes > size_ || start > size_ - bytes) throw std::bad_alloc();
        offset_ = start + bytes;
        ++allocations_;
        peak_ = std::max(peak_, offset_);
        return buffer_ + start;
    }

    template <typename T, typename... Args>
    T* create(Args&&... args) {
        void* p = allocate(sizeof(T), alignof(T));
        return new (p) T(std::forward<Args>(args)...);  // caller must call ~T() if non-trivial
    }

    void reset() noexcept { offset_ = 0; allocations_ = 0; }
    std::size_t used() const noexcept { return offset_; }
    std::size_t capacity() const noexcept { return size_; }
    std::size_t peak() const noexcept { return peak_; }
    std::size_t allocations() const noexcept { return allocations_; }

private:
    std::size_t size_;
    unsigned char* buffer_;
    std::size_t offset_ = 0;
    std::size_t peak_ = 0;
    std::size_t allocations_ = 0;
};

}  // namespace mem

// ===================== tests & benchmark =====================

namespace {

int g_pass = 0, g_fail = 0;
void check(bool ok, const char* what) {
    if (ok) ++g_pass;
    else { ++g_fail; std::cout << "  FAIL: " << what << "\n"; }
}

struct Particle {
    double x, y, z, vx, vy, vz;
    int id;
};

void runTests() {
    std::cout << "Running allocator self-tests...\n";

    // FixedPool basics
    mem::FixedPool<sizeof(Particle)> pool(4);
    check(pool.capacity() == 0 && pool.liveBlocks() == 0, "pool starts empty");
    void* a = pool.allocate();
    void* b = pool.allocate();
    check(a != b && pool.liveBlocks() == 2 && pool.capacity() == 4, "two distinct blocks from one chunk");
    check(reinterpret_cast<std::uintptr_t>(a) % alignof(std::max_align_t) == 0, "block is aligned");
    void* c = pool.allocate();
    void* d = pool.allocate();
    void* e = pool.allocate();  // forces a second chunk
    check(pool.chunks() == 2 && pool.capacity() == 8, "second chunk added on demand");
    pool.deallocate(c);
    void* c2 = pool.allocate();
    check(c2 == c, "freed block is reused first (LIFO)");
    bool threw = false;
    try { pool.deallocate(c); pool.deallocate(c); } catch (const std::logic_error&) { threw = true; }
    check(threw, "double free detected");
    threw = false;
    int stackVar = 0;
    try { pool.deallocate(&stackVar); } catch (const std::logic_error&) { threw = true; }
    check(threw, "foreign pointer rejected");
    check(pool.owns(a) && !pool.owns(&stackVar), "owns()");
    pool.deallocate(a); pool.deallocate(b); pool.deallocate(d); pool.deallocate(e);
    check(pool.liveBlocks() == 0, "all blocks returned");

    // Placement new with the pool
    Particle* p = new (pool.allocate()) Particle{1, 2, 3, 0, 0, 0, 42};
    check(p->id == 42 && p->z == 3, "placement new into pool block");
    p->~Particle();
    pool.deallocate(p);

    // STL container on PoolAllocator
    {
        std::list<int, mem::PoolAllocator<int>> lst;
        for (int i = 0; i < 1000; ++i) lst.push_back(i);
        long long sum = 0;
        for (int v : lst) sum += v;
        check(sum == 499500 && lst.size() == 1000, "std::list with PoolAllocator");
        lst.clear();
        check(lst.get_allocator().pool_->liveBlocks() == 0, "list nodes returned to pool");

        std::map<int, std::string, std::less<int>, mem::PoolAllocator<std::pair<const int, std::string>>> m;
        for (int i = 0; i < 100; ++i) m[i] = "v" + std::to_string(i);
        check(m.size() == 100 && m[57] == "v57", "std::map with PoolAllocator");
    }

    // Arena
    mem::ArenaAllocator arena(1024);
    void* a1 = arena.allocate(10, 1);
    void* a2 = arena.allocate(16, 64);
    check(reinterpret_cast<std::uintptr_t>(a2) % 64 == 0 && a2 > a1, "arena honours alignment");
    auto* part = arena.create<Particle>(Particle{0, 0, 0, 1, 1, 1, 7});
    check(part->id == 7 && arena.allocations() == 3, "arena create<T>");
    threw = false;
    try { arena.allocate(2000); } catch (const std::bad_alloc&) { threw = true; }
    check(threw, "arena overflow throws bad_alloc");
    const std::size_t usedBefore = arena.used();
    arena.reset();
    check(arena.used() == 0 && arena.peak() == usedBefore, "arena reset keeps peak statistics");

    // Locked pool smoke test
    mem::LockedPool<mem::FixedPool<32>> locked(16);
    void* l1 = locked.allocate();
    locked.deallocate(l1);
    check(locked.liveBlocks() == 0, "locked pool wrapper");

    std::cout << g_pass << " passed, " << g_fail << " failed.\n";
}

void runBenchmark(int iterations) {
    using clock = std::chrono::steady_clock;
    std::vector<Particle*> ptrs(static_cast<std::size_t>(iterations));

    auto t0 = clock::now();
    for (int round = 0; round < 10; ++round) {
        for (auto& ptr : ptrs) ptr = new Particle{};
        for (auto& ptr : ptrs) delete ptr;
    }
    auto t1 = clock::now();

    mem::FixedPool<sizeof(Particle), alignof(Particle)> pool(1024);
    for (int round = 0; round < 10; ++round) {
        for (auto& ptr : ptrs) ptr = static_cast<Particle*>(pool.allocate());
        for (auto& ptr : ptrs) pool.deallocate(ptr);
    }
    auto t2 = clock::now();

    mem::ArenaAllocator arena(static_cast<std::size_t>(iterations) * sizeof(Particle) + 64);
    for (int round = 0; round < 10; ++round) {
        for (auto& ptr : ptrs) ptr = static_cast<Particle*>(arena.allocate(sizeof(Particle), alignof(Particle)));
        arena.reset();
    }
    auto t3 = clock::now();

    auto ms = [](auto d) { return std::chrono::duration_cast<std::chrono::microseconds>(d).count() / 1000.0; };
    std::cout << "\nBenchmark: " << iterations << " x " << sizeof(Particle) << "-byte objects, 10 rounds of alloc+free\n";
    std::cout << "  new/delete : " << ms(t1 - t0) << " ms\n";
    std::cout << "  FixedPool  : " << ms(t2 - t1) << " ms  (" << pool.bytesReserved() / 1024 << " KiB reserved in " << pool.chunks() << " chunks)\n";
    std::cout << "  Arena      : " << ms(t3 - t2) << " ms  (peak " << arena.peak() / 1024 << " KiB)\n";
    std::cout << "Note: results vary by platform, optimiser and system allocator; build with -O2 for meaningful numbers.\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        runTests();
        int iterations = 100000;
        if (argc > 1) {
            try {
                iterations = std::stoi(argv[1]);
                if (iterations < 1 || iterations > 10'000'000) throw std::out_of_range("range");
            } catch (const std::exception&) {
                std::cerr << "Usage: " << argv[0] << " [iterations 1..10000000]\n";
                return 1;
            }
        }
        runBenchmark(iterations);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return g_fail == 0 ? 0 : 1;
}

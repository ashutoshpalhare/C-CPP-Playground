/*
 * Memory Allocator Simulator (C11)
 *
 * Simulates a heap of N bytes managed with a linked list of blocks. Nothing here
 * replaces malloc/free: the "heap" is only a list of {start, size, owner}
 * descriptors, so you can safely experiment and watch fragmentation appear.
 *
 * Concepts shown:
 *   - first-fit, best-fit and worst-fit placement strategies
 *   - splitting a free block when it is larger than needed
 *   - coalescing neighbouring free blocks when memory is freed
 *   - alignment (sizes are rounded up to a multiple of ALIGNMENT)
 *   - internal fragmentation (rounding waste) and external fragmentation
 *     (free memory split into pieces that are too small to use)
 *
 * Commands: alloc N | free ID | strategy first|best|worst | show | stats |
 *           reset [SIZE] | demo | help | quit
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIGNMENT 8
#define DEFAULT_HEAP 1024
#define MIN_HEAP 64
#define MAX_HEAP (1024UL * 1024UL)
#define BAR_WIDTH 64
#define LINE_CAP 128

typedef enum { FIRST_FIT, BEST_FIT, WORST_FIT } Strategy;

typedef struct Block {
    size_t start;
    size_t size;
    size_t requested; /* bytes the caller asked for (0 when free) */
    int id;           /* 0 means the block is free */
    struct Block *next;
} Block;

typedef struct {
    Block *head;
    size_t heap_size;
    Strategy strategy;
    int next_id;
    unsigned long successes;
    unsigned long failures;
} Heap;

static const char *strategy_name(Strategy s) {
    switch (s) {
        case FIRST_FIT: return "first-fit";
        case BEST_FIT: return "best-fit";
        case WORST_FIT: return "worst-fit";
    }
    return "unknown";
}

static size_t align_up(size_t n) {
    return (n + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;
}

/* ---------- heap operations ---------- */

static void heap_clear(Heap *h) {
    Block *b = h->head;
    while (b) {
        Block *next = b->next;
        free(b);
        b = next;
    }
    h->head = NULL;
}

/* Returns 1 on success. The size is rounded down to a multiple of ALIGNMENT. */
static int heap_init(Heap *h, size_t size) {
    size -= size % ALIGNMENT;
    Block *first = malloc(sizeof *first);
    if (!first) return 0;
    first->start = 0;
    first->size = size;
    first->requested = 0;
    first->id = 0;
    first->next = NULL;
    h->head = first;
    h->heap_size = size;
    h->next_id = 1;
    h->successes = h->failures = 0;
    return 1;
}

static Block *find_block(const Heap *h, size_t need) {
    Block *chosen = NULL;
    for (Block *b = h->head; b; b = b->next) {
        if (b->id != 0 || b->size < need) continue;
        if (h->strategy == FIRST_FIT) return b;
        if (!chosen) chosen = b;
        else if (h->strategy == BEST_FIT && b->size < chosen->size) chosen = b;
        else if (h->strategy == WORST_FIT && b->size > chosen->size) chosen = b;
    }
    return chosen;
}

/* Returns the new block's id, or 0 if no free block is large enough. */
static int heap_alloc(Heap *h, size_t requested) {
    size_t need = align_up(requested);
    Block *b = (requested > 0 && need >= requested) ? find_block(h, need) : NULL;
    if (!b) {
        h->failures++;
        return 0;
    }
    if (b->size >= need + ALIGNMENT) { /* split off the unused tail as a new free block */
        Block *rest = malloc(sizeof *rest);
        if (rest) {
            rest->start = b->start + need;
            rest->size = b->size - need;
            rest->requested = 0;
            rest->id = 0;
            rest->next = b->next;
            b->size = need;
            b->next = rest;
        } /* if malloc failed we simply hand out the whole block (more internal waste) */
    }
    b->id = h->next_id++;
    b->requested = requested;
    h->successes++;
    return b->id;
}

/* Returns 1 if a block with that id existed and was freed. */
static int heap_free(Heap *h, int id) {
    Block *prev = NULL, *b = h->head;
    while (b && b->id != id) {
        prev = b;
        b = b->next;
    }
    if (!b) return 0;
    b->id = 0;
    b->requested = 0;
    if (b->next && b->next->id == 0) { /* merge with the following free block */
        Block *n = b->next;
        b->size += n->size;
        b->next = n->next;
        free(n);
    }
    if (prev && prev->id == 0) { /* merge into the preceding free block */
        prev->size += b->size;
        prev->next = b->next;
        free(b);
    }
    return 1;
}

typedef struct {
    size_t used, free_total, largest_free, internal_waste;
    unsigned long free_blocks, used_blocks;
} Stats;

static Stats compute_stats(const Heap *h) {
    Stats s = {0, 0, 0, 0, 0, 0};
    for (const Block *b = h->head; b; b = b->next) {
        if (b->id == 0) {
            s.free_total += b->size;
            s.free_blocks++;
            if (b->size > s.largest_free) s.largest_free = b->size;
        } else {
            s.used += b->size;
            s.used_blocks++;
            s.internal_waste += b->size - b->requested;
        }
    }
    return s;
}

static double external_fragmentation(const Stats *s) {
    if (s->free_total == 0) return 0.0;
    return 100.0 * (1.0 - (double)s->largest_free / (double)s->free_total);
}

/* ---------- display ---------- */

static void print_bar(const Heap *h) {
    char bar[BAR_WIDTH + 1];
    const Block *b = h->head;
    for (int i = 0; i < BAR_WIDTH; i++) {
        size_t offset = (size_t)i * h->heap_size / BAR_WIDTH;
        while (b && b->start + b->size <= offset) b = b->next;
        if (!b) bar[i] = '?';
        else if (b->id == 0) bar[i] = '.';
        else bar[i] = (char)('0' + b->id % 10);
    }
    bar[BAR_WIDTH] = '\0';
    printf("[%s]\n", bar);
    puts("('.' = free, digit = last digit of the owning block id)");
}

static void print_heap(const Heap *h) {
    printf("Heap: %lu bytes | strategy: %s | alignment: %d\n", (unsigned long)h->heap_size,
           strategy_name(h->strategy), ALIGNMENT);
    print_bar(h);
    printf("%-5s %8s %8s %8s  %-9s %s\n", "ID", "Start", "End", "Size", "State", "Requested");
    for (const Block *b = h->head; b; b = b->next) {
        char id[16];
        if (b->id == 0) snprintf(id, sizeof id, "-");
        else snprintf(id, sizeof id, "%d", b->id);
        printf("%-5s %8lu %8lu %8lu  %-9s ", id, (unsigned long)b->start, (unsigned long)(b->start + b->size - 1),
               (unsigned long)b->size, b->id == 0 ? "free" : "allocated");
        if (b->id == 0) puts("");
        else printf("%lu\n", (unsigned long)b->requested);
    }
}

static void print_stats(const Heap *h) {
    Stats s = compute_stats(h);
    printf("Allocated blocks     : %lu (%lu bytes)\n", s.used_blocks, (unsigned long)s.used);
    printf("Free blocks          : %lu (%lu bytes)\n", s.free_blocks, (unsigned long)s.free_total);
    printf("Largest free block   : %lu bytes\n", (unsigned long)s.largest_free);
    printf("Internal waste       : %lu bytes (rounding to %d-byte alignment)\n", (unsigned long)s.internal_waste, ALIGNMENT);
    printf("External fragmentation: %.1f%% (1 - largest free / total free)\n", external_fragmentation(&s));
    printf("Requests             : %lu succeeded, %lu failed\n", h->successes, h->failures);
}

/* ---------- demo ---------- */

static void run_demo(void) {
    static const size_t first_wave[] = {100, 200, 50, 300, 80, 120};
    static const int to_free[] = {2, 4, 6};
    static const size_t second_wave[] = {150, 60, 250, 90, 40};
    puts("Demo: 1024-byte heap, six allocations, free blocks 2, 4 and 6, then five more requests.");
    puts("Each strategy replays the same script.\n");
    printf("%-10s %9s %7s %13s %14s\n", "Strategy", "Succeeded", "Failed", "Largest free", "Ext. fragment.");
    for (int s = FIRST_FIT; s <= WORST_FIT; s++) {
        Heap h;
        h.strategy = (Strategy)s;
        if (!heap_init(&h, 1024)) {
            puts("Out of memory.");
            return;
        }
        for (size_t i = 0; i < sizeof first_wave / sizeof first_wave[0]; i++) heap_alloc(&h, first_wave[i]);
        for (size_t i = 0; i < sizeof to_free / sizeof to_free[0]; i++) heap_free(&h, to_free[i]);
        for (size_t i = 0; i < sizeof second_wave / sizeof second_wave[0]; i++) heap_alloc(&h, second_wave[i]);
        Stats st = compute_stats(&h);
        printf("%-10s %9lu %7lu %13lu %13.1f%%\n", strategy_name(h.strategy), h.successes, h.failures,
               (unsigned long)st.largest_free, external_fragmentation(&st));
        heap_clear(&h);
    }
    puts("\nTip: use 'reset', 'alloc', 'free' and 'show' to replay the scenario step by step.");
}

/* ---------- command parsing ---------- */

/* Parses a plain non-negative decimal integer. Returns 1 on success. */
static int parse_size(const char *text, unsigned long *out) {
    if (!isdigit((unsigned char)text[0])) return 0;
    char *end;
    errno = 0;
    unsigned long v = strtoul(text, &end, 10);
    if (errno != 0 || *end != '\0') return 0;
    *out = v;
    return 1;
}

static void print_help(void) {
    puts("Commands:");
    puts("  alloc N                 request N bytes (rounded up to a multiple of 8)");
    puts("  free ID                 release the block with that id");
    puts("  strategy first|best|worst   choose the placement strategy");
    puts("  show                    list every block and draw a map of the heap");
    puts("  stats                   fragmentation and usage figures");
    puts("  reset [SIZE]            start over with a new heap (64 to 1048576 bytes, default 1024)");
    puts("  demo                    compare the three strategies on one scenario");
    puts("  help, quit");
}

int main(void) {
    Heap heap;
    heap.strategy = FIRST_FIT;
    if (!heap_init(&heap, DEFAULT_HEAP)) {
        fputs("Out of memory.\n", stderr);
        return 1;
    }
    puts("Memory Allocator Simulator - type 'help' for commands.");

    char line[LINE_CAP];
    for (;;) {
        printf("alloc> ");
        if (!fgets(line, sizeof line, stdin)) break;
        if (strchr(line, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("Input line too long.");
            continue;
        }

        char cmd[16] = "", arg[32] = "", extra[32] = "";
        int fields = sscanf(line, "%15s %31s %31s", cmd, arg, extra);
        if (fields <= 0) continue;
        if (fields == 3) {
            puts("Too many arguments. Type 'help'.");
            continue;
        }

        unsigned long value = 0;
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "alloc") == 0) {
            if (fields != 2 || !parse_size(arg, &value) || value == 0 || value > heap.heap_size) {
                printf("Usage: alloc N   (N from 1 to %lu)\n", (unsigned long)heap.heap_size);
            } else {
                int id = heap_alloc(&heap, (size_t)value);
                if (id) printf("Allocated block %d (%lu bytes requested).\n", id, value);
                else puts("Allocation failed: no free block is large enough (try 'stats' to see fragmentation).");
            }
        } else if (strcmp(cmd, "free") == 0) {
            if (fields != 2 || !parse_size(arg, &value) || value == 0 || value > 2147483647UL) {
                puts("Usage: free ID   (ID is a positive block number from 'show')");
            } else if (heap_free(&heap, (int)value)) {
                printf("Freed block %lu.\n", value);
            } else {
                printf("No allocated block with id %lu.\n", value);
            }
        } else if (strcmp(cmd, "strategy") == 0) {
            if (fields == 2 && strcmp(arg, "first") == 0) heap.strategy = FIRST_FIT;
            else if (fields == 2 && strcmp(arg, "best") == 0) heap.strategy = BEST_FIT;
            else if (fields == 2 && strcmp(arg, "worst") == 0) heap.strategy = WORST_FIT;
            else {
                puts("Usage: strategy first|best|worst");
                continue;
            }
            printf("Strategy set to %s.\n", strategy_name(heap.strategy));
        } else if (strcmp(cmd, "show") == 0) {
            print_heap(&heap);
        } else if (strcmp(cmd, "stats") == 0) {
            print_stats(&heap);
        } else if (strcmp(cmd, "reset") == 0) {
            unsigned long size = DEFAULT_HEAP;
            if (fields == 2 && (!parse_size(arg, &size) || size < MIN_HEAP || size > MAX_HEAP)) {
                printf("Heap size must be a number from %d to %lu.\n", MIN_HEAP, MAX_HEAP);
                continue;
            }
            Strategy keep = heap.strategy;
            heap_clear(&heap);
            if (!heap_init(&heap, (size_t)size)) {
                fputs("Out of memory.\n", stderr);
                return 1;
            }
            heap.strategy = keep;
            printf("Heap reset to %lu bytes.\n", (unsigned long)heap.heap_size);
        } else if (strcmp(cmd, "demo") == 0) {
            run_demo();
        } else {
            printf("Unknown command \"%s\". Type 'help'.\n", cmd);
        }
    }
    heap_clear(&heap);
    return 0;
}

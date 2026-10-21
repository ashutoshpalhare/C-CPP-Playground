/*
 * File Compression Tool (C11) - Huffman coding
 *
 * Usage:
 *   huffc compress   <input> <output>
 *   huffc decompress <input> <output>      (short forms: c, d)
 *
 * Compressed file format ("HUF1", all integers little-endian):
 *   bytes 0-3      magic "HUF1"
 *   bytes 4-1027   256 x uint32 : how many times each byte value occurs
 *   bytes 1028-    the Huffman-coded bit stream, most significant bit first,
 *                  padded with zero bits to a whole byte
 *
 * The decoder rebuilds the exact same Huffman tree from the frequency table.
 * That only works because tree construction is fully deterministic: ties are
 * always broken by node number.
 *
 * Limitations: input files must be smaller than 4 GiB (counts are 32-bit), and
 * the fixed 1 KiB header means small files usually get BIGGER, not smaller.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOLS 256
#define MAX_NODES (2 * SYMBOLS - 1)
#define MAGIC "HUF1"
#define MAGIC_LEN 4
#define HEADER_BYTES (MAGIC_LEN + SYMBOLS * 4)

/* Leaves use node numbers 0..255 (the byte value); internal nodes follow. */
typedef struct {
    uint64_t weight;
    int left;  /* -1 for leaves */
    int right; /* -1 for leaves */
} Node;

typedef struct {
    Node nodes[MAX_NODES];
    int node_count;
    int root;     /* -1 when the input is empty */
    int distinct; /* number of byte values that occur at least once */
} Tree;

typedef struct {
    unsigned char bits[SYMBOLS]; /* one entry per bit: 0 or 1 */
    int len;
} Code;

/* ---------- tree construction ---------- */

/* Is node a "smaller" than node b? Lower weight first, then lower node number. */
static int node_less(const Tree *t, int a, int b) {
    if (t->nodes[a].weight != t->nodes[b].weight) return t->nodes[a].weight < t->nodes[b].weight;
    return a < b;
}

static void build_tree(Tree *t, const uint64_t freq[SYMBOLS]) {
    int alive[MAX_NODES];
    int alive_count = 0;

    memset(t, 0, sizeof *t);
    t->node_count = SYMBOLS;
    for (int s = 0; s < SYMBOLS; s++) {
        t->nodes[s].weight = freq[s];
        t->nodes[s].left = t->nodes[s].right = -1;
        if (freq[s] > 0) alive[alive_count++] = s;
    }
    t->distinct = alive_count;
    t->root = -1;
    if (alive_count == 0) return;

    while (alive_count > 1) {
        int first = 0;
        for (int i = 1; i < alive_count; i++) {
            if (node_less(t, alive[i], alive[first])) first = i;
        }
        int second = (first == 0) ? 1 : 0;
        for (int i = 0; i < alive_count; i++) {
            if (i != first && node_less(t, alive[i], alive[second])) second = i;
        }

        int id = t->node_count++;
        t->nodes[id].left = alive[first];
        t->nodes[id].right = alive[second];
        t->nodes[id].weight = t->nodes[alive[first]].weight + t->nodes[alive[second]].weight;

        alive[first] = id;                 /* the new node replaces the first child */
        alive[second] = alive[alive_count - 1]; /* fill the second slot with the last entry */
        alive_count--;
    }
    t->root = alive[0];
}

static void assign_codes(const Tree *t, int node, Code *codes, unsigned char *path, int depth) {
    const Node *n = &t->nodes[node];
    if (n->left < 0) {
        codes[node].len = depth;
        memcpy(codes[node].bits, path, (size_t)depth);
        return;
    }
    path[depth] = 0;
    assign_codes(t, n->left, codes, path, depth + 1);
    path[depth] = 1;
    assign_codes(t, n->right, codes, path, depth + 1);
}

/* ---------- little-endian integer helpers ---------- */

static void put_u32(unsigned char *out, uint32_t v) {
    out[0] = (unsigned char)(v & 0xFF);
    out[1] = (unsigned char)((v >> 8) & 0xFF);
    out[2] = (unsigned char)((v >> 16) & 0xFF);
    out[3] = (unsigned char)((v >> 24) & 0xFF);
}

static uint32_t get_u32(const unsigned char *in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
}

/* ---------- bit writer ---------- */

typedef struct {
    FILE *fp;
    unsigned current;
    int filled;
    uint64_t bytes_written;
    int error;
} BitWriter;

static void bw_put_byte(BitWriter *w) {
    if (fputc((int)w->current, w->fp) == EOF) w->error = 1;
    w->bytes_written++;
    w->current = 0;
    w->filled = 0;
}

static void bw_put_bit(BitWriter *w, int bit) {
    w->current = (w->current << 1) | (unsigned)(bit & 1);
    if (++w->filled == 8) bw_put_byte(w);
}

static void bw_flush(BitWriter *w) {
    if (w->filled > 0) {
        w->current <<= (8 - w->filled);
        bw_put_byte(w);
    }
}

/* ---------- compress ---------- */

static int compress_file(const char *in_path, const char *out_path) {
    FILE *in = fopen(in_path, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open input file \"%s\".\n", in_path);
        return 1;
    }

    uint64_t freq[SYMBOLS];
    memset(freq, 0, sizeof freq);
    uint64_t total = 0;
    int c;
    while ((c = fgetc(in)) != EOF) {
        freq[c]++;
        total++;
    }
    if (ferror(in)) {
        fprintf(stderr, "Error while reading \"%s\".\n", in_path);
        fclose(in);
        return 1;
    }
    if (total > UINT32_MAX) {
        fprintf(stderr, "Input is too large (limit is 4 GiB minus 1 byte).\n");
        fclose(in);
        return 1;
    }

    Tree *tree = malloc(sizeof *tree);
    Code *codes = calloc(SYMBOLS, sizeof *codes);
    if (!tree || !codes) {
        fprintf(stderr, "Out of memory.\n");
        free(tree);
        free(codes);
        fclose(in);
        return 1;
    }
    build_tree(tree, freq);
    if (tree->root >= 0) {
        unsigned char path[SYMBOLS];
        assign_codes(tree, tree->root, codes, path, 0);
    }

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file \"%s\".\n", out_path);
        free(tree);
        free(codes);
        fclose(in);
        return 1;
    }

    int ok = 1;
    unsigned char header[HEADER_BYTES];
    memcpy(header, MAGIC, MAGIC_LEN);
    for (int s = 0; s < SYMBOLS; s++) put_u32(header + MAGIC_LEN + s * 4, (uint32_t)freq[s]);
    if (fwrite(header, 1, sizeof header, out) != sizeof header) ok = 0;

    BitWriter bw = {out, 0, 0, 0, 0};
    rewind(in);
    while (ok && (c = fgetc(in)) != EOF) {
        const Code *code = &codes[c];
        for (int i = 0; i < code->len; i++) bw_put_bit(&bw, code->bits[i]);
        if (bw.error) ok = 0;
    }
    if (ferror(in)) ok = 0;
    bw_flush(&bw);
    if (bw.error) ok = 0;

    fclose(in);
    if (fclose(out) != 0) ok = 0;
    if (!ok) {
        fprintf(stderr, "Failed while writing \"%s\"; removing the incomplete file.\n", out_path);
        remove(out_path);
        free(tree);
        free(codes);
        return 1;
    }

    unsigned long long compressed = (unsigned long long)HEADER_BYTES + bw.bytes_written;
    printf("Original size  : %llu bytes\n", (unsigned long long)total);
    printf("Compressed size: %llu bytes (includes the %d-byte header)\n", compressed, HEADER_BYTES);
    if (total > 0) printf("Ratio          : %.1f%% of original\n", 100.0 * (double)compressed / (double)total);
    if (compressed >= total) puts("Note: the output is not smaller than the input (typical for small or random data).");
    free(tree);
    free(codes);
    return 0;
}

/* ---------- decompress ---------- */

static int decompress_file(const char *in_path, const char *out_path) {
    FILE *in = fopen(in_path, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open input file \"%s\".\n", in_path);
        return 1;
    }

    unsigned char header[HEADER_BYTES];
    if (fread(header, 1, sizeof header, in) != sizeof header || memcmp(header, MAGIC, MAGIC_LEN) != 0) {
        fprintf(stderr, "\"%s\" is not a valid HUF1 file (bad or missing header).\n", in_path);
        fclose(in);
        return 1;
    }

    uint64_t freq[SYMBOLS];
    uint64_t total = 0;
    for (int s = 0; s < SYMBOLS; s++) {
        freq[s] = get_u32(header + MAGIC_LEN + s * 4);
        total += freq[s];
    }

    Tree *tree = malloc(sizeof *tree);
    if (!tree) {
        fprintf(stderr, "Out of memory.\n");
        fclose(in);
        return 1;
    }
    build_tree(tree, freq);

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file \"%s\".\n", out_path);
        free(tree);
        fclose(in);
        return 1;
    }

    int ok = 1;
    uint64_t remaining = total;

    if (tree->distinct == 1) {
        /* Only one byte value occurs: the stream carries no bits at all. */
        unsigned char chunk[4096];
        memset(chunk, tree->root, sizeof chunk);
        while (remaining > 0 && ok) {
            size_t n = remaining < sizeof chunk ? (size_t)remaining : sizeof chunk;
            if (fwrite(chunk, 1, n, out) != n) ok = 0;
            remaining -= n;
        }
    } else if (tree->distinct > 1) {
        int node = tree->root;
        while (remaining > 0 && ok) {
            int byte = fgetc(in);
            if (byte == EOF) {
                fprintf(stderr, "Compressed data ended early; the file is truncated or corrupt.\n");
                ok = 0;
                break;
            }
            for (int bit = 7; bit >= 0 && remaining > 0; bit--) {
                node = ((byte >> bit) & 1) ? tree->nodes[node].right : tree->nodes[node].left;
                if (tree->nodes[node].left < 0) { /* reached a leaf: its number is the byte value */
                    if (fputc(node, out) == EOF) {
                        ok = 0;
                        break;
                    }
                    remaining--;
                    node = tree->root;
                }
            }
        }
    }

    fclose(in);
    if (fclose(out) != 0) ok = 0;
    free(tree);
    if (!ok) {
        fprintf(stderr, "Decompression failed; removing the incomplete output file.\n");
        remove(out_path);
        return 1;
    }
    printf("Restored %llu bytes to \"%s\".\n", (unsigned long long)total, out_path);
    return 0;
}

/* ---------- main ---------- */

static void usage(const char *prog) {
    fprintf(stderr, "Usage:\n  %s compress   <input> <output>\n  %s decompress <input> <output>\n", prog, prog);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }
    if (strcmp(argv[2], argv[3]) == 0) {
        fprintf(stderr, "Input and output must be different files.\n");
        return 1;
    }
    if (strcmp(argv[1], "compress") == 0 || strcmp(argv[1], "c") == 0) return compress_file(argv[2], argv[3]);
    if (strcmp(argv[1], "decompress") == 0 || strcmp(argv[1], "d") == 0) return decompress_file(argv[2], argv[3]);
    usage(argv[0]);
    return 1;
}

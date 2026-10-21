/*
 * Terminal Text Editor (C11 + POSIX termios)
 *
 * A small full-screen editor that talks to the terminal directly using raw mode
 * and ANSI/VT100 escape sequences:
 *   Arrow keys, Home/End, PageUp/PageDown   move the cursor
 *   Printable ASCII, Enter, Tab             insert text (Tab inserts 4 spaces)
 *   Backspace, Delete                       delete text
 *   Ctrl+S                                  save (asks for a name if needed)
 *   Ctrl+Q                                  quit (press twice if there are unsaved changes)
 *
 * REQUIRES a POSIX terminal: Linux, macOS, or Windows through WSL / a Linux shell.
 * It does not build natively on Windows. Text is treated as single-byte ASCII.
 */
#define _POSIX_C_SOURCE 200809L

#ifdef _WIN32
#error "The Terminal Text Editor needs POSIX termios. Build it on Linux, macOS, or inside WSL."
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define TAB_WIDTH 4
#define STATUS_SECONDS 5

enum Key {
    KEY_BACKSPACE = 127,
    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE
};

typedef struct {
    char *chars;
    size_t len;
} Line;

typedef struct {
    int cx, cy;           /* cursor: column (byte index) and line index */
    int rowoff, coloff;   /* first visible line / column */
    int rows, cols;       /* size of the text area */
    Line *lines;
    int numlines;
    int capacity;
    char *filename;
    int dirty;
    int quit_warned;
    char status[160];
    time_t status_time;
    struct termios orig;
    int raw_enabled;
} Editor;

static Editor E;

/* ---------- terminal handling ---------- */

static void term_write(const char *s, size_t n) {
    ssize_t r = write(STDOUT_FILENO, s, n);
    (void)r; /* nothing sensible can be done if the terminal write fails */
}

static void disable_raw_mode(void) {
    if (E.raw_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig);
        E.raw_enabled = 0;
    }
}

static void fatal(const char *message) {
    term_write("\x1b[2J\x1b[H", 7);
    disable_raw_mode();
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static void enable_raw_mode(void) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "This editor must be run in an interactive terminal.\n");
        exit(1);
    }
    if (tcgetattr(STDIN_FILENO, &E.orig) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    struct termios raw = E.orig;
    raw.c_iflag &= ~(tcflag_t)(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(tcflag_t)(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;  /* read() returns even if no key was pressed ... */
    raw.c_cc[VTIME] = 1; /* ... after 0.1 seconds */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
    E.raw_enabled = 1;
    atexit(disable_raw_mode);
}

static void update_window_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0) {
        E.rows = 22;
        E.cols = 80;
    } else {
        E.rows = (int)ws.ws_row - 2; /* two lines are reserved for the status and message bars */
        E.cols = (int)ws.ws_col;
    }
    if (E.rows < 1) E.rows = 1;
}

static int read_key(void) {
    ssize_t n;
    char c;
    while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN && errno != EINTR) fatal("Error reading from the terminal.");
    }
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': case '7': return KEY_HOME;
                        case '3': return KEY_DELETE;
                        case '4': case '8': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        default: break;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                    default: break;
                }
            }
        } else if (seq[0] == 'O') {
            if (seq[1] == 'H') return KEY_HOME;
            if (seq[1] == 'F') return KEY_END;
        }
        return '\x1b';
    }
    return (unsigned char)c;
}

/* ---------- status message ---------- */

static void set_status(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.status, sizeof E.status, fmt, ap);
    va_end(ap);
    E.status_time = time(NULL);
}

/* ---------- text buffer ---------- */

static void out_of_memory(void) {
    fatal("Out of memory.");
}

static void line_insert(int at, const char *s, size_t len) {
    if (at < 0 || at > E.numlines) return;
    if (E.numlines == E.capacity) {
        int new_cap = E.capacity ? E.capacity * 2 : 32;
        Line *tmp = realloc(E.lines, (size_t)new_cap * sizeof *tmp);
        if (!tmp) out_of_memory();
        E.lines = tmp;
        E.capacity = new_cap;
    }
    memmove(&E.lines[at + 1], &E.lines[at], (size_t)(E.numlines - at) * sizeof(Line));
    E.lines[at].chars = malloc(len + 1);
    if (!E.lines[at].chars) out_of_memory();
    memcpy(E.lines[at].chars, s, len);
    E.lines[at].chars[len] = '\0';
    E.lines[at].len = len;
    E.numlines++;
}

static void line_delete(int at) {
    if (at < 0 || at >= E.numlines) return;
    free(E.lines[at].chars);
    memmove(&E.lines[at], &E.lines[at + 1], (size_t)(E.numlines - at - 1) * sizeof(Line));
    E.numlines--;
}

static void line_insert_char(Line *l, size_t at, int c) {
    if (at > l->len) at = l->len;
    char *tmp = realloc(l->chars, l->len + 2);
    if (!tmp) out_of_memory();
    memmove(&tmp[at + 1], &tmp[at], l->len - at + 1);
    tmp[at] = (char)c;
    l->chars = tmp;
    l->len++;
}

static void line_append(Line *l, const char *s, size_t n) {
    char *tmp = realloc(l->chars, l->len + n + 1);
    if (!tmp) out_of_memory();
    memcpy(&tmp[l->len], s, n);
    l->len += n;
    tmp[l->len] = '\0';
    l->chars = tmp;
}

static void line_delete_char(Line *l, size_t at) {
    if (at >= l->len) return;
    memmove(&l->chars[at], &l->chars[at + 1], l->len - at);
    l->len--;
}

/* ---------- editing operations ---------- */

static void insert_char(int c) {
    if (E.cy == E.numlines) line_insert(E.numlines, "", 0);
    line_insert_char(&E.lines[E.cy], (size_t)E.cx, c);
    E.cx++;
    E.dirty = 1;
}

static void insert_newline(void) {
    if (E.cy == E.numlines) line_insert(E.numlines, "", 0);
    Line *l = &E.lines[E.cy];
    size_t cut = (size_t)E.cx > l->len ? l->len : (size_t)E.cx;
    line_insert(E.cy + 1, l->chars + cut, l->len - cut); /* the tail becomes a new line */
    l = &E.lines[E.cy];                                  /* array may have moved */
    l->len = cut;
    l->chars[cut] = '\0';
    E.cy++;
    E.cx = 0;
    E.dirty = 1;
}

static void delete_before_cursor(void) {
    if (E.cy == E.numlines) { /* on the empty line past the end: just step back */
        if (E.cy > 0) {
            E.cy--;
            E.cx = (int)E.lines[E.cy].len;
        }
        return;
    }
    if (E.cx > 0) {
        line_delete_char(&E.lines[E.cy], (size_t)E.cx - 1);
        E.cx--;
        E.dirty = 1;
    } else if (E.cy > 0) {
        Line *prev = &E.lines[E.cy - 1];
        E.cx = (int)prev->len;
        line_append(prev, E.lines[E.cy].chars, E.lines[E.cy].len);
        line_delete(E.cy);
        E.cy--;
        E.dirty = 1;
    }
}

static void move_cursor(int key) {
    switch (key) {
        case KEY_ARROW_LEFT:
            if (E.cx > 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = (int)E.lines[E.cy].len;
            }
            break;
        case KEY_ARROW_RIGHT:
            if (E.cy < E.numlines) {
                if (E.cx < (int)E.lines[E.cy].len) {
                    E.cx++;
                } else {
                    E.cy++;
                    E.cx = 0;
                }
            }
            break;
        case KEY_ARROW_UP:
            if (E.cy > 0) E.cy--;
            break;
        case KEY_ARROW_DOWN:
            if (E.cy < E.numlines) E.cy++;
            break;
        default:
            break;
    }
    int len = (E.cy < E.numlines) ? (int)E.lines[E.cy].len : 0;
    if (E.cx > len) E.cx = len;
}

static void delete_at_cursor(void) {
    if (E.cy >= E.numlines) return;
    if (E.cy == E.numlines - 1 && E.cx == (int)E.lines[E.cy].len) return; /* end of file */
    move_cursor(KEY_ARROW_RIGHT);
    delete_before_cursor();
}

/* ---------- file I/O ---------- */

static void open_file(const char *path) {
    E.filename = strdup(path);
    if (!E.filename) out_of_memory();
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        if (errno == ENOENT) return; /* a new file: it will be created on save */
        fprintf(stderr, "Cannot open \"%s\": %s\n", path, strerror(errno));
        exit(1);
    }
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    while ((n = getline(&line, &cap, fp)) != -1) {
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) n--;
        line_insert(E.numlines, line, (size_t)n);
    }
    free(line);
    fclose(fp);
    E.dirty = 0;
}

static char *prompt(const char *label);

static void save_file(void) {
    if (!E.filename) {
        E.filename = prompt("Save as: ");
        if (!E.filename) {
            set_status("Save cancelled.");
            return;
        }
    }
    /* Write to a temporary file first so a failed save cannot destroy the original. */
    size_t tmp_len = strlen(E.filename) + 5;
    char *tmp_name = malloc(tmp_len);
    if (!tmp_name) out_of_memory();
    snprintf(tmp_name, tmp_len, "%s.tmp", E.filename);

    FILE *fp = fopen(tmp_name, "wb");
    if (!fp) {
        set_status("Cannot save: %s", strerror(errno));
        free(tmp_name);
        return;
    }
    size_t total = 0;
    int ok = 1;
    for (int i = 0; ok && i < E.numlines; i++) {
        if (fwrite(E.lines[i].chars, 1, E.lines[i].len, fp) != E.lines[i].len || fputc('\n', fp) == EOF) ok = 0;
        total += E.lines[i].len + 1;
    }
    if (fclose(fp) != 0) ok = 0;
    if (ok && rename(tmp_name, E.filename) != 0) ok = 0;
    if (ok) {
        E.dirty = 0;
        set_status("%lu bytes written to %s", (unsigned long)total, E.filename);
    } else {
        set_status("Save failed: %s", strerror(errno));
        remove(tmp_name);
    }
    free(tmp_name);
}

/* ---------- drawing ---------- */

typedef struct {
    char *data;
    size_t len;
} Buffer;

static void buf_append(Buffer *b, const char *s, size_t n) {
    char *tmp = realloc(b->data, b->len + n);
    if (!tmp) out_of_memory();
    memcpy(&tmp[b->len], s, n);
    b->data = tmp;
    b->len += n;
}

static void scroll(void) {
    if (E.cy < E.rowoff) E.rowoff = E.cy;
    if (E.cy >= E.rowoff + E.rows) E.rowoff = E.cy - E.rows + 1;
    if (E.cx < E.coloff) E.coloff = E.cx;
    if (E.cx >= E.coloff + E.cols) E.coloff = E.cx - E.cols + 1;
}

static void draw_rows(Buffer *b) {
    for (int y = 0; y < E.rows; y++) {
        int file_row = y + E.rowoff;
        if (file_row >= E.numlines) {
            if (E.numlines == 0 && y == E.rows / 3) {
                char welcome[80];
                int n = snprintf(welcome, sizeof welcome, "Mini Editor - Ctrl+S save, Ctrl+Q quit");
                if (n > E.cols) n = E.cols;
                int pad = (E.cols - n) / 2;
                if (pad > 0) {
                    buf_append(b, "~", 1);
                    pad--;
                }
                while (pad-- > 0) buf_append(b, " ", 1);
                buf_append(b, welcome, (size_t)n);
            } else {
                buf_append(b, "~", 1);
            }
        } else {
            const Line *l = &E.lines[file_row];
            int len = (int)l->len - E.coloff;
            if (len < 0) len = 0;
            if (len > E.cols) len = E.cols;
            for (int i = 0; i < len; i++) {
                unsigned char ch = (unsigned char)l->chars[E.coloff + i];
                char shown = (ch == '\t') ? '>' : ((ch >= 32 && ch < 127) ? (char)ch : '?');
                buf_append(b, &shown, 1);
            }
        }
        buf_append(b, "\x1b[K\r\n", 5); /* clear to end of line, then next row */
    }
}

static void draw_status_bar(Buffer *b) {
    char left[128], right[64];
    int llen = snprintf(left, sizeof left, "%.40s%s - %d lines", E.filename ? E.filename : "[No Name]",
                        E.dirty ? " (modified)" : "", E.numlines);
    int rlen = snprintf(right, sizeof right, "Ln %d, Col %d", E.cy + 1, E.cx + 1);
    if (llen > E.cols) llen = E.cols;
    buf_append(b, "\x1b[7m", 4); /* inverted colours */
    buf_append(b, left, (size_t)llen);
    while (llen < E.cols) {
        if (E.cols - llen == rlen) {
            buf_append(b, right, (size_t)rlen);
            break;
        }
        buf_append(b, " ", 1);
        llen++;
    }
    buf_append(b, "\x1b[m\r\n", 5);
}

static void draw_message_bar(Buffer *b) {
    buf_append(b, "\x1b[K", 3);
    int len = (int)strlen(E.status);
    if (len > E.cols) len = E.cols;
    if (len > 0 && time(NULL) - E.status_time < STATUS_SECONDS) buf_append(b, E.status, (size_t)len);
}

static void refresh_screen(void) {
    update_window_size();
    scroll();
    Buffer b = {NULL, 0};
    buf_append(&b, "\x1b[?25l\x1b[H", 9); /* hide cursor, move to top-left */
    draw_rows(&b);
    draw_status_bar(&b);
    draw_message_bar(&b);
    char pos[32];
    int n = snprintf(pos, sizeof pos, "\x1b[%d;%dH", E.cy - E.rowoff + 1, E.cx - E.coloff + 1);
    buf_append(&b, pos, (size_t)n);
    buf_append(&b, "\x1b[?25h", 6); /* show cursor */
    term_write(b.data, b.len);
    free(b.data);
}

/* ---------- prompt and key handling ---------- */

static char *prompt(const char *label) {
    size_t cap = 64, len = 0;
    char *buf = malloc(cap);
    if (!buf) out_of_memory();
    buf[0] = '\0';
    for (;;) {
        set_status("%s%s  (Enter = ok, Esc = cancel)", label, buf);
        refresh_screen();
        int c = read_key();
        if (c == KEY_DELETE || c == CTRL_KEY('h') || c == KEY_BACKSPACE) {
            if (len > 0) buf[--len] = '\0';
        } else if (c == '\x1b') {
            set_status("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (len > 0) {
                set_status("");
                return buf;
            }
        } else if (c >= 32 && c < 127) {
            if (len + 1 >= cap) {
                cap *= 2;
                char *tmp = realloc(buf, cap);
                if (!tmp) out_of_memory();
                buf = tmp;
            }
            buf[len++] = (char)c;
            buf[len] = '\0';
        }
    }
}

static void process_key(void) {
    int c = read_key();
    switch (c) {
        case '\r':
            insert_newline();
            break;
        case CTRL_KEY('q'):
            if (E.dirty && !E.quit_warned) {
                set_status("Unsaved changes! Press Ctrl+Q again to quit without saving.");
                E.quit_warned = 1;
                return;
            }
            term_write("\x1b[2J\x1b[H", 7);
            exit(0);
        case CTRL_KEY('s'):
            save_file();
            break;
        case KEY_HOME:
            E.cx = 0;
            break;
        case KEY_END:
            if (E.cy < E.numlines) E.cx = (int)E.lines[E.cy].len;
            break;
        case KEY_BACKSPACE:
        case CTRL_KEY('h'):
            delete_before_cursor();
            break;
        case KEY_DELETE:
            delete_at_cursor();
            break;
        case KEY_PAGE_UP:
        case KEY_PAGE_DOWN: {
            if (c == KEY_PAGE_UP) {
                E.cy = E.rowoff;
            } else {
                E.cy = E.rowoff + E.rows - 1;
                if (E.cy > E.numlines) E.cy = E.numlines;
            }
            for (int times = E.rows; times > 0; times--) {
                move_cursor(c == KEY_PAGE_UP ? KEY_ARROW_UP : KEY_ARROW_DOWN);
            }
            break;
        }
        case KEY_ARROW_UP:
        case KEY_ARROW_DOWN:
        case KEY_ARROW_LEFT:
        case KEY_ARROW_RIGHT:
            move_cursor(c);
            break;
        case '\t':
            for (int i = 0; i < TAB_WIDTH; i++) insert_char(' ');
            break;
        case CTRL_KEY('l'):
        case '\x1b':
            break;
        default:
            if (c >= 32 && c < 127) insert_char(c);
            break; /* other keys (including non-ASCII input) are ignored */
    }
    E.quit_warned = 0;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [file]\n", argv[0]);
        return 1;
    }
    if (argc == 2) open_file(argv[1]); /* done before raw mode so errors print normally */
    enable_raw_mode();
    set_status("Ctrl+S = save | Ctrl+Q = quit");
    for (;;) {
        refresh_screen();
        process_key();
    }
}

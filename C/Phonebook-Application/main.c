/*
 * Phonebook Application (C11)
 *
 * Keeps contacts in a dynamically-sized array that is ALWAYS sorted by name
 * (case-insensitive). Because the array is sorted we can:
 *   - find a contact with binary search (O(log n)),
 *   - find every contact whose name starts with a prefix quickly,
 *   - insert new entries at the correct position with memmove.
 *
 * Data is stored in a tab-separated text file, "phonebook.tsv", written via a
 * temporary file so a failed save does not destroy the previous data.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_CAP 50
#define PHONE_CAP 21
#define EMAIL_CAP 61
#define LINE_CAP 256
#define DATA_FILE "phonebook.tsv"
#define TEMP_FILE "phonebook.tsv.tmp"
#define FILE_HEADER "PHONEBOOK-V1"

typedef struct {
    char name[NAME_CAP];
    char phone[PHONE_CAP];
    char email[EMAIL_CAP];
} Entry;

typedef struct {
    Entry *items;
    size_t count;
    size_t capacity;
    int dirty; /* 1 when there are unsaved changes */
} Phonebook;

typedef enum { FIELD_NAME, FIELD_PHONE, FIELD_EMAIL } FieldKind;
typedef enum { MODE_REQUIRED, MODE_OPTIONAL, MODE_KEEP } FieldMode;

/* ---------- small string helpers ---------- */

static int compare_ci(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static int starts_with_ci(const char *text, const char *prefix) {
    while (*prefix) {
        if (*text == '\0' || tolower((unsigned char)*text) != tolower((unsigned char)*prefix)) return 0;
        text++;
        prefix++;
    }
    return 1;
}

static int contains_ci(const char *text, const char *needle) {
    for (; *text; text++) {
        if (starts_with_ci(text, needle)) return 1;
    }
    return *needle == '\0';
}

static void trim(char *s) {
    size_t len = strlen(s);
    size_t start = 0;
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) memmove(s, s + start, strlen(s + start) + 1);
}

/* Returns 1 = ok, 0 = EOF, -1 = line was too long (rest discarded). */
static int read_line(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        int ch, overflow = 0;
        while ((ch = getchar()) != '\n' && ch != EOF) overflow = 1;
        if (overflow) return -1;
    }
    if (len > 0 && buf[len - 1] == '\r') buf[len - 1] = '\0';
    return 1;
}

/* ---------- field validation ---------- */

static int has_control_chars(const char *s) {
    for (; *s; s++) {
        if (iscntrl((unsigned char)*s)) return 1;
    }
    return 0;
}

static int valid_phone(const char *s) {
    int digits = 0;
    for (; *s; s++) {
        if (isdigit((unsigned char)*s)) digits++;
        else if (!strchr(" +-()", *s)) return 0;
    }
    return digits >= 5 && digits <= 15;
}

static int valid_email(const char *s) {
    const char *at = strchr(s, '@');
    if (!at || at == s || strchr(at + 1, '@')) return 0;
    if (strchr(s, ' ')) return 0;
    const char *dot = strchr(at + 1, '.');
    return dot && dot > at + 1 && dot[1] != '\0';
}

static int field_valid(FieldKind kind, const char *s) {
    if (has_control_chars(s)) return 0;
    switch (kind) {
        case FIELD_NAME: return 1;
        case FIELD_PHONE: return valid_phone(s);
        case FIELD_EMAIL: return valid_email(s);
    }
    return 0;
}

static const char *field_hint(FieldKind kind) {
    switch (kind) {
        case FIELD_NAME: return "Names cannot contain control characters.";
        case FIELD_PHONE: return "Use 5-15 digits; spaces, +, -, ( and ) are allowed.";
        case FIELD_EMAIL: return "Enter an address like name@example.com.";
    }
    return "Invalid input.";
}

/*
 * Prompts until a valid value is entered. Returns 1 on success, 0 on EOF.
 * MODE_OPTIONAL: blank accepted (stored as ""). MODE_KEEP: blank accepted and
 * the single character "-" is also accepted (the caller interprets both).
 */
static int ask_field(const char *label, FieldKind kind, FieldMode mode, char *out, size_t cap) {
    char line[LINE_CAP];
    for (;;) {
        printf("%s: ", label);
        int r = read_line(line, sizeof line);
        if (r == 0) return 0;
        if (r < 0) {
            puts("Input too long. Please try again.");
            continue;
        }
        trim(line);
        if (line[0] == '\0') {
            if (mode == MODE_REQUIRED) {
                puts("This field is required.");
                continue;
            }
            out[0] = '\0';
            return 1;
        }
        if (mode == MODE_KEEP && strcmp(line, "-") == 0) {
            snprintf(out, cap, "-");
            return 1;
        }
        if (strlen(line) >= cap) {
            printf("Maximum length is %lu characters.\n", (unsigned long)(cap - 1));
            continue;
        }
        if (!field_valid(kind, line)) {
            puts(field_hint(kind));
            continue;
        }
        snprintf(out, cap, "%s", line);
        return 1;
    }
}

/* ---------- phonebook data structure ---------- */

static void pb_free(Phonebook *pb) {
    free(pb->items);
    pb->items = NULL;
    pb->count = pb->capacity = 0;
}

/* First index whose name is >= `name` (binary search over the sorted array). */
static size_t lower_bound(const Phonebook *pb, const char *name) {
    size_t lo = 0, hi = pb->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (compare_ci(pb->items[mid].name, name) < 0) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static int pb_find(const Phonebook *pb, const char *name, size_t *index) {
    size_t pos = lower_bound(pb, name);
    if (pos < pb->count && compare_ci(pb->items[pos].name, name) == 0) {
        *index = pos;
        return 1;
    }
    return 0;
}

/* Returns 0 on success, -1 if the name already exists, -2 if out of memory. */
static int pb_add(Phonebook *pb, const Entry *entry) {
    size_t pos = lower_bound(pb, entry->name);
    if (pos < pb->count && compare_ci(pb->items[pos].name, entry->name) == 0) return -1;
    if (pb->count == pb->capacity) {
        size_t new_cap = pb->capacity ? pb->capacity * 2 : 8;
        Entry *tmp = realloc(pb->items, new_cap * sizeof *tmp);
        if (!tmp) return -2;
        pb->items = tmp;
        pb->capacity = new_cap;
    }
    memmove(&pb->items[pos + 1], &pb->items[pos], (pb->count - pos) * sizeof(Entry));
    pb->items[pos] = *entry;
    pb->count++;
    pb->dirty = 1;
    return 0;
}

static void pb_remove(Phonebook *pb, size_t index) {
    memmove(&pb->items[index], &pb->items[index + 1], (pb->count - index - 1) * sizeof(Entry));
    pb->count--;
    pb->dirty = 1;
}

/* ---------- persistence ---------- */

static int pb_save(Phonebook *pb) {
    FILE *fp = fopen(TEMP_FILE, "w");
    if (!fp) {
        perror("Cannot create temporary file");
        return 0;
    }
    int ok = fprintf(fp, "%s\n", FILE_HEADER) > 0;
    for (size_t i = 0; ok && i < pb->count; i++) {
        ok = fprintf(fp, "%s\t%s\t%s\n", pb->items[i].name, pb->items[i].phone, pb->items[i].email) > 0;
    }
    if (fclose(fp) != 0) ok = 0;
    if (!ok) {
        fprintf(stderr, "Error while writing %s; previous data file left untouched.\n", TEMP_FILE);
        remove(TEMP_FILE);
        return 0;
    }
    /* On Windows rename() fails if the target exists, so remove it first. */
    remove(DATA_FILE);
    if (rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Cannot replace data file");
        fprintf(stderr, "Your data is still in %s.\n", TEMP_FILE);
        return 0;
    }
    pb->dirty = 0;
    printf("Saved %lu contact(s) to %s.\n", (unsigned long)pb->count, DATA_FILE);
    return 1;
}

static void pb_load(Phonebook *pb) {
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp) {
        puts("No saved phonebook found; starting with an empty one.");
        return;
    }
    char line[LINE_CAP];
    unsigned long skipped = 0;
    if (!fgets(line, sizeof line, fp) || strncmp(line, FILE_HEADER, strlen(FILE_HEADER)) != 0) {
        fprintf(stderr, "%s is not a valid phonebook file (missing header). Not loaded.\n", DATA_FILE);
        fclose(fp);
        return;
    }
    while (fgets(line, sizeof line, fp)) {
        size_t len = strlen(line);
        if (len == sizeof line - 1 && line[len - 1] != '\n') {
            int ch;
            while ((ch = fgetc(fp)) != '\n' && ch != EOF) {}
            skipped++;
            continue;
        }
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;

        char *name = line;
        char *phone = strchr(name, '\t');
        char *email = phone ? strchr(phone + 1, '\t') : NULL;
        if (!phone || !email) {
            skipped++;
            continue;
        }
        *phone++ = '\0';
        *email++ = '\0';
        if (strlen(name) >= NAME_CAP || strlen(phone) >= PHONE_CAP || strlen(email) >= EMAIL_CAP ||
            name[0] == '\0' || !valid_phone(phone) || (email[0] != '\0' && !valid_email(email)) ||
            strchr(email, '\t') || has_control_chars(name)) {
            skipped++;
            continue;
        }
        Entry e;
        snprintf(e.name, sizeof e.name, "%s", name);
        snprintf(e.phone, sizeof e.phone, "%s", phone);
        snprintf(e.email, sizeof e.email, "%s", email);
        if (pb_add(pb, &e) != 0) skipped++;
    }
    fclose(fp);
    pb->dirty = 0;
    printf("Loaded %lu contact(s).\n", (unsigned long)pb->count);
    if (skipped > 0) printf("Warning: skipped %lu malformed or duplicate line(s).\n", skipped);
}

/* ---------- user-facing actions ---------- */

static void print_entry(size_t number, const Entry *e) {
    printf("%-4lu %-30s %-20s %s\n", (unsigned long)number, e->name, e->phone, e->email);
}

static void print_header(void) {
    printf("%-4s %-30s %-20s %s\n", "No.", "Name", "Phone", "Email");
}

static void action_add(Phonebook *pb) {
    Entry e;
    if (!ask_field("Name", FIELD_NAME, MODE_REQUIRED, e.name, sizeof e.name)) return;
    size_t existing;
    if (pb_find(pb, e.name, &existing)) {
        puts("A contact with that name already exists. Use Edit to change it.");
        return;
    }
    if (!ask_field("Phone", FIELD_PHONE, MODE_REQUIRED, e.phone, sizeof e.phone)) return;
    if (!ask_field("Email (optional)", FIELD_EMAIL, MODE_OPTIONAL, e.email, sizeof e.email)) return;
    int r = pb_add(pb, &e);
    if (r == 0) puts("Contact added.");
    else if (r == -1) puts("A contact with that name already exists.");
    else puts("Out of memory; contact not added.");
}

static void action_list(const Phonebook *pb) {
    if (pb->count == 0) {
        puts("The phonebook is empty.");
        return;
    }
    print_header();
    for (size_t i = 0; i < pb->count; i++) print_entry(i + 1, &pb->items[i]);
}

static int cmp_phone(const void *a, const void *b) {
    const Entry *x = a, *y = b;
    int c = strcmp(x->phone, y->phone);
    return c != 0 ? c : compare_ci(x->name, y->name);
}

static void action_list_by_phone(const Phonebook *pb) {
    if (pb->count == 0) {
        puts("The phonebook is empty.");
        return;
    }
    Entry *copy = malloc(pb->count * sizeof *copy);
    if (!copy) {
        puts("Out of memory.");
        return;
    }
    memcpy(copy, pb->items, pb->count * sizeof *copy);
    qsort(copy, pb->count, sizeof *copy, cmp_phone);
    print_header();
    for (size_t i = 0; i < pb->count; i++) print_entry(i + 1, &copy[i]);
    free(copy);
}

static void action_lookup(const Phonebook *pb) {
    char name[NAME_CAP];
    if (!ask_field("Exact name", FIELD_NAME, MODE_REQUIRED, name, sizeof name)) return;
    size_t idx;
    if (!pb_find(pb, name, &idx)) {
        puts("No contact with that name.");
        return;
    }
    print_header();
    print_entry(idx + 1, &pb->items[idx]);
}

static void action_prefix(const Phonebook *pb) {
    char prefix[NAME_CAP];
    if (!ask_field("Name starts with", FIELD_NAME, MODE_REQUIRED, prefix, sizeof prefix)) return;
    size_t i = lower_bound(pb, prefix);
    size_t shown = 0;
    for (; i < pb->count && starts_with_ci(pb->items[i].name, prefix); i++) {
        if (shown == 0) print_header();
        print_entry(i + 1, &pb->items[i]);
        shown++;
    }
    if (shown == 0) puts("No matching contacts.");
}

static void action_search(const Phonebook *pb) {
    char term[NAME_CAP];
    if (!ask_field("Search text (name, phone or email)", FIELD_NAME, MODE_REQUIRED, term, sizeof term)) return;
    size_t shown = 0;
    for (size_t i = 0; i < pb->count; i++) {
        const Entry *e = &pb->items[i];
        if (contains_ci(e->name, term) || contains_ci(e->phone, term) || contains_ci(e->email, term)) {
            if (shown == 0) print_header();
            print_entry(i + 1, e);
            shown++;
        }
    }
    if (shown == 0) puts("No matching contacts.");
}

static void action_edit(Phonebook *pb) {
    char name[NAME_CAP];
    if (!ask_field("Name of contact to edit", FIELD_NAME, MODE_REQUIRED, name, sizeof name)) return;
    size_t idx;
    if (!pb_find(pb, name, &idx)) {
        puts("No contact with that name.");
        return;
    }
    Entry old = pb->items[idx];
    Entry updated = old;
    char new_name[NAME_CAP], new_phone[PHONE_CAP], new_email[EMAIL_CAP];

    puts("Leave a field blank to keep its current value.");
    printf("Current name: %s\n", old.name);
    if (!ask_field("New name", FIELD_NAME, MODE_OPTIONAL, new_name, sizeof new_name)) return;
    printf("Current phone: %s\n", old.phone);
    if (!ask_field("New phone", FIELD_PHONE, MODE_OPTIONAL, new_phone, sizeof new_phone)) return;
    printf("Current email: %s\n", old.email[0] ? old.email : "(none)");
    if (!ask_field("New email (- to clear)", FIELD_EMAIL, MODE_KEEP, new_email, sizeof new_email)) return;

    if (new_phone[0]) snprintf(updated.phone, sizeof updated.phone, "%s", new_phone);
    if (strcmp(new_email, "-") == 0) updated.email[0] = '\0';
    else if (new_email[0]) snprintf(updated.email, sizeof updated.email, "%s", new_email);

    int renamed = new_name[0] && strcmp(new_name, old.name) != 0;
    if (renamed && compare_ci(new_name, old.name) != 0) {
        size_t clash;
        if (pb_find(pb, new_name, &clash)) {
            puts("Another contact already uses that name; nothing changed.");
            return;
        }
        snprintf(updated.name, sizeof updated.name, "%s", new_name);
        /* Add the new entry first so a failure cannot lose the old one. */
        if (pb_add(pb, &updated) != 0) {
            puts("Could not store the updated contact; nothing changed.");
            return;
        }
        size_t old_idx;
        if (pb_find(pb, old.name, &old_idx)) pb_remove(pb, old_idx);
    } else {
        /* Same name (or only the letter case changed): the position is unchanged. */
        if (renamed) snprintf(updated.name, sizeof updated.name, "%s", new_name);
        pb->items[idx] = updated;
        pb->dirty = 1;
    }
    puts("Contact updated.");
}

static void action_delete(Phonebook *pb) {
    char name[NAME_CAP];
    if (!ask_field("Name of contact to delete", FIELD_NAME, MODE_REQUIRED, name, sizeof name)) return;
    size_t idx;
    if (!pb_find(pb, name, &idx)) {
        puts("No contact with that name.");
        return;
    }
    char answer[LINE_CAP];
    printf("Delete \"%s\"? (y/n): ", pb->items[idx].name);
    if (read_line(answer, sizeof answer) <= 0) return;
    if (tolower((unsigned char)answer[0]) == 'y') {
        pb_remove(pb, idx);
        puts("Contact deleted.");
    } else {
        puts("Cancelled.");
    }
}

static int confirm_quit(Phonebook *pb) {
    if (!pb->dirty) return 1;
    char answer[LINE_CAP];
    printf("You have unsaved changes. Save before quitting? (y/n/c=cancel): ");
    int r = read_line(answer, sizeof answer);
    if (r <= 0) {
        puts("\nInput closed; unsaved changes were discarded.");
        return 1;
    }
    char c = (char)tolower((unsigned char)answer[0]);
    if (c == 'c') return 0;
    if (c == 'y') return pb_save(pb);
    return 1;
}

static void print_menu(void) {
    puts("\n=== Phonebook ===");
    puts("1. Add contact");
    puts("2. List all (sorted by name)");
    puts("3. List all (sorted by phone)");
    puts("4. Look up by exact name");
    puts("5. Find names starting with...");
    puts("6. Search anywhere (name/phone/email)");
    puts("7. Edit contact");
    puts("8. Delete contact");
    puts("9. Save");
    puts("0. Quit");
    printf("Choose an option: ");
}

int main(void) {
    Phonebook pb = {NULL, 0, 0, 0};
    pb_load(&pb);

    for (;;) {
        char line[LINE_CAP];
        print_menu();
        int r = read_line(line, sizeof line);
        if (r == 0) {
            if (pb.dirty) puts("\nInput closed; unsaved changes were discarded.");
            break;
        }
        if (r < 0) {
            puts("Input too long.");
            continue;
        }
        char *end;
        long choice = strtol(line, &end, 10);
        if (end == line || *end != '\0') {
            puts("Please enter a number from the menu.");
            continue;
        }
        switch (choice) {
            case 1: action_add(&pb); break;
            case 2: action_list(&pb); break;
            case 3: action_list_by_phone(&pb); break;
            case 4: action_lookup(&pb); break;
            case 5: action_prefix(&pb); break;
            case 6: action_search(&pb); break;
            case 7: action_edit(&pb); break;
            case 8: action_delete(&pb); break;
            case 9: pb_save(&pb); break;
            case 0:
                if (confirm_quit(&pb)) {
                    pb_free(&pb);
                    puts("Goodbye!");
                    return 0;
                }
                break;
            default: puts("Unknown option."); break;
        }
    }
    pb_free(&pb);
    return 0;
}

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "expenses.csv"
#define DATE_CAP 11
#define CATEGORY_CAP 40
#define DESCRIPTION_CAP 100
#define LINE_CAP 320

typedef struct {
    int id;
    char date[DATE_CAP];
    char category[CATEGORY_CAP];
    char description[DESCRIPTION_CAP];
    double amount;
} Expense;

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    size_t n = strlen(buffer);
    if (n && buffer[n - 1] == '\n') {
        trim_newline(buffer);
        return 1;
    }
    if (feof(stdin)) {
        trim_newline(buffer);
        return 1;
    }
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
    puts("Input too long. Please try again.");
    return -1;
}

static int parse_positive_int(const char *s, int *out) {
    char *end;
    long value;
    while (isspace((unsigned char)*s)) ++s;
    if (!*s) return 0;
    errno = 0;
    value = strtol(s, &end, 10);
    if (s == end || errno == ERANGE || value < 1 || value > 2147483647L) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end) return 0;
    *out = (int)value;
    return 1;
}

static int valid_text(const char *s, size_t cap) {
    if (!s[0] || strlen(s) >= cap || strchr(s, ',') != NULL) return 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p)
        if (*p < 32 || *p == 127) return 0;
    return 1;
}

static int valid_date(const char *s) {
    if (strlen(s) != 10 || s[4] != '-' || s[7] != '-') return 0;
    for (int i = 0; i < 10; ++i)
        if (i != 4 && i != 7 && !isdigit((unsigned char)s[i])) return 0;
    int month = (s[5] - '0') * 10 + s[6] - '0';
    int day = (s[8] - '0') * 10 + s[9] - '0';
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

static int parse_expense(const char *line, Expense *e) {
    int consumed = 0;
    if (sscanf(line, "%d,%10[^,],%39[^,],%99[^,],%lf%n",
               &e->id, e->date, e->category, e->description, &e->amount, &consumed) != 5)
        return 0;
    while (line[consumed] == '\r' || line[consumed] == '\n' || line[consumed] == ' ') ++consumed;
    return line[consumed] == '\0' && e->id > 0 && valid_date(e->date) &&
           isfinite(e->amount) && e->amount > 0.0;
}

static int next_id(FILE *file) {
    char line[LINE_CAP];
    int highest = 0;
    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        Expense e;
        if (parse_expense(line, &e) && e.id > highest) highest = e.id;
    }
    if (ferror(file) || highest == 2147483647) return 0;
    return highest + 1;
}

static int add_expense(void) {
    Expense e;
    char text[LINE_CAP];
    int status;

    FILE *file = fopen(DATA_FILE, "a+");
    if (!file) { perror("Could not open expenses file"); return 1; }
    e.id = next_id(file);
    if (!e.id) { puts("Could not allocate an expense ID (or file read failed)."); fclose(file); return 1; }
    printf("New expense ID: %d\n", e.id);

    status = read_line("Date (YYYY-MM-DD): ", e.date, sizeof(e.date));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (!valid_date(e.date)) { puts("Enter a date in YYYY-MM-DD format with a valid month/day range."); fclose(file); return 1; }

    status = read_line("Category (e.g. Food, Travel): ", e.category, sizeof(e.category));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (!valid_text(e.category, sizeof(e.category))) { puts("Category is required, must be under 40 characters, and cannot contain commas."); fclose(file); return 1; }

    status = read_line("Description: ", e.description, sizeof(e.description));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (!valid_text(e.description, sizeof(e.description))) { puts("Description is required, must be under 100 characters, and cannot contain commas."); fclose(file); return 1; }

    status = read_line("Amount (positive number): ", text, sizeof(text));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    char *end;
    errno = 0;
    e.amount = strtod(text, &end);
    while (isspace((unsigned char)*end)) ++end;
    if (text == end || *end || errno == ERANGE || !isfinite(e.amount) || e.amount <= 0.0) {
        puts("Enter a valid finite amount greater than zero."); fclose(file); return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0 ||
        fprintf(file, "%d,%s,%s,%s,%.2f\n", e.id, e.date, e.category, e.description, e.amount) < 0 ||
        fflush(file) != 0) {
        perror("Could not save expense"); fclose(file); return 1;
    }
    if (fclose(file) != 0) { perror("Could not close expenses file"); return 1; }
    puts("Expense saved successfully.");
    return 1;
}

static void list_expenses(void) {
    FILE *file = fopen(DATA_FILE, "r");
    char line[LINE_CAP];
    int count = 0;
    double total = 0.0;
    if (!file) {
        if (errno == ENOENT) puts("No expenses saved yet.");
        else perror("Could not open expenses file");
        return;
    }
    puts("\nID   DATE       CATEGORY                 AMOUNT       DESCRIPTION");
    puts("--------------------------------------------------------------------------");
    while (fgets(line, sizeof(line), file)) {
        Expense e;
        if (!parse_expense(line, &e)) {
            fprintf(stderr, "Warning: skipped malformed record in %s.\n", DATA_FILE);
            continue;
        }
        printf("%-4d %-10s %-24s %10.2f   %s\n", e.id, e.date, e.category, e.amount, e.description);
        total += e.amount;
        ++count;
    }
    if (ferror(file)) perror("Error reading expenses file");
    fclose(file);
    if (count == 0) puts("No valid expense records found.");
    printf("Records: %d | Total: %.2f\n", count, total);
}

static void show_summary(void) {
    FILE *file = fopen(DATA_FILE, "r");
    char line[LINE_CAP];
    double total = 0.0;
    int count = 0;
    if (!file) {
        if (errno == ENOENT) puts("No expenses saved yet.");
        else perror("Could not open expenses file");
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        Expense e;
        if (parse_expense(line, &e)) {
            total += e.amount;
            ++count;
        } else {
            fprintf(stderr, "Warning: skipped malformed record in %s.\n", DATA_FILE);
        }
    }
    if (ferror(file)) perror("Error reading expenses file");
    fclose(file);
    puts("\n========== EXPENSE SUMMARY ==========");
    printf("Number of expenses: %d\n", count);
    printf("Total spending:     %.2f\n", total);
    printf("Average expense:    %.2f\n", count ? total / count : 0.0);
    puts("=====================================");
}

int main(void) {
    char line[LINE_CAP];
    puts("================================");
    puts("          EXPENSE TRACKER");
    puts("================================");
    for (;;) {
        int choice, status;
        puts("\n1. Add expense");
        puts("2. List expenses");
        puts("3. View summary");
        puts("4. Exit");
        status = read_line("Choose an option: ", line, sizeof(line));
        if (status == 0) { puts("\nInput closed. Goodbye!"); break; }
        if (status < 0 || !parse_positive_int(line, &choice) || choice > 4) {
            puts("Choose a number from 1 to 4."); continue;
        }
        if (choice == 1) { if (add_expense() == 0) break; }
        else if (choice == 2) list_expenses();
        else if (choice == 3) show_summary();
        else { puts("Goodbye!"); break; }
    }
    return 0;
}

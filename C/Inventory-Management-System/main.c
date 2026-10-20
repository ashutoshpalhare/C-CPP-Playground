#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "inventory.csv"
#define TEMP_FILE "inventory.tmp"
#define NAME_CAP 80
#define CATEGORY_CAP 40
#define LINE_CAP 240

typedef struct {
    int id;
    char name[NAME_CAP];
    char category[CATEGORY_CAP];
    int quantity;
    double price;
} Item;

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

static int parse_int(const char *s, int *out, int allow_zero) {
    char *end;
    long value;
    while (isspace((unsigned char)*s)) ++s;
    if (!*s) return 0;
    errno = 0;
    value = strtol(s, &end, 10);
    if (s == end || errno == ERANGE || value < (allow_zero ? 0 : 1) || value > INT_MAX) return 0;
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

static int parse_item(const char *line, Item *item) {
    int used = 0;
    if (sscanf(line, "%d,%79[^,],%39[^,],%d,%lf%n",
               &item->id, item->name, item->category, &item->quantity, &item->price, &used) != 5)
        return 0;
    while (line[used] == '\r' || line[used] == '\n' || line[used] == ' ') ++used;
    return line[used] == '\0' && item->id > 0 && item->quantity >= 0 &&
           item->price >= 0.0 && item->price <= 1.0e12;
}

static int find_item(FILE *file, int id, Item *result) {
    char line[LINE_CAP];
    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        Item item;
        if (parse_item(line, &item) && item.id == id) {
            if (result) *result = item;
            return 1;
        }
    }
    return 0;
}

static int next_id(FILE *file) {
    char line[LINE_CAP];
    int highest = 0;
    rewind(file);
    while (fgets(line, sizeof(line), file)) {
        Item item;
        if (parse_item(line, &item) && item.id > highest) highest = item.id;
    }
    if (ferror(file) || highest == INT_MAX) return 0;
    return highest + 1;
}

static int get_item_fields(Item *item, int is_new) {
    char text[LINE_CAP];
    int status;
    if (is_new) {
        status = read_line("Item name: ", item->name, sizeof(item->name));
        if (status <= 0) return status;
        if (!valid_text(item->name, sizeof(item->name))) { puts("Name is required, under 80 characters, and cannot contain commas."); return -1; }
    }
    status = read_line("Category: ", item->category, sizeof(item->category));
    if (status <= 0) return status;
    if (!valid_text(item->category, sizeof(item->category))) { puts("Category is required, under 40 characters, and cannot contain commas."); return -1; }

    status = read_line("Quantity (whole number, zero or more): ", text, sizeof(text));
    if (status <= 0) return status;
    if (!parse_int(text, &item->quantity, 1)) { puts("Enter a whole-number quantity of zero or more."); return -1; }

    status = read_line("Unit price (zero or more): ", text, sizeof(text));
    if (status <= 0) return status;
    char *end;
    errno = 0;
    item->price = strtod(text, &end);
    while (isspace((unsigned char)*end)) ++end;
    if (text == end || *end || errno == ERANGE || item->price < 0.0 ||
        item->price > 1.0e12 || item->price != item->price) {
        puts("Enter a valid non-negative unit price (maximum 1,000,000,000,000).");
        return -1;
    }
    return 1;
}

static int add_item(void) {
    FILE *file = fopen(DATA_FILE, "a+");
    if (!file) { perror("Could not open inventory file"); return 1; }
    Item item;
    item.id = next_id(file);
    if (!item.id) { puts("Cannot allocate another item ID or inventory file could not be read."); fclose(file); return 1; }
    printf("New item ID: %d\n", item.id);
    int status = get_item_fields(&item, 1);
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (fseek(file, 0, SEEK_END) != 0 ||
        fprintf(file, "%d,%s,%s,%d,%.2f\n", item.id, item.name, item.category, item.quantity, item.price) < 0 ||
        fflush(file) != 0) {
        perror("Could not save item"); fclose(file); return 1;
    }
    if (fclose(file) != 0) { perror("Could not close inventory file"); return 1; }
    puts("Item added successfully.");
    return 1;
}

static void list_items(void) {
    FILE *file = fopen(DATA_FILE, "r");
    char line[LINE_CAP];
    int count = 0;
    double value = 0.0;
    if (!file) {
        if (errno == ENOENT) puts("Inventory is empty.");
        else perror("Could not open inventory file");
        return;
    }
    puts("\nID   ITEM                                     CATEGORY               QTY       PRICE       STOCK VALUE");
    puts("-----------------------------------------------------------------------------------------------------");
    while (fgets(line, sizeof(line), file)) {
        Item item;
        if (!parse_item(line, &item)) { fprintf(stderr, "Warning: skipped malformed inventory record.\n"); continue; }
        double stock = item.quantity * item.price;
        printf("%-4d %-40s %-22s %-9d %-11.2f %.2f\n",
               item.id, item.name, item.category, item.quantity, item.price, stock);
        value += stock;
        ++count;
    }
    if (ferror(file)) perror("Error reading inventory file");
    fclose(file);
    if (!count) puts("No valid inventory items found.");
    printf("Items: %d | Estimated stock value: %.2f\n", count, value);
}

static int update_quantity(void) {
    char text[LINE_CAP], line[LINE_CAP];
    int id, quantity, status, found = 0, failed = 0;
    status = read_line("Item ID to update: ", text, sizeof(text));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!parse_int(text, &id, 0) || id == 0) { puts("Invalid item ID."); return 1; }
    status = read_line("New quantity (zero or more): ", text, sizeof(text));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!parse_int(text, &quantity, 1)) { puts("Enter a whole-number quantity of zero or more."); return 1; }

    FILE *source = fopen(DATA_FILE, "r");
    if (!source) { if (errno == ENOENT) puts("Inventory is empty."); else perror("Could not open inventory file"); return 1; }
    FILE *temp = fopen(TEMP_FILE, "w");
    if (!temp) { perror("Could not create temporary file"); fclose(source); return 1; }
    while (fgets(line, sizeof(line), source)) {
        Item item;
        if (parse_item(line, &item) && item.id == id) {
            item.quantity = quantity;
            if (fprintf(temp, "%d,%s,%s,%d,%.2f\n", item.id, item.name, item.category, item.quantity, item.price) < 0) failed = 1;
            found = 1;
        } else if (fputs(line, temp) == EOF) failed = 1;
    }
    if (ferror(source) || fflush(temp) != 0) failed = 1;
    if (fclose(source) != 0) failed = 1;
    if (fclose(temp) != 0) failed = 1;
    if (failed) { perror("Could not write temporary inventory file"); remove(TEMP_FILE); return 1; }
    if (!found) { puts("Item ID not found."); remove(TEMP_FILE); return 1; }
    if (remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Could not finish update; check inventory.csv and inventory.tmp");
        return 1;
    }
    puts("Quantity updated successfully.");
    return 1;
}

static int search_item(void) {
    char query[LINE_CAP], line[LINE_CAP];
    int status, found = 0;
    status = read_line("Search item name or category: ", query, sizeof(query));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!query[0]) { puts("Search cannot be empty."); return 1; }
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) { if (errno == ENOENT) puts("Inventory is empty."); else perror("Could not open inventory file"); return 1; }
    while (fgets(line, sizeof(line), file)) {
        Item item;
        if (parse_item(line, &item) &&
            (strstr(item.name, query) || strstr(item.category, query))) {
            printf("ID: %d | %s | Category: %s | Quantity: %d | Unit price: %.2f\n",
                   item.id, item.name, item.category, item.quantity, item.price);
            found = 1;
        }
    }
    if (ferror(file)) perror("Error searching inventory file");
    fclose(file);
    if (!found) puts("No matching items found (search is case-sensitive).");
    return 1;
}

int main(void) {
    char input[LINE_CAP];
    puts("======================================");
    puts("       INVENTORY MANAGEMENT SYSTEM");
    puts("======================================");
    for (;;) {
        int choice, status;
        puts("\n1. Add item");
        puts("2. List inventory");
        puts("3. Update item quantity");
        puts("4. Search items");
        puts("5. Exit");
        status = read_line("Choose an option: ", input, sizeof(input));
        if (status == 0) { puts("\nInput closed. Goodbye!"); break; }
        if (status < 0 || !parse_int(input, &choice, 0) || choice < 1 || choice > 5) {
            puts("Choose a number from 1 to 5."); continue;
        }
        if (choice == 1) { if (add_item() == 0) break; }
        else if (choice == 2) list_items();
        else if (choice == 3) { if (update_quantity() == 0) break; }
        else if (choice == 4) { if (search_item() == 0) break; }
        else { puts("Goodbye!"); break; }
    }
    return 0;
}

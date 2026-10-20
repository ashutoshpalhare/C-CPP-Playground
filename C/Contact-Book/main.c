#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "contacts.csv"
#define TEMP_FILE "contacts.tmp"
#define NAME_CAP 80
#define PHONE_CAP 32
#define EMAIL_CAP 100
#define LINE_CAP 320

typedef struct {
    int id;
    char name[NAME_CAP];
    char phone[PHONE_CAP];
    char email[EMAIL_CAP];
} Contact;

static void trim_newline(char *text) {
    size_t n = strlen(text);
    while (n > 0 && (text[n - 1] == '\n' || text[n - 1] == '\r')) text[--n] = '\0';
}

static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    size_t n = strlen(buffer);
    if (n > 0 && buffer[n - 1] == '\n') {
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

static int parse_positive_int(const char *text, int *out) {
    char *end;
    long value;
    while (isspace((unsigned char)*text)) ++text;
    if (*text == '\0') return 0;
    errno = 0;
    value = strtol(text, &end, 10);
    if (text == end || errno == ERANGE || value < 1 || value > INT_MAX) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return 0;
    *out = (int)value;
    return 1;
}

static int valid_field(const char *text, size_t capacity) {
    if (text[0] == '\0' || strlen(text) >= capacity || strchr(text, ',') != NULL) return 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (*p < 32 || *p == 127) return 0;
    }
    return 1;
}

static int parse_record(const char *line, Contact *contact) {
    char extra;
    return sscanf(line, "%d,%79[^,],%31[^,],%99[^,]%c",
                  &contact->id, contact->name, contact->phone, contact->email, &extra) == 4
           && contact->id > 0;
}

static int find_contact(FILE *file, int id, Contact *found) {
    char line[LINE_CAP];
    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        Contact contact;
        if (parse_record(line, &contact) && contact.id == id) {
            if (found != NULL) *found = contact;
            return 1;
        }
    }
    return 0;
}

static int next_id(FILE *file) {
    char line[LINE_CAP];
    int highest = 0;
    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        Contact contact;
        if (parse_record(line, &contact) && contact.id > highest) highest = contact.id;
    }
    return highest == INT_MAX ? 0 : highest + 1;
}

static int add_contact(void) {
    Contact contact;
    int status;
    char id_text[LINE_CAP];

    FILE *file = fopen(DATA_FILE, "a+");
    if (file == NULL) {
        perror("Could not open contacts file");
        return 1;
    }
    contact.id = next_id(file);
    if (contact.id == 0) {
        puts("Cannot allocate another contact ID.");
        fclose(file);
        return 1;
    }
    printf("New contact ID: %d\n", contact.id);
    status = read_line("Name: ", contact.name, sizeof(contact.name));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (!valid_field(contact.name, sizeof(contact.name))) {
        puts("Name must be non-empty, under 80 characters, and contain no commas.");
        fclose(file); return 1;
    }
    status = read_line("Phone: ", contact.phone, sizeof(contact.phone));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (!valid_field(contact.phone, sizeof(contact.phone))) {
        puts("Phone must be non-empty, under 32 characters, and contain no commas.");
        fclose(file); return 1;
    }
    status = read_line("Email: ", contact.email, sizeof(contact.email));
    if (status <= 0) { fclose(file); return status == 0 ? 0 : 1; }
    if (!valid_field(contact.email, sizeof(contact.email))) {
        puts("Email must be non-empty, under 100 characters, and contain no commas.");
        fclose(file); return 1;
    }
    if (fseek(file, 0, SEEK_END) != 0 ||
        fprintf(file, "%d,%s,%s,%s\n", contact.id, contact.name, contact.phone, contact.email) < 0 ||
        fflush(file) != 0) {
        perror("Could not save contact");
        fclose(file); return 1;
    }
    if (fclose(file) != 0) { perror("Could not close contacts file"); return 1; }
    puts("Contact added successfully.");
    (void)id_text;
    return 1;
}

static void list_contacts(void) {
    FILE *file = fopen(DATA_FILE, "r");
    char line[LINE_CAP];
    int count = 0;
    if (file == NULL) {
        if (errno == ENOENT) puts("No contacts saved yet.");
        else perror("Could not open contacts file");
        return;
    }
    puts("\nID   NAME                                     PHONE                           EMAIL");
    puts("---------------------------------------------------------------------------------------------");
    while (fgets(line, sizeof(line), file) != NULL) {
        Contact contact;
        if (parse_record(line, &contact)) {
            printf("%-4d %-40s %-31s %s\n", contact.id, contact.name, contact.phone, contact.email);
            ++count;
        } else {
            fprintf(stderr, "Warning: skipped malformed record in %s.\n", DATA_FILE);
        }
    }
    if (ferror(file)) perror("Error reading contacts file");
    fclose(file);
    if (count == 0) puts("No valid contacts found.");
    printf("Total contacts: %d\n", count);
}

static int search_contact(void) {
    char query[LINE_CAP];
    int status, found = 0;
    status = read_line("Search by name or phone: ", query, sizeof(query));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (query[0] == '\0') { puts("Search cannot be empty."); return 1; }

    FILE *file = fopen(DATA_FILE, "r");
    if (file == NULL) {
        if (errno == ENOENT) puts("No contacts saved yet.");
        else perror("Could not open contacts file");
        return 1;
    }
    char line[LINE_CAP];
    while (fgets(line, sizeof(line), file) != NULL) {
        Contact contact;
        if (parse_record(line, &contact) &&
            (strstr(contact.name, query) != NULL || strstr(contact.phone, query) != NULL)) {
            printf("ID: %d | Name: %s | Phone: %s | Email: %s\n",
                   contact.id, contact.name, contact.phone, contact.email);
            found = 1;
        }
    }
    if (ferror(file)) perror("Error searching contacts file");
    fclose(file);
    if (!found) puts("No matching contacts found (search is case-sensitive).");
    return 1;
}

static int delete_contact(void) {
    char text[LINE_CAP], line[LINE_CAP];
    int id, status, found = 0, failed = 0;
    status = read_line("Contact ID to delete: ", text, sizeof(text));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!parse_positive_int(text, &id)) { puts("Invalid ID."); return 1; }

    FILE *source = fopen(DATA_FILE, "r");
    if (source == NULL) {
        if (errno == ENOENT) puts("No contacts saved yet.");
        else perror("Could not open contacts file");
        return 1;
    }
    FILE *temp = fopen(TEMP_FILE, "w");
    if (temp == NULL) { perror("Could not create temporary file"); fclose(source); return 1; }

    while (fgets(line, sizeof(line), source) != NULL) {
        Contact contact;
        if (parse_record(line, &contact) && contact.id == id) { found = 1; continue; }
        if (fputs(line, temp) == EOF) failed = 1;
    }
    if (ferror(source) || fflush(temp) != 0) failed = 1;
    if (fclose(source) != 0 || fclose(temp) != 0) failed = 1;
    if (failed) {
        perror("Could not write temporary contacts file");
        remove(TEMP_FILE);
        return 1;
    }
    if (!found) { puts("Contact ID not found."); remove(TEMP_FILE); return 1; }
    if (remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Could not finish deletion; check contacts.csv and contacts.tmp");
        return 1;
    }
    puts("Contact deleted successfully.");
    return 1;
}

int main(void) {
    char line[LINE_CAP];
    puts("================================");
    puts("           CONTACT BOOK");
    puts("================================");
    for (;;) {
        int choice, status;
        puts("\n1. Add contact");
        puts("2. List contacts");
        puts("3. Search by name or phone");
        puts("4. Delete contact");
        puts("5. Exit");
        status = read_line("Choose an option: ", line, sizeof(line));
        if (status == 0) { puts("\nInput closed. Goodbye!"); break; }
        if (status < 0 || !parse_positive_int(line, &choice) || choice > 5) {
            puts("Choose a number from 1 to 5."); continue;
        }
        if (choice == 1) { if (add_contact() == 0) break; }
        else if (choice == 2) list_contacts();
        else if (choice == 3) { if (search_contact() == 0) break; }
        else if (choice == 4) { if (delete_contact() == 0) break; }
        else { puts("Goodbye!"); break; }
    }
    return 0;
}

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "students.csv"
#define TEMP_FILE "students.tmp"
#define NAME_CAP 80
#define LINE_CAP 256

typedef struct {
    int id;
    char name[NAME_CAP];
    double marks;
} Student;

static void trim_newline(char *text) {
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[--length] = '\0';
    }
}

static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }
    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        trim_newline(buffer);
        return 1;
    }
    if (feof(stdin)) {
        trim_newline(buffer);
        return 1;
    }
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        /* Discard excess characters. */
    }
    puts("Input too long. Please try again.");
    return -1;
}

static int parse_int(const char *text, int *value) {
    char *end;
    long parsed;
    while (isspace((unsigned char)*text)) ++text;
    if (*text == '\0') return 0;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (text == end || errno == ERANGE || parsed < 1 || parsed > INT_MAX) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return 0;
    *value = (int)parsed;
    return 1;
}

static int parse_marks(const char *text, double *value) {
    char *end;
    double parsed;
    while (isspace((unsigned char)*text)) ++text;
    if (*text == '\0') return 0;
    errno = 0;
    parsed = strtod(text, &end);
    if (text == end || errno == ERANGE || parsed < 0.0 || parsed > 100.0) return 0;
    while (isspace((unsigned char)*end)) ++end;
    if (*end != '\0') return 0;
    *value = parsed;
    return 1;
}

static int valid_name(const char *name) {
    if (name[0] == '\0') return 0;
    if (strchr(name, ',') != NULL) return 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        if (*p < 32) return 0;
    }
    return 1;
}

static int find_student(FILE *file, int id, Student *found) {
    char line[LINE_CAP];
    rewind(file);
    while (fgets(line, sizeof(line), file) != NULL) {
        Student student;
        char extra;
        if (sscanf(line, "%d,%79[^,],%lf %c", &student.id, student.name,
                   &student.marks, &extra) == 3 && student.id == id) {
            if (found != NULL) *found = student;
            return 1;
        }
    }
    return 0;
}

static int load_student(FILE *file, int id, Student *found) {
    return find_student(file, id, found);
}

static int add_student(void) {
    char line[LINE_CAP];
    Student student;
    int status;

    status = read_line("Student ID (positive integer): ", line, sizeof(line));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!parse_int(line, &student.id)) {
        puts("Invalid ID.");
        return 1;
    }

    FILE *file = fopen(DATA_FILE, "a+");
    if (file == NULL) {
        perror("Could not open student data file");
        return 1;
    }
    if (find_student(file, student.id, NULL)) {
        printf("A student with ID %d already exists.\n", student.id);
        fclose(file);
        return 1;
    }

    status = read_line("Student name (commas are not allowed): ", student.name,
                       sizeof(student.name));
    if (status <= 0) {
        fclose(file);
        return status == 0 ? 0 : 1;
    }
    if (!valid_name(student.name) || strlen(student.name) >= NAME_CAP) {
        puts("Name must be non-empty, shorter than 80 characters, and contain no commas/control characters.");
        fclose(file);
        return 1;
    }

    status = read_line("Marks (0-100): ", line, sizeof(line));
    if (status <= 0) {
        fclose(file);
        return status == 0 ? 0 : 1;
    }
    if (!parse_marks(line, &student.marks)) {
        puts("Invalid marks. Enter a number from 0 to 100.");
        fclose(file);
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0 ||
        fprintf(file, "%d,%s,%.2f\n", student.id, student.name, student.marks) < 0 ||
        fflush(file) != 0) {
        perror("Could not save student");
        fclose(file);
        return 1;
    }
    if (fclose(file) != 0) {
        perror("Could not close student data file");
        return 1;
    }
    puts("Student added successfully.");
    return 1;
}

static void list_students(void) {
    FILE *file = fopen(DATA_FILE, "r");
    char line[LINE_CAP];
    int count = 0;

    if (file == NULL) {
        if (errno == ENOENT) {
            puts("No student records yet.");
        } else {
            perror("Could not open student data file");
        }
        return;
    }

    puts("\nID       NAME                                     MARKS");
    puts("---------------------------------------------------------");
    while (fgets(line, sizeof(line), file) != NULL) {
        Student student;
        char extra;
        if (sscanf(line, "%d,%79[^,],%lf %c", &student.id, student.name,
                   &student.marks, &extra) == 3) {
            printf("%-8d %-40s %6.2f\n", student.id, student.name, student.marks);
            ++count;
        } else {
            fprintf(stderr, "Warning: skipped malformed record in %s.\n", DATA_FILE);
        }
    }
    if (ferror(file)) perror("Error reading student data");
    if (fclose(file) != 0) perror("Error closing student data");
    if (count == 0) puts("No valid student records found.");
    printf("Total records: %d\n", count);
}

static int update_student(void) {
    char line[LINE_CAP];
    int id, status;
    Student student;

    status = read_line("Student ID to update: ", line, sizeof(line));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!parse_int(line, &id)) {
        puts("Invalid ID.");
        return 1;
    }

    FILE *source = fopen(DATA_FILE, "r");
    if (source == NULL) {
        if (errno == ENOENT) puts("No student records yet.");
        else perror("Could not open student data file");
        return 1;
    }
    if (!load_student(source, id, &student)) {
        puts("Student ID not found.");
        fclose(source);
        return 1;
    }
    fclose(source);

    printf("Current name: %s\n", student.name);
    status = read_line("New name (leave blank to keep current): ", line, sizeof(line));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (line[0] != '\0') {
        if (!valid_name(line) || strlen(line) >= sizeof(student.name)) {
            puts("Invalid name.");
            return 1;
        }
        strcpy(student.name, line);
    }

    printf("Current marks: %.2f\n", student.marks);
    status = read_line("New marks (0-100, or blank to keep current): ", line, sizeof(line));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (line[0] != '\0' && !parse_marks(line, &student.marks)) {
        puts("Invalid marks.");
        return 1;
    }

    source = fopen(DATA_FILE, "r");
    FILE *temp = fopen(TEMP_FILE, "w");
    if (source == NULL || temp == NULL) {
        perror("Could not open files for update");
        if (source != NULL) fclose(source);
        if (temp != NULL) fclose(temp);
        remove(TEMP_FILE);
        return 1;
    }

    char record[LINE_CAP];
    int written = 0;
    while (fgets(record, sizeof(record), source) != NULL) {
        Student existing;
        char extra;
        if (sscanf(record, "%d,%79[^,],%lf %c", &existing.id, existing.name,
                   &existing.marks, &extra) == 3 && existing.id == id) {
            if (fprintf(temp, "%d,%s,%.2f\n", student.id, student.name,
                        student.marks) < 0) written = -1;
            else written = 1;
        } else if (fputs(record, temp) == EOF) {
            written = -1;
        }
    }
    if (ferror(source) || fflush(temp) != 0) written = -1;
    if (fclose(source) != 0) written = -1;
    if (fclose(temp) != 0) written = -1;
    if (written < 0 || remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Could not finish update; check students.csv and students.tmp");
        return 1;
    }
    puts("Student updated successfully.");
    return 1;
}

static int delete_student(void) {
    char line[LINE_CAP];
    int id, status, found = 0;

    status = read_line("Student ID to delete: ", line, sizeof(line));
    if (status <= 0) return status == 0 ? 0 : 1;
    if (!parse_int(line, &id)) {
        puts("Invalid ID.");
        return 1;
    }

    FILE *source = fopen(DATA_FILE, "r");
    if (source == NULL) {
        if (errno == ENOENT) puts("No student records yet.");
        else perror("Could not open student data file");
        return 1;
    }
    FILE *temp = fopen(TEMP_FILE, "w");
    if (temp == NULL) {
        perror("Could not create temporary file");
        fclose(source);
        return 1;
    }

    char record[LINE_CAP];
    int failed = 0;
    while (fgets(record, sizeof(record), source) != NULL) {
        Student student;
        char extra;
        if (sscanf(record, "%d,%79[^,],%lf %c", &student.id, student.name,
                   &student.marks, &extra) == 3 && student.id == id) {
            found = 1;
            continue;
        }
        if (fputs(record, temp) == EOF) failed = 1;
    }
    if (ferror(source) || fflush(temp) != 0) failed = 1;
    if (fclose(source) != 0) failed = 1;
    if (fclose(temp) != 0) failed = 1;

    if (failed) {
        perror("Could not write updated student data");
        remove(TEMP_FILE);
        return 1;
    }
    if (!found) {
        puts("Student ID not found.");
        remove(TEMP_FILE);
        return 1;
    }
    if (remove(DATA_FILE) != 0 || rename(TEMP_FILE, DATA_FILE) != 0) {
        perror("Could not finish deletion; check students.csv and students.tmp");
        return 1;
    }
    puts("Student deleted successfully.");
    return 1;
}

int main(void) {
    char line[LINE_CAP];
    puts("================================");
    puts("      STUDENT MANAGEMENT");
    puts("================================");

    for (;;) {
        int choice, status;
        puts("\n1. Add student");
        puts("2. List students");
        puts("3. Update student");
        puts("4. Delete student");
        puts("5. Exit");
        status = read_line("Choose an option: ", line, sizeof(line));
        if (status == 0) {
            puts("\nInput closed. Goodbye!");
            break;
        }
        if (status < 0 || !parse_int(line, &choice) || choice > 5) {
            puts("Choose a number from 1 to 5.");
            continue;
        }
        if (choice == 1) {
            if (add_student() == 0) break;
        } else if (choice == 2) {
            list_students();
        } else if (choice == 3) {
            if (update_student() == 0) break;
        } else if (choice == 4) {
            if (delete_student() == 0) break;
        } else {
            puts("Goodbye!");
            break;
        }
    }
    return 0;
}

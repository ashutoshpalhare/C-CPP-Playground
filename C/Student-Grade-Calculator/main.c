#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SUBJECTS 100
#define INPUT_SIZE 128

static int read_integer(const char *prompt, int minimum, int maximum, int *value) {
    char buffer[INPUT_SIZE];
    for (;;) {
        char *start;
        char *end = NULL;
        long parsed;
        printf("%s", prompt);
        if (fgets(buffer, sizeof buffer, stdin) == NULL) return 0;
        if (strchr(buffer, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) { }
            puts("Input is too long. Please try again.");
            continue;
        }
        start = buffer;
        while (isspace((unsigned char)*start)) ++start;
        errno = 0;
        parsed = strtol(start, &end, 10);
        if (start == end || errno == ERANGE) {
            printf("Enter a whole number from %d to %d.\n", minimum, maximum);
            continue;
        }
        while (end != NULL && isspace((unsigned char)*end)) ++end;
        if ((end != NULL && *end != '\0') || parsed < minimum || parsed > maximum) {
            printf("Enter a whole number from %d to %d.\n", minimum, maximum);
            continue;
        }
        *value = (int)parsed;
        return 1;
    }
}

static char calculate_grade(double average) {
    if (average >= 90.0) return 'A';
    if (average >= 80.0) return 'B';
    if (average >= 70.0) return 'C';
    if (average >= 60.0) return 'D';
    if (average >= 40.0) return 'E';
    return 'F';
}

int main(void) {
    int subject_count, total = 0;
    int *marks;
    double average;
    puts("================================");
    puts("      STUDENT GRADE CALCULATOR");
    puts("================================");
    if (!read_integer("Number of subjects (1-100): ", 1, MAX_SUBJECTS, &subject_count)) {
        puts("Input ended. Goodbye!");
        return 0;
    }
    marks = calloc((size_t)subject_count, sizeof *marks);
    if (marks == NULL) {
        fputs("Error: unable to allocate memory for marks.\n", stderr);
        return 1;
    }
    for (int i = 0; i < subject_count; ++i) {
        char prompt[64];
        (void)snprintf(prompt, sizeof prompt, "Marks for subject %d (0-100): ", i + 1);
        if (!read_integer(prompt, 0, 100, &marks[i])) {
            puts("Input ended before all marks were entered.");
            free(marks);
            return 0;
        }
        total += marks[i];
    }
    average = (double)total / (double)subject_count;
    puts("\n------------ RESULT ------------");
    printf("Subjects       : %d\n", subject_count);
    printf("Total marks    : %d / %d\n", total, subject_count * 100);
    printf("Average        : %.2f%%\n", average);
    printf("Grade          : %c\n", calculate_grade(average));
    printf("Result         : %s\n", average >= 40.0 ? "PASS" : "FAIL");
    puts("--------------------------------");
    puts("Grade scale: A >= 90, B >= 80, C >= 70, D >= 60, E >= 40, F < 40.");
    free(marks);
    return 0;
}

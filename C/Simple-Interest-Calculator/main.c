#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 128

static int read_nonnegative_double(const char *prompt, double *value) {
    char buffer[INPUT_SIZE];
    for (;;) {
        char *start;
        char *end;
        double parsed;

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
        if (*start == '\0') {
            puts("Input cannot be blank. Enter a non-negative number.");
            continue;
        }

        errno = 0;
        parsed = strtod(start, &end);
        if (start == end || errno == ERANGE || !isfinite(parsed)) {
            puts("Invalid number. Please enter a finite numeric value.");
            continue;
        }
        while (isspace((unsigned char)*end)) ++end;
        if (*end != '\0') {
            puts("Unexpected characters. Enter a number only.");
            continue;
        }
        if (parsed < 0.0) {
            puts("Value cannot be negative. Please try again.");
            continue;
        }
        *value = parsed;
        return 1;
    }
}

int main(void) {
    double principal;
    double annual_rate;
    double time_years;
    double interest;
    double amount;

    puts("================================");
    puts("      SIMPLE INTEREST CALCULATOR");
    puts("================================");
    puts("Formula: Simple Interest = (Principal x Rate x Time) / 100");
    puts("Enter principal in your currency, rate as annual percent, and time in years.\n");

    if (!read_nonnegative_double("Principal amount: ", &principal) ||
        !read_nonnegative_double("Annual interest rate (%): ", &annual_rate) ||
        !read_nonnegative_double("Time period (years): ", &time_years)) {
        puts("\nInput ended. Goodbye!");
        return 0;
    }

    interest = (principal / 100.0) * annual_rate * time_years;
    amount = principal + interest;
    if (!isfinite(interest) || !isfinite(amount)) {
        fputs("The result is too large to represent. Try smaller values.\n", stderr);
        return 1;
    }

    puts("\n----------- RESULT -----------");
    printf("Principal amount : %.2f\n", principal);
    printf("Annual rate      : %.2f%%\n", annual_rate);
    printf("Time period      : %.2f years\n", time_years);
    printf("Simple interest  : %.2f\n", interest);
    printf("Total amount     : %.2f\n", amount);
    puts("------------------------------");
    puts("Note: This calculation uses simple interest, not compound interest.");
    return 0;
}

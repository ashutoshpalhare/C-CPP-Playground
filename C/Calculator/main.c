#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 128

static int read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Discard the rest of an overlong input line. */
        }
    }

    return 1;
}

static int parse_number(const char *text, double *value) {
    char *end = NULL;

    while (isspace((unsigned char)*text)) {
        text++;
    }

    if (*text == '\0') {
        return 0;
    }

    errno = 0;
    double parsed = strtod(text, &end);

    if (text == end || errno == ERANGE || !isfinite(parsed)) {
        return 0;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0') {
        return 0;
    }

    *value = parsed;
    return 1;
}

static int read_number(const char *prompt, double *value) {
    char input[INPUT_SIZE];

    for (;;) {
        if (!read_line(prompt, input, sizeof input)) {
            return 0;
        }

        if (parse_number(input, value)) {
            return 1;
        }

        puts("Invalid number. Enter a finite numeric value and try again.");
    }
}

static void print_result(double result) {
    if (!isfinite(result)) {
        puts("Error: the result is outside the supported numeric range.");
        return;
    }

    printf("Result: %.10g\n", result);
}

int main(void) {
    char input[INPUT_SIZE];

    puts("================================");
    puts("       C Command-Line Calculator");
    puts("================================");
    puts("Supported operations: +  -  *  /");
    puts("Enter q at the operator prompt to quit.");

    for (;;) {
        if (!read_line("\nOperator (+, -, *, /, q): ", input, sizeof input)) {
            putchar('\n');
            break;
        }

        char *operator_text = input;
        while (isspace((unsigned char)*operator_text)) {
            operator_text++;
        }

        size_t operator_length = strlen(operator_text);
        while (operator_length > 0 &&
               isspace((unsigned char)operator_text[operator_length - 1])) {
            operator_text[--operator_length] = '\0';
        }

        if (operator_length == 1 &&
            (*operator_text == 'q' || *operator_text == 'Q')) {
            break;
        }

        if (operator_length != 1 ||
            strchr("+-*/", operator_text[0]) == NULL) {
            puts("Unsupported operator. Choose +, -, *, /, or q.");
            continue;
        }

        double left = 0.0;
        double right = 0.0;

        if (!read_number("First number: ", &left) ||
            !read_number("Second number: ", &right)) {
            putchar('\n');
            break;
        }

        switch (operator_text[0]) {
            case '+':
                print_result(left + right);
                break;
            case '-':
                print_result(left - right);
                break;
            case '*':
                print_result(left * right);
                break;
            case '/':
                if (right == 0.0) {
                    puts("Error: division by zero is not allowed.");
                } else {
                    print_result(left / right);
                }
                break;
            default:
                /* The operator is validated before reaching this switch. */
                puts("Unexpected operator.");
                break;
        }
    }

    puts("Calculator closed. Happy coding!");
    return 0;
}

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
            /* Drain input that did not fit in the buffer. */
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
        puts("Invalid number. Please enter a finite numeric value.");
    }
}

static void print_conversion(const char *label, double value, const char *unit) {
    if (!isfinite(value)) {
        puts("Error: converted value is outside the supported numeric range.");
        return;
    }
    printf("%s: %.10g %s\n", label, value, unit);
}

static void show_menu(void) {
    puts("\n========== Unit Converter ==========");
    puts("1. Kilometres to miles");
    puts("2. Miles to kilometres");
    puts("3. Metres to feet");
    puts("4. Feet to metres");
    puts("5. Kilograms to pounds");
    puts("6. Pounds to kilograms");
    puts("7. Celsius to Fahrenheit");
    puts("8. Fahrenheit to Celsius");
    puts("q. Quit");
}

static int convert(int choice, double input) {
    switch (choice) {
        case 1:
            print_conversion("Converted value", input * 0.621371192237, "mi");
            return 1;
        case 2:
            print_conversion("Converted value", input / 0.621371192237, "km");
            return 1;
        case 3:
            print_conversion("Converted value", input * 3.28083989501, "ft");
            return 1;
        case 4:
            print_conversion("Converted value", input / 3.28083989501, "m");
            return 1;
        case 5:
            print_conversion("Converted value", input * 2.20462262185, "lb");
            return 1;
        case 6:
            print_conversion("Converted value", input / 2.20462262185, "kg");
            return 1;
        case 7:
            print_conversion("Converted value", input * 9.0 / 5.0 + 32.0, "F");
            return 1;
        case 8:
            print_conversion("Converted value", (input - 32.0) * 5.0 / 9.0, "C");
            return 1;
        default:
            return 0;
    }
}

int main(void) {
    char input[INPUT_SIZE];

    puts("Welcome to the C Unit Converter.");

    for (;;) {
        show_menu();

        if (!read_line("Choose a conversion (1-8, q to quit): ", input, sizeof input)) {
            putchar('\n');
            break;
        }

        char *choice_text = input;
        while (isspace((unsigned char)*choice_text)) {
            choice_text++;
        }
        size_t length = strlen(choice_text);
        while (length > 0 && isspace((unsigned char)choice_text[length - 1])) {
            choice_text[--length] = '\0';
        }

        if (length == 1 && (*choice_text == 'q' || *choice_text == 'Q')) {
            break;
        }

        if (length != 1 || choice_text[0] < '1' || choice_text[0] > '8') {
            puts("Invalid choice. Select a menu option from 1 to 8, or q to quit.");
            continue;
        }

        int choice = choice_text[0] - '0';
        double value = 0.0;
        if (!read_number("Enter the value to convert: ", &value)) {
            putchar('\n');
            break;
        }

        (void)convert(choice, value);
    }

    puts("Unit Converter closed. Happy coding!");
    return 0;
}

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_NUMBER 1
#define MAX_NUMBER 100
#define INPUT_SIZE 128

typedef enum {
    INPUT_INVALID = 0,
    INPUT_VALID = 1,
    INPUT_QUIT = 2,
    INPUT_END = 3
} InputResult;

static InputResult read_guess(int *guess) {
    char input[INPUT_SIZE];

    for (;;) {
        printf("Your guess (%d-%d, or q to quit): ", MIN_NUMBER, MAX_NUMBER);

        if (fgets(input, sizeof input, stdin) == NULL) {
            return INPUT_END;
        }

        char *start = input;
        while (isspace((unsigned char)*start)) {
            start++;
        }

        if ((*start == 'q' || *start == 'Q') && start[1] == '\n') {
            return INPUT_QUIT;
        }
        if ((*start == 'q' || *start == 'Q') && start[1] == '\0') {
            return INPUT_QUIT;
        }

        errno = 0;
        char *end = NULL;
        long value = strtol(start, &end, 10);

        if (start == end || errno == ERANGE || value < INT_MIN || value > INT_MAX) {
            puts("Invalid input. Enter a whole number in the allowed range.");
            continue;
        }

        while (isspace((unsigned char)*end)) {
            end++;
        }

        if (*end != '\0' || value < MIN_NUMBER || value > MAX_NUMBER) {
            printf("Please enter a whole number from %d to %d.\n", MIN_NUMBER, MAX_NUMBER);
            continue;
        }

        *guess = (int)value;
        return INPUT_VALID;
    }
}

static int play_round(void) {
    const int target = MIN_NUMBER + rand() % (MAX_NUMBER - MIN_NUMBER + 1);
    int attempts = 0;

    puts("\nI picked a number between 1 and 100. Can you guess it?");

    for (;;) {
        int guess = 0;
        InputResult result = read_guess(&guess);

        if (result == INPUT_QUIT || result == INPUT_END) {
            puts("\nRound ended.");
            return 0;
        }
        if (result != INPUT_VALID) {
            continue;
        }

        attempts++;

        if (guess < target) {
            puts("Too low. Try a higher number.");
        } else if (guess > target) {
            puts("Too high. Try a lower number.");
        } else {
            printf("Correct! You guessed the number in %d attempt%s.\n",
                   attempts, attempts == 1 ? "" : "s");
            return 1;
        }
    }
}

int main(void) {
    srand((unsigned int)time(NULL));

    puts("================================");
    puts("        Number Guessing Game");
    puts("================================");
    puts("Guess the secret number from 1 to 100.");
    puts("Enter q to quit the current round.");

    (void)play_round();
    puts("Thanks for playing!");
    return 0;
}

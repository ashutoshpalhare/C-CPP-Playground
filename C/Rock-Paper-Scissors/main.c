#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    ROCK = 0,
    PAPER = 1,
    SCISSORS = 2
} Choice;

static int read_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Discard the remainder of an overlong line. */
        }
        if (!feof(stdin)) {
            puts("Input is too long. Please try again.");
            return -1;
        }
    }
    return 1;
}

static int parse_choice(const char *input, Choice *choice) {
    char normalized[32];
    size_t length = strlen(input);

    if (length == 0 || length >= sizeof(normalized)) {
        return 0;
    }

    for (size_t i = 0; i <= length; ++i) {
        normalized[i] = (char)tolower((unsigned char)input[i]);
    }

    if (strcmp(normalized, "1") == 0 || strcmp(normalized, "r") == 0 ||
        strcmp(normalized, "rock") == 0) {
        *choice = ROCK;
    } else if (strcmp(normalized, "2") == 0 || strcmp(normalized, "p") == 0 ||
               strcmp(normalized, "paper") == 0) {
        *choice = PAPER;
    } else if (strcmp(normalized, "3") == 0 || strcmp(normalized, "s") == 0 ||
               strcmp(normalized, "scissors") == 0) {
        *choice = SCISSORS;
    } else {
        return 0;
    }
    return 1;
}

static const char *choice_name(Choice choice) {
    switch (choice) {
        case ROCK: return "Rock";
        case PAPER: return "Paper";
        case SCISSORS: return "Scissors";
        default: return "Unknown";
    }
}

static int round_result(Choice player, Choice computer) {
    if (player == computer) {
        return 0;
    }
    return ((player == ROCK && computer == SCISSORS) ||
            (player == PAPER && computer == ROCK) ||
            (player == SCISSORS && computer == PAPER)) ? 1 : -1;
}

int main(void) {
    char input[128];
    unsigned long player_score = 0;
    unsigned long computer_score = 0;
    unsigned long ties = 0;

    srand((unsigned int)time(NULL));

    puts("================================");
    puts("       ROCK PAPER SCISSORS");
    puts("================================");
    puts("Choose 1/Rock, 2/Paper, or 3/Scissors.");
    puts("Enter q at the prompt to quit.\n");

    for (;;) {
        Choice player;
        Choice computer;
        int status;
        int result;

        printf("Your choice [1/2/3 or r/p/s]: ");
        status = read_line(input, sizeof(input));
        if (status == 0) {
            puts("\nInput closed. Thanks for playing!");
            break;
        }
        if (status < 0) {
            continue;
        }

        if (tolower((unsigned char)input[0]) == 'q' && input[1] == '\0') {
            break;
        }
        if (!parse_choice(input, &player)) {
            puts("Invalid choice. Enter rock, paper, scissors, or 1/2/3.");
            continue;
        }

        computer = (Choice)(rand() % 3);
        printf("You chose: %s\nComputer chose: %s\n",
               choice_name(player), choice_name(computer));

        result = round_result(player, computer);
        if (result > 0) {
            puts("You win this round!");
            ++player_score;
        } else if (result < 0) {
            puts("Computer wins this round.");
            ++computer_score;
        } else {
            puts("It's a tie!");
            ++ties;
        }

        printf("Score — You: %lu | Computer: %lu | Ties: %lu\n\n",
               player_score, computer_score, ties);
    }

    puts("\n========== FINAL SCORE ==========");
    printf("You: %lu | Computer: %lu | Ties: %lu\n",
           player_score, computer_score, ties);
    if (player_score > computer_score) {
        puts("Overall result: You are ahead!");
    } else if (player_score < computer_score) {
        puts("Overall result: Computer is ahead.");
    } else {
        puts("Overall result: It's level.");
    }
    puts("Thanks for playing!");
    return 0;
}

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOARD_SIZE 9

static void display_board(const char board[BOARD_SIZE]) {
    puts("");
    for (int row = 0; row < 3; ++row) {
        int i = row * 3;
        printf(" %c | %c | %c\n", board[i], board[i + 1], board[i + 2]);
        if (row < 2) {
            puts("---+---+---");
        }
    }
    puts("");
}

static int has_won(const char board[BOARD_SIZE], char mark) {
    static const int lines[8][3] = {
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
        {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
        {0, 4, 8}, {2, 4, 6}
    };

    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i) {
        if (board[lines[i][0]] == mark &&
            board[lines[i][1]] == mark &&
            board[lines[i][2]] == mark) {
            return 1;
        }
    }
    return 0;
}

static int read_move(int *move) {
    char input[128];
    char *end;
    long value;

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }

    size_t length = strlen(input);
    if (length > 0 && input[length - 1] != '\n' && !feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Discard overlong input. */
        }
        puts("Input is too long. Enter a number from 1 to 9.");
        return -1;
    }

    char *start = input;
    while (isspace((unsigned char)*start)) {
        ++start;
    }
    if (*start == '\0' || *start == '\n') {
        puts("Please enter a number from 1 to 9.");
        return -1;
    }

    value = strtol(start, &end, 10);
    if (start == end) {
        puts("Invalid input. Enter a number from 1 to 9.");
        return -1;
    }
    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0') {
        puts("Invalid input. Enter only a number from 1 to 9.");
        return -1;
    }
    if (value < 1 || value > 9) {
        puts("Move out of range. Choose a cell from 1 to 9.");
        return -1;
    }

    *move = (int)value - 1;
    return 1;
}

int main(void) {
    char board[BOARD_SIZE] = {'1','2','3','4','5','6','7','8','9'};
    char current = 'X';
    int moves = 0;

    puts("================================");
    puts("           TIC TAC TOE");
    puts("================================");
    puts("Two players take turns. Player 1 is X; Player 2 is O.");
    puts("Choose a cell number from 1 to 9.\n");
    display_board(board);

    while (moves < BOARD_SIZE) {
        int move;
        int status;

        printf("Player %c, choose a cell: ", current);
        status = read_move(&move);
        if (status == 0) {
            puts("\nInput closed. Game ended.");
            return 0;
        }
        if (status < 0) {
            continue;
        }
        if (board[move] == 'X' || board[move] == 'O') {
            puts("That cell is already taken. Choose another.");
            continue;
        }

        board[move] = current;
        ++moves;
        display_board(board);

        if (has_won(board, current)) {
            printf("Player %c wins in %d move(s)!\n", current, moves);
            return 0;
        }
        if (moves == BOARD_SIZE) {
            puts("It's a draw!");
            return 0;
        }
        current = (current == 'X') ? 'O' : 'X';
    }

    return 0;
}

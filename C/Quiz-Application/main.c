#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPTION_COUNT 4
#define QUESTION_COUNT 10
#define ANSWER_CAP 64

typedef struct {
    const char *question;
    const char *options[OPTION_COUNT];
    char answer;
    const char *explanation;
} Question;

static const Question questions[QUESTION_COUNT] = {
    {"Which header declares printf()?", {"<stdio.h>", "<stdlib.h>", "<string.h>", "<ctype.h>"}, 'A', "printf() is declared in <stdio.h>."},
    {"Which type stores a single character?", {"int", "char", "double", "void"}, 'B', "The char type stores a character-sized integer value."},
    {"Which operator tests equality?", {"=", "==", "!=", ":="}, 'B', "== compares values; = assigns a value."},
    {"Which loop checks its condition before each iteration?", {"do-while only", "switch", "while", "typedef"}, 'C', "A while loop checks its condition before executing the body."},
    {"Which function releases memory allocated by malloc()?", {"delete", "clear", "remove", "free"}, 'D', "free() releases dynamically allocated memory."},
    {"Array indexes in C normally begin at:", {"0", "1", "-1", "Depends on compiler"}, 'A', "The first element of an array is at index zero."},
    {"Which format specifier is commonly used for int in printf()?", {"%c", "%s", "%d", "%f"}, 'C', "%d formats an int in printf()."},
    {"What does strlen() return?", {"Buffer capacity", "Number of characters before the null terminator", "Number of words", "Always the allocated size"}, 'B', "strlen() counts characters before the terminating null byte."},
    {"Which keyword defines a constant-qualified object?", {"static", "fixed", "const", "final"}, 'C', "const qualifies an object so it cannot be modified through that name."},
    {"What does fopen() return on failure?", {"A negative file descriptor", "NULL", "EOF", "An empty string"}, 'B', "fopen() returns NULL when it cannot open a file."}
};

static int read_answer(char *answer, size_t size) {
    if (fgets(answer, (int)size, stdin) == NULL) return 0;
    size_t n = strlen(answer);
    if (n && answer[n - 1] == '\n') answer[--n] = '\0';
    else if (!feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
        return -1;
    }
    while (n && isspace((unsigned char)answer[n - 1])) answer[--n] = '\0';
    size_t start = 0;
    while (answer[start] && isspace((unsigned char)answer[start])) ++start;
    if (start > 0) memmove(answer, answer + start, strlen(answer + start) + 1);
    return 1;
}

static char get_choice(const char *input) {
    while (isspace((unsigned char)*input)) ++input;
    char choice = (char)toupper((unsigned char)*input);
    if (input[0] == '\0' || input[1] != '\0' || choice < 'A' || choice > 'D') return '\0';
    return choice;
}

int main(void) {
    int score = 0;
    char input[ANSWER_CAP];
    puts("================================");
    puts("          C QUIZ APPLICATION");
    puts("================================");
    puts("10 multiple-choice questions. Enter A, B, C, or D.");
    puts("Your score and explanations will be shown at the end.\n");

    for (int i = 0; i < QUESTION_COUNT; ++i) {
        char choice;
        int status;
        printf("\nQuestion %d/%d: %s\n", i + 1, QUESTION_COUNT, questions[i].question);
        for (int j = 0; j < OPTION_COUNT; ++j)
            printf("  %c) %s\n", 'A' + j, questions[i].options[j]);
        for (;;) {
            status = read_answer(input, sizeof(input));
            if (status == 0) { puts("\nInput closed. Quiz ended early."); goto results; }
            if (status < 0) { puts("Answer too long. Enter A, B, C, or D."); continue; }
            choice = get_choice(input);
            if (choice != '\0') break;
            printf("Invalid answer. Enter A, B, C, or D: ");
        }
        if (choice == questions[i].answer) {
            puts("Correct!");
            ++score;
        } else {
            printf("Not quite. Correct answer: %c) %s\n",
                   questions[i].answer,
                   questions[i].options[questions[i].answer - 'A']);
        }
    }

results:
    puts("\n============= RESULTS =============");
    printf("Score: %d/%d (%.1f%%)\n", score, QUESTION_COUNT,
           (100.0 * score) / QUESTION_COUNT);
    if (score == QUESTION_COUNT) puts("Excellent! Perfect score.");
    else if (score >= 8) puts("Great work!");
    else if (score >= 5) puts("Good effort. Keep practising.");
    else puts("Keep learning and try again.");
    puts("\nAnswer review:");
    for (int i = 0; i < QUESTION_COUNT; ++i)
        printf("%2d. %c — %s\n", i + 1, questions[i].answer, questions[i].explanation);
    puts("===================================");
    return 0;
}

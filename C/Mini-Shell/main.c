/*
 * Mini Shell (C11 + POSIX)
 *
 * A small command interpreter that demonstrates processes and file descriptors:
 *   - fork() + execvp() + waitpid() to run external programs
 *   - pipelines:            ls | sort | head
 *   - redirection:          cmd < in.txt   cmd > out.txt   cmd >> log.txt
 *   - background jobs:      sleep 5 &
 *   - quoting:              echo "two words" 'single quoted'
 *   - built-ins:            cd, pwd, exit, help, history, status
 *   - signals: the shell ignores Ctrl+C itself; foreground children get the default action.
 *
 * REQUIRES a POSIX system (Linux, macOS, or Windows via WSL). It does not build natively on Windows.
 */
#define _POSIX_C_SOURCE 200809L

#ifdef _WIN32
#error "Mini Shell needs POSIX APIs (fork, exec, pipe). Build it on Linux, macOS, or inside WSL."
#endif

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_TOKENS 128
#define MAX_CMDS 16
#define MAX_HISTORY 100
#define CWD_CAP 4096

typedef struct {
    char *text;
    int is_op; /* 1 for an unquoted |, <, >, >> or & */
} Token;

typedef struct {
    char *argv[MAX_TOKENS]; /* NULL-terminated; points into token strings */
    int argc;
    char *in_file;
    char *out_file;
    int append;
} Command;

typedef struct {
    Command cmds[MAX_CMDS];
    int count;
    int background;
} Pipeline;

static int last_status = 0;
static char *history[MAX_HISTORY];
static int history_count = 0;

/* ---------- tokenising ---------- */

static void free_tokens(Token *toks, int n) {
    for (int i = 0; i < n; i++) free(toks[i].text);
}

static int is_operator_char(char c) {
    return c == '|' || c == '<' || c == '>' || c == '&';
}

/* Splits a line into words and operators. Returns 0 on success, -1 on error. */
static int tokenize(const char *line, Token *toks, int *count) {
    char word[MAX_LINE];
    int n = 0;
    const char *p = line;
    *count = 0;

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (n >= MAX_TOKENS - 1) {
            fprintf(stderr, "msh: too many words on one line (limit %d)\n", MAX_TOKENS - 1);
            free_tokens(toks, n);
            return -1;
        }

        if (is_operator_char(*p)) {
            char op[3] = {*p, '\0', '\0'};
            if (*p == '>' && p[1] == '>') {
                op[1] = '>';
                p += 2;
            } else {
                p++;
            }
            toks[n].text = strdup(op);
            toks[n].is_op = 1;
            if (!toks[n].text) {
                free_tokens(toks, n);
                return -1;
            }
            n++;
            continue;
        }

        size_t w = 0;
        int ok = 1;
        while (*p && *p != ' ' && *p != '\t' && !is_operator_char(*p)) {
            if (*p == '\'') {
                p++;
                while (*p && *p != '\'') word[w++] = *p++;
                if (!*p) { ok = 0; break; }
                p++;
            } else if (*p == '"') {
                p++;
                while (*p && *p != '"') {
                    if (*p == '\\' && (p[1] == '"' || p[1] == '\\')) p++;
                    word[w++] = *p++;
                }
                if (!*p) { ok = 0; break; }
                p++;
            } else if (*p == '\\' && p[1]) {
                word[w++] = p[1];
                p += 2;
            } else {
                word[w++] = *p++;
            }
        }
        if (!ok) {
            fprintf(stderr, "msh: unterminated quote\n");
            free_tokens(toks, n);
            return -1;
        }
        word[w] = '\0';
        toks[n].text = strdup(word);
        toks[n].is_op = 0;
        if (!toks[n].text) {
            free_tokens(toks, n);
            return -1;
        }
        n++;
    }
    *count = n;
    return 0;
}

/* ---------- parsing ---------- */

/* Builds a Pipeline from tokens. Returns 0 on success, -1 on a syntax error. */
static int parse_pipeline(Token *toks, int n, Pipeline *pl) {
    memset(pl, 0, sizeof *pl);
    pl->count = 1;
    Command *cur = &pl->cmds[0];

    for (int i = 0; i < n; i++) {
        const char *t = toks[i].text;
        if (!toks[i].is_op) {
            cur->argv[cur->argc++] = toks[i].text;
        } else if (strcmp(t, "|") == 0) {
            if (cur->argc == 0) {
                fprintf(stderr, "msh: syntax error: missing command before '|'\n");
                return -1;
            }
            if (pl->count == MAX_CMDS) {
                fprintf(stderr, "msh: too many commands in a pipeline (limit %d)\n", MAX_CMDS);
                return -1;
            }
            cur = &pl->cmds[pl->count++];
        } else if (strcmp(t, "&") == 0) {
            if (i != n - 1) {
                fprintf(stderr, "msh: '&' is only allowed at the end of a line\n");
                return -1;
            }
            pl->background = 1;
        } else { /* <, > or >> */
            if (i + 1 >= n || toks[i + 1].is_op) {
                fprintf(stderr, "msh: syntax error: expected a file name after '%s'\n", t);
                return -1;
            }
            char *file = toks[++i].text;
            if (t[0] == '<') {
                cur->in_file = file;
            } else {
                cur->out_file = file;
                cur->append = (t[1] == '>');
            }
        }
    }
    for (int c = 0; c < pl->count; c++) {
        if (pl->cmds[c].argc == 0) {
            fprintf(stderr, "msh: syntax error: missing command\n");
            return -1;
        }
    }
    return 0;
}

/* ---------- built-in commands ---------- */

static int is_builtin(const char *name) {
    static const char *names[] = {"cd", "pwd", "exit", "help", "history", "status"};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        if (strcmp(name, names[i]) == 0) return 1;
    }
    return 0;
}

static void free_history(void) {
    for (int i = 0; i < history_count; i++) free(history[i]);
    history_count = 0;
}

static int run_builtin(Command *cmd) {
    const char *name = cmd->argv[0];
    if (strcmp(name, "cd") == 0) {
        const char *dir = cmd->argc > 1 ? cmd->argv[1] : getenv("HOME");
        if (!dir) {
            fprintf(stderr, "msh: cd: HOME is not set\n");
            return 1;
        }
        if (cmd->argc > 2) {
            fprintf(stderr, "msh: cd: too many arguments\n");
            return 1;
        }
        if (chdir(dir) != 0) {
            fprintf(stderr, "msh: cd: %s: %s\n", dir, strerror(errno));
            return 1;
        }
        return 0;
    }
    if (strcmp(name, "pwd") == 0) {
        char cwd[CWD_CAP];
        if (!getcwd(cwd, sizeof cwd)) {
            perror("msh: pwd");
            return 1;
        }
        puts(cwd);
        return 0;
    }
    if (strcmp(name, "exit") == 0) {
        int code = last_status;
        if (cmd->argc > 1) {
            char *end;
            long v = strtol(cmd->argv[1], &end, 10);
            if (*end != '\0' || end == cmd->argv[1]) {
                fprintf(stderr, "msh: exit: numeric argument required\n");
                return 2;
            }
            code = (int)(v & 0xFF);
        }
        free_history();
        exit(code);
    }
    if (strcmp(name, "help") == 0) {
        puts("Mini Shell built-ins:");
        puts("  cd [dir]     change directory (HOME if omitted)");
        puts("  pwd          print the working directory");
        puts("  history      show previously entered lines");
        puts("  status       show the exit status of the last command");
        puts("  exit [code]  leave the shell");
        puts("Syntax: cmd args | cmd2   cmd < in   cmd > out   cmd >> out   cmd &   'quotes' \"quotes\"");
        return 0;
    }
    if (strcmp(name, "history") == 0) {
        for (int i = 0; i < history_count; i++) printf("%4d  %s\n", i + 1, history[i]);
        return 0;
    }
    if (strcmp(name, "status") == 0) {
        printf("%d\n", last_status);
        return 0;
    }
    return 1;
}

/* ---------- running external commands ---------- */

static void close_all_pipes(int pipes[][2], int count) {
    for (int i = 0; i < count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

static void redirect_or_die(const char *path, int flags, int target_fd) {
    int fd = open(path, flags, 0644);
    if (fd < 0) {
        fprintf(stderr, "msh: %s: %s\n", path, strerror(errno));
        _exit(1);
    }
    if (dup2(fd, target_fd) < 0) {
        perror("msh: dup2");
        _exit(1);
    }
    close(fd);
}

/* Runs in the child process after fork(). Never returns. */
static void child_main(const Pipeline *pl, int index, int pipes[][2]) {
    const Command *cmd = &pl->cmds[index];
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sigemptyset(&sa.sa_mask);
    /* Background jobs ignore Ctrl+C so that it only stops foreground work. */
    sa.sa_handler = pl->background ? SIG_IGN : SIG_DFL;
    sigaction(SIGINT, &sa, NULL);

    if (index > 0 && dup2(pipes[index - 1][0], STDIN_FILENO) < 0) {
        perror("msh: dup2");
        _exit(1);
    }
    if (index < pl->count - 1 && dup2(pipes[index][1], STDOUT_FILENO) < 0) {
        perror("msh: dup2");
        _exit(1);
    }
    close_all_pipes(pipes, pl->count - 1);

    if (cmd->in_file) redirect_or_die(cmd->in_file, O_RDONLY, STDIN_FILENO);
    else if (pl->background && index == 0) redirect_or_die("/dev/null", O_RDONLY, STDIN_FILENO);
    if (cmd->out_file) {
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);
        redirect_or_die(cmd->out_file, flags, STDOUT_FILENO);
    }

    execvp(cmd->argv[0], cmd->argv);
    fprintf(stderr, "msh: %s: %s\n", cmd->argv[0], strerror(errno));
    _exit(errno == ENOENT ? 127 : 126);
}

static int status_to_code(int st) {
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) {
        fprintf(stderr, "msh: process terminated by signal %d\n", WTERMSIG(st));
        return 128 + WTERMSIG(st);
    }
    return 1;
}

static int run_pipeline(Pipeline *pl) {
    /* Built-ins run inside the shell itself (cd must change the shell's own directory). */
    if (is_builtin(pl->cmds[0].argv[0])) {
        if (pl->count > 1 || pl->cmds[0].in_file || pl->cmds[0].out_file || pl->background) {
            fprintf(stderr, "msh: built-in '%s' cannot be piped, redirected or backgrounded\n", pl->cmds[0].argv[0]);
            return 1;
        }
        return run_builtin(&pl->cmds[0]);
    }

    int pipes[MAX_CMDS][2];
    int made = 0;
    for (; made < pl->count - 1; made++) {
        if (pipe(pipes[made]) < 0) {
            perror("msh: pipe");
            close_all_pipes(pipes, made);
            return 1;
        }
    }

    pid_t pids[MAX_CMDS];
    int started = 0;
    int failed = 0;
    for (int i = 0; i < pl->count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("msh: fork");
            failed = 1;
            break;
        }
        if (pid == 0) child_main(pl, i, pipes);
        pids[started++] = pid;
    }
    close_all_pipes(pipes, pl->count - 1);

    if (pl->background && !failed) {
        printf("[background] pid %ld\n", (long)pids[started - 1]);
        return 0;
    }

    int code = failed ? 1 : 0;
    for (int i = 0; i < started; i++) {
        int st;
        while (waitpid(pids[i], &st, 0) < 0) {
            if (errno != EINTR) {
                st = 1 << 8;
                break;
            }
        }
        if (i == started - 1 && !failed) code = status_to_code(st);
    }
    return code;
}

/* ---------- history, input and main loop ---------- */

static void add_history(const char *line) {
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) return;
    char *copy = strdup(line);
    if (!copy) return;
    if (history_count == MAX_HISTORY) {
        free(history[0]);
        memmove(&history[0], &history[1], (MAX_HISTORY - 1) * sizeof history[0]);
        history_count--;
    }
    history[history_count++] = copy;
}

static void reap_background_jobs(void) {
    int st;
    pid_t pid;
    while ((pid = waitpid(-1, &st, WNOHANG)) > 0) {
        printf("[background] pid %ld finished\n", (long)pid);
    }
}

/* Returns 1 = line read, 0 = EOF, -1 = line too long. */
static int read_line(char *buf, size_t size) {
    if (!fgets(buf, (int)size, stdin)) return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        return 1;
    }
    int ch, overflow = 0;
    while ((ch = getchar()) != '\n' && ch != EOF) overflow = 1;
    return overflow ? -1 : 1;
}

int main(void) {
    struct sigaction ignore;
    memset(&ignore, 0, sizeof ignore);
    sigemptyset(&ignore.sa_mask);
    ignore.sa_handler = SIG_IGN;
    sigaction(SIGINT, &ignore, NULL); /* Ctrl+C must not kill the shell */

    puts("Mini Shell - type 'help' for built-ins, 'exit' or Ctrl+D to quit.");

    for (;;) {
        reap_background_jobs();

        char cwd[CWD_CAP];
        if (!getcwd(cwd, sizeof cwd)) snprintf(cwd, sizeof cwd, "?");
        printf("msh:%s$ ", cwd);
        fflush(stdout);

        char line[MAX_LINE];
        int r = read_line(line, sizeof line);
        if (r == 0) {
            putchar('\n');
            break;
        }
        if (r < 0) {
            fprintf(stderr, "msh: line too long (limit %d characters)\n", MAX_LINE - 1);
            last_status = 1;
            continue;
        }

        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') continue;
        add_history(line);

        Token toks[MAX_TOKENS];
        int ntok = 0;
        if (tokenize(line, toks, &ntok) != 0) {
            last_status = 1;
            continue;
        }
        if (ntok == 0) continue;

        Pipeline pl;
        if (parse_pipeline(toks, ntok, &pl) != 0) {
            last_status = 2;
        } else {
            last_status = run_pipeline(&pl);
        }
        free_tokens(toks, ntok);
    }
    free_history();
    return last_status;
}

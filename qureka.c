#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 1024
#define MAX_LEN 256
#define MAX_VARS 64
#define MAX_NAME 32
#define MAX_VAL 64

typedef struct { char name[MAX_NAME]; char val[MAX_VAL]; } Var;

static Var vars[MAX_VARS];
static int nvars = 0;
static char lines[MAX_LINES][MAX_LEN];
static int nlines = 0;

static int find_var(const char *n) {
    for (int i = 0; i < nvars; i++)
        if (strcmp(vars[i].name, n) == 0) return i;
    return -1;
}

static void set_var(const char *n, const char *v) {
    int i = find_var(n);
    if (i >= 0) { strncpy(vars[i].val, v, MAX_VAL-1); return; }
    if (nvars < MAX_VARS) {
        strncpy(vars[nvars].name, n, MAX_NAME-1);
        strncpy(vars[nvars].val, v, MAX_VAL-1);
        nvars++;
    }
}

static const char *get_var(const char *n) {
    int i = find_var(n);
    return i >= 0 ? vars[i].val : "";
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s) - 1;
    while (e > s && (*e == ' ' || *e == '\t' || *e == '\n')) *e-- = '\0';
    return s;
}

int main(int argc, char **argv) {
    if (argc != 2) { printf("Usage: %s <file.qrk>\n", argv[0]); return 1; }

    FILE *f = fopen(argv[1], "r");
    if (!f) { printf("Error: cannot open %s\n", argv[1]); return 1; }

    nlines = 0;
    while (fgets(lines[nlines], MAX_LEN, f) && nlines < MAX_LINES) nlines++;
    fclose(f);

    int pc = 0;
    while (pc < nlines) {
        char buf[MAX_LEN];
        strncpy(buf, lines[pc], MAX_LEN);
        char *cmd = trim(buf);

        if (*cmd == '\0' || *cmd == '#') { pc++; continue; }

        if (strncmp(cmd, "//", 2) == 0) {
            printf("%s\n", trim(cmd + 2));
            pc++; continue;
        }

        if (strncmp(cmd, "PRINT ", 6) == 0) {
            char *a = trim(cmd + 6);
            if (*a == '"') {
                char *e = strrchr(a, '"');
                if (e && e != a) { *e = '\0'; printf("%s\n", a+1); }
            } else {
                printf("%s\n", get_var(a));
            }
        }
        else if (strncmp(cmd, "SET ", 4) == 0) {
            char *r = trim(cmd + 4);
            char *sp = strchr(r, ' ');
            if (sp) { *sp = '\0'; set_var(r, trim(sp+1)); }
        }
        else if (strncmp(cmd, "INPUT ", 6) == 0) {
            char b[MAX_VAL];
            if (fgets(b, sizeof(b), stdin)) {
                b[strcspn(b, "\n")] = '\0';
                set_var(trim(cmd + 6), b);
            }
        }
        else if (strncmp(cmd, "LABEL ", 6) == 0) { /* no-op */ }
        else if (strncmp(cmd, "GOTO ", 5) == 0) {
            char *lbl = trim(cmd + 5);
            int found = 0;
            for (int i = 0; i < nlines; i++) {
                char lb[MAX_LEN];
                strncpy(lb, lines[i], MAX_LEN);
                char *c = trim(lb);
                if (strncmp(c, "LABEL ", 6) == 0 && strcmp(trim(c+6), lbl) == 0) {
                    pc = i; found = 1; break;
                }
            }
            if (!found) { printf("Error: label '%s' not found\n", lbl); return 1; }
            continue;
        }
        else if (strncmp(cmd, "IF ", 3) == 0) {
            char *r = trim(cmd + 3);
            char *eq = strstr(r, "==");
            if (eq) {
                char vn[MAX_NAME] = {0};
                strncpy(vn, r, eq - r);
                if (strcmp(get_var(trim(vn)), trim(eq+2)) != 0) {
                    pc++;
                    while (pc < nlines) {
                        char lb[MAX_LEN]; strncpy(lb, lines[pc], MAX_LEN);
                        if (strcmp(trim(lb), "ENDIF") == 0) break;
                        pc++;
                    }
                }
            }
        }
        else if (strcmp(cmd, "ENDIF") == 0) { /* no-op */ }
        else if (strcmp(cmd, "HALT") == 0) { break; }
        else { printf("Unknown at line %d: %s\n", pc+1, cmd); }

        pc++;
    }
    return 0;
}

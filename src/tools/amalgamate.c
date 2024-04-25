#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dirent.h"

#define null NULL

#ifndef countof
    #define countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

#define strequ(s1, s2) (strcmp((s1), (s2)) == 0)

static const char* exe;
static const char* name;
static int strlen_name;
static char inc[256];
static char src[256];
static char mem[1024 * 1023];
static char* brk = mem;



#define fatal_if(x, ...) do {                                   \
    if (x) {                                                    \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); \
        fprintf(stderr, "" __VA_ARGS__);                        \
        fprintf(stderr, "\nFATAL\n");                           \
        exit(1);                                                \
    }                                                           \
} while (0)


static const char* basename(const char* filename) {
    const char* s = strrchr(filename, '\\');
    if (s == null) { s = strrchr(filename, '/'); }
    return s != null ? s + 1 : filename;
}

static char* dup(const char* s) { // strdup() like to avoid leaks reporting
    int n = (int)strlen(s) + 1;
    fatal_if(brk + n > mem + sizeof(mem), "out of memory");
    char* c = memcpy(brk, s, n);
    brk += n;
    return c;
}

static char* concat(const char* s1, const char* s2) {
    int n1 = (int)strlen(s1);
    int n2 = (int)strlen(s2);
    fatal_if(brk + n1 + n2 + 1 > mem + sizeof(mem), "out of memory");
    char* c = brk;
    memcpy((char*)memcpy(brk, s1, n1) + n1, s2, n2 + 1);
    brk += n1 + n2 + 1;
    return c;
}

static bool ends_with(const char* s1, const char* s2) {
    int n1 = (int)strlen(s1);
    int n2 = (int)strlen(s2);
    return n1 >= n2 && strequ(s1 + n1 - n2, s2);
}

typedef struct { const char* a[1024]; int n; } set_t;

static set_t files;

static bool set_has(set_t* set, const char* s) {
    for (int i = 0; i < set->n; i++) {
        if (strequ(set->a[i], s)) { return true; }
    }
    return false;
}

static void set_add(set_t* set, const char* s) {
    assert(!set_has(set, s));
    if (!set_has(set, s)) {
        fatal_if(set->n == countof(set->a), "too many files");
        set->a[set->n] = dup(s);
        set->n++;
    }
}

static int usage(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s <name>\n", exe);
    fprintf(stderr, "Assumes src/name and inc/name folders exist\n");
    fprintf(stderr, "and inc/<name>/ contain <name>.h\n");
    fprintf(stderr, "\n");
    return 1;
}

static void tail_trim(char* s) {
    char* p = s + strlen(s) - 1;
    while (p >= s && *p < 0x20) { *p-- = 0x00; }
}

static void divider(const char* fn) {
    char underscores[40] = {0};
    memset(underscores, '_', countof(underscores) - 1);
    int i = (int)(74 - strlen(fn)) / 2;
    int j = (int)(74 - i - strlen(fn));
    printf("// %.*s %s %.*s\n\n", i, underscores, fn, j, underscores);
}

static const char* include(char* s) {
    char fn[256] = {0};
    const char* include = "#include\x20\"";
    if (strstr(s, include) == s) {
        s += strlen(include);
        const char* q = strchr(s, '"');
        if (q != null) {
            snprintf(fn, countof(fn) - 1, "%.*s", (int)(q - s), s);
            return strstr(fn, name) == fn && fn[strlen_name] == '/' ?
                   dup(fn + strlen_name + 1) : null;
        }
    }
    return null;
}

static bool ignore(const char* s) {
    return strequ(s, "#pragma once") ||
           strequ(s, "begin_c") || strequ(s, "end_c");
}

static void parse(const char* fn) {
    FILE* f = fopen(fn, "r");
    fatal_if(f == null, "file not found: `%s`", fn);
    static char line[16 * 1024];
    bool first = true;
    while (fgets(line, countof(line) - 1, f) != null) {
        tail_trim(line);
        const char* in = include(line);
        if (in != null) {
            if (!set_has(&files, in)) {
                set_add(&files, in);
                parse(concat(inc, concat("/", in)));
            }
        } else if (ends_with(fn, ".c") || !ignore(line)) {
            if (first && line[0] != 0) {
                divider(fn + 5 + strlen_name); first = false;
            }
            printf("%s\n", line);
        }
    }
    fclose(f);
    if (ends_with(fn, "/std.h")) { printf("\nbegin_c\n"); }
}

static void definition(void) {
    printf("#ifndef %s_definition\n", name);
    printf("#define %s_definition\n", name);
    printf("\n");
    parse(concat(inc, concat("/", concat(name, ".h"))));
    printf("\n");
    printf("end_c\n");
    printf("\n");
    printf("#endif // %s_definition\n", name);
    const char* name_h = concat(name, ".h");
    // because name.h is fully processed do not include it again:
    if (!set_has(&files, name_h)) { set_add(&files, concat(name, ".h")); }
}

static void implementation(void) {
    printf("\n");
    printf("#ifdef %s_implementation\n", name);
    DIR* d = opendir(src);
    fatal_if(d == null, "folder not found: `%s`", src);
    struct dirent* e = readdir(d);
    while (e != null) {
        if (ends_with(e->d_name, ".c")) {
            parse(concat(src, concat("/", e->d_name)));
        }
        e = readdir(d);
    }
    fatal_if(closedir(d) != 0);
    printf("\n");
    printf("#endif // %s_implementation\n", name);
    printf("\n");
}

int main(int argc, const char* argv[]) {
    exe = basename(argv[0]);
    if (argc < 2) { exit(usage()); }
    name = argv[1];
    strlen_name = (int)strlen(name);
    snprintf(inc, countof(inc) - 1, "inc/%s", name);
    snprintf(src, countof(inc) - 1, "src/%s", name);
    definition();
    implementation();
    return 0;
}

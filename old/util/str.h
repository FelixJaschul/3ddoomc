#pragma once

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "util/types.h"
#include "util/macros.h"

#define STRNFTIME_MAX 128
ALWAYS_INLINE size_t strnftime(
    char *p,
    usize n,
    const char *fmt,
    const struct tm *t) {
    size_t sz = strftime(p, n, fmt, t);
    if (sz == 0) {
        char buf[STRNFTIME_MAX];
        sz = strftime(buf, sizeof buf, fmt, t);
        if (sz == 0) {
            return 0;
        }
        p[0] = 0;
        strncat(p, buf, n - 1);
    }
    return sz;
}

ALWAYS_INLINE void strntimestamp(char *p, usize n) {
    time_t tm = time(NULL);
    strnftime(p, n, "%c", gmtime(&tm));
}

// reads the next line of 'str' into 'buf', or the entire string if it contains
// no newlines
// returns NULL if buf cannot contain line, otherwise returns pointer to next
// line in str (which can be pointer to \0 if this was the last line)
ALWAYS_INLINE const char *strline(const char *str, char *line, usize n) {
    const char *end = strchr(str, '\n');

    if (end) {
        if ((usize) (end - str) > n) { return NULL; }
        memcpy(line, str, end - str);
        line[end - str] = '\0';
        return end + 1;
    } else {
        const usize len = strlen(str);
        if (len > n) { return NULL; }
        memcpy(line, str, len);
        line[len] = '\0';
        return &str[len - 1];
    }
}

// returns 0 if str is prefixed by or equal to pre, otherwise returns 1
ALWAYS_INLINE int strpre(const char *str, const char *pre) {
    while (*str && *pre && *str == *pre) {
        str++;
        pre++;
    }
    return (!*pre && !*str) || (!*pre && *str) ? 0 : 1;
}

// returns 0 if str is suffixed by or equal to pre, otherwise returns 1
ALWAYS_INLINE int strsuf(const char *str, const char *suf) {
    const char
        *e_str = str + strlen(str) - 1,
        *e_suf = suf + strlen(suf) - 1;

    while (e_str != str && e_suf != suf && *e_str== *e_suf) {
        e_str--;
        e_suf--;
    }

    return
        (e_str == str && e_suf == suf) || (e_str != str && e_suf == suf) ?
            0 : 1;
}

// trim left isspace chars
ALWAYS_INLINE char *strltrim(char *str) {
    while (isspace(*str)) { str++; }
    return str;
}

// trim right isspace chars
ALWAYS_INLINE char *strrtrim(char *str) {
    char *end = str + strlen(str) - 1;
    while (isspace(*end)) {
        *end = '\0';

        if (end == str) {
            break;
        }

        end--;
    }

    return str;
}

ALWAYS_INLINE char *strtrim(char *str) {
    return strltrim(strrtrim(str));
}

// safe snprintf alternative which can be to concatenate onto the end of a
// buffer
//
// char buf[100];
// snprintf(buf, sizeof(buf), "%s", ...);
// xnprintf(buf, sizeof(buf), "%d %f %x", ...);
//
// returns index of null terminator in buf
ALWAYS_INLINE int xnprintf(char *buf, int n, const char *fmt, ...) {
    va_list args;
    int res = 0;
    int len = strnlen(buf, n);
    va_start(args, fmt);
    if (n - len > 0) {
        res = vsnprintf(buf + len, n - len, fmt, args);
    }
    va_end(args);
    return res + len;
}

ALWAYS_INLINE void strtoupper(char *str) {
    while (*str) { *str = toupper(*str); str++; }
}

ALWAYS_INLINE void strtolower(char *str) {
    while (*str) { *str = tolower(*str); str++; }
}

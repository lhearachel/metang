// SPDX-License-Identifier: MIT

#include "libs/strings.h"

#include <stdlib.h>
#include <string.h>

int space(int c)
{
    return (c >= '\t' && c <= '\r') || c == ' ';
}

int alpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

strpair strcut(const string s, unsigned char c)
{
    strpair pair = {
        .head = s,
        .tail = stringZ,
    };

    for (long i = 0; i < pair.head.len; i++) {
        if (pair.head.s[i] == c) {
            pair.tail.s   = pair.head.s + i + 1;
            pair.tail.len = pair.head.len - i - 1;
            pair.head.len = i;
            break;
        }
    }

    return pair;
}

string strltrim(string s)
{
    while (s.len > 0 && space(s.s[0])) {
        s.s++;
        s.len--;
    }
    return s;
}

string strrtrim(string s)
{
    while (s.len > 0 && space(s.s[s.len - 1])) s.len--;
    return s;
}

static inline int _strnequ(const string a, const string b, long n) // NOLINT
{
    long i = 0;
    for (; i < n && a.s[i] == b.s[i]; i++);
    return i == n;
}

int strequ(const string a, const string b) // NOLINT
{
    return a.len == b.len && _strnequ(a, b, a.len);
}

#define min(__a, __b) ((__a) <= (__b) ? (__a) : (__b))

int strnequ(const string a, const string b, long n) // NOLINT
{
    n = n < a.len && n < b.len ? n : min(a.len, b.len);
    return _strnequ(a, b, n);
}

#define lower(__c) (((__c) >= 'A' && (__c) <= 'Z') ? (__c) + ('a' - 'A') : (__c))

int stricmp(const string a, const string b)
{
    long n = min(a.len, b.len);
    long i = 0;
    for (; i < n && lower(a.s[i]) == lower(b.s[i]); i++);

    unsigned char ac = lower(a.s[i]);
    unsigned char bc = lower(b.s[i]);

    // 0 if equal, -1 if a is lesser, 1 if b is lesser
    return i == n ? ac != bc : ac < bc ? -1 : 1;
}

long strnum(string s, const int base, char *inval)
{
    // just base-10 for now
    (void)base;

    long sum  = 0;
    int  sign = 1;
    if (s.len && s.s[0] == '-') {
        s.s++;
        s.len--;
        sign = -1;
    }

    for (int i = 0; i < s.len; i++) {
        int digit = (s.s[i] - '0');
        if (digit < 0 || digit > 9) {
            *inval = (char)s.s[i];
            return 0;
        }

        sum *= 10;
        sum += digit;
    }

    return sum * sign;
}

string strmake(const char *s)
{
    return string(s, strlen(s));
}

string strupper(const string s)
{
    char       *upper = malloc(s.len + 1);
    const char *p     = (char *)s.s;
    char       *u     = upper;

    for (; *p; p++, u++) {
        if (*p >= 'a' && *p <= 'z') *u = (char)(*p - ('a' - 'A'));
        else if (*p < '0' || (*p > '9' && *p < 'A') || *p > 'Z') *u = '_';
        else *u = *p;
    }

    *u = '\0';
    return string(upper, s.len);
}

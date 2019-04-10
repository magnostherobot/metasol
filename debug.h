#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifndef NDEBUG
# define debug(...) fprintf(stderr, __VA_ARGS__)
#else
# define debug(...)
#endif

static inline char *move_str_buf(const ms_move *m) {
    static const int buf_size = 30;
    static char buf[buf_size];
    static char *buf_p = &buf[0];

    if (m->stock) {
        snprintf(buf_p, buf_size, "stock");
    } else {
        snprintf(buf_p, buf_size, "%2ux %2u->%2u", m->size, m->from, m->to);
    }

    return buf;
}

#endif /* DEBUG_H */

#ifndef UTIL_H
#define UTIL_H

#define list_push(l, v) \
    if (l.count >= l.size) { \
        l.size++; \
        l.items = realloc(l.items, sizeof(*l.items)*l.size); \
    } \
    l.items[l.count++] = v

#define list_push2(l, v) \
    if (l->count == l->length - 1) { \
        l->length *= 2; \
        l->array = realloc(l->array, sizeof(*l->array)*l->length); \
    } \
    l->array[l->count++] = v

#endif
#ifndef hstack_h__
#define hstack_h__

#include <stddef.h>

typedef struct hstack_header_t *hstack_t;
typedef const struct hstack_header_t *chstack_t;
#define HSTACK_MIN_CAPACITY 1024

hstack_t hstack_new ();
hstack_t hstack_new2 (size_t capacity);
void hstack_delete(hstack_t hstack);

#define /* T* */ hstack_push(hstack, pel) hstack_push2 (&hstack, pel, sizeof (*pel))
/* returns pointer to pushed element or NULL */
void* hstack_push2 (hstack_t* phstack, void* elem, size_t elsize);
void* hstack_push0 (hstack_t* phstack, size_t elsize);

/* 0 or -1 is stack is empty */
#define /* int */ hstack_pop(hstack) hstack_pop2 (&hstack)
int hstack_pop2 (hstack_t* phstack);

/* returns pointer to top or NULL if empty */
void* hstack_top (chstack_t hstack, /* opt, out */ size_t* elsize);

#endif

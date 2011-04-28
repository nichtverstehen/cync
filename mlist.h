#include <stddef.h>
#include <sys/types.h>

/* 
 mlist does not preserve order, but follows a consistent insert/remove manner 
 mlist is not trivially copyable
 */

#define MLIST_MIN_LEN 1

#define /* T* */ mlist_new(T) (T*) mlist_new2 (sizeof (T))
void* mlist_new2 (size_t elsize);
void mlist_delete (void* list);
size_t mlist_len (const void* list);
size_t mlist_capacity (const void* list);
void* mlist_clone (const void* list);

/* can returns index or -1. may change list ptr */
#define /* ssize_t */ mlist_add(list, pel) mlist_add_(list, pel)
#define /* void */ mlist_remove(list, pel) mlist_remove2 ((void**) &list, pel, sizeof (list[0]))
#define /* void */ mlist_reserve(list, len) mlist_reserve2 ((void**) &list, len, sizeof (list[0]))
ssize_t mlist_add2 (void** list, const void* pel, size_t elsize);
void mlist_remove2 (void** list, size_t pos, size_t elsize);
void mlist_reserve2 (void** list, size_t len, size_t elsize);

/* impl */

#define countof(x) ( sizeof (x) / sizeof (x[0]) )

/* some rudimentary type checking */
#define MLIST_ASSERT_SAME_TYPE(a, b) ( \
		(0 ? a : b), \
		(struct { char dummy_[sizeof (a) == sizeof (b) ? 1 : -1]; } *) 0 \
	)

#define /* ssize_t */ mlist_add_(list, pel) ( \
		MLIST_ASSERT_SAME_TYPE (*list, *pel), \
		mlist_add2 ((void**) &list, pel, sizeof (list[0])) /* malloc-aligned anyway */ \
	)

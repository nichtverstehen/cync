#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "mlist.h"

#define ASSERT assert

struct mlist_header_t
{
	size_t len;
	size_t capacity;
};

#define GET_MLIST(x) ((struct mlist_header_t*) x - 1)

void* mlist_new2 (size_t elsize)
{
	struct mlist_header_t* mlist = malloc (sizeof (struct mlist_header_t) + elsize * MLIST_MIN_LEN);
	if (!mlist)
	{
		return NULL;
	}
	
	void* data = &mlist[1];
	mlist->len = 0;
	mlist->capacity = MLIST_MIN_LEN;
	return data;
}

void mlist_delete (void* list)
{
	struct mlist_header_t* mlist = GET_MLIST (list);
	free (mlist);
}

size_t mlist_len (const void* list)
{
	const struct mlist_header_t* mlist = GET_MLIST (list);
	return mlist->len;
}

size_t mlist_capacity (const void* list)
{
	const struct mlist_header_t* mlist = GET_MLIST (list);
	return mlist->capacity;
}

void* mlist_clone2 (const void* list, size_t elsize)
{
	const struct mlist_header_t* mlist = GET_MLIST (list);
	const size_t cbs = sizeof (struct mlist_header_t) + elsize * mlist->len;
	struct mlist_header_t* newlist = malloc (cbs);
	memcpy (newlist, mlist, cbs);
	return &newlist[1];
}

int mlist_grow (void** list, size_t newsize, size_t elsize)
{
	struct mlist_header_t* mlist = GET_MLIST (*list);
	const size_t maxelements = (SIZE_MAX - sizeof (struct mlist_header_t)) / elsize;
	if (newsize > maxelements)
	{
		return -1;
	}
	
	mlist = realloc (mlist, sizeof (struct mlist_header_t) + newsize * elsize);
	if (!mlist)
	{
		return -1;
	}
	
	mlist->capacity = newsize;
	*list = mlist + 1;
	return 0;
}

ssize_t mlist_add2 (void** list, const void* pel, size_t elsize)
{
	struct mlist_header_t* mlist = GET_MLIST (*list);
	ASSERT (mlist->len <= mlist->capacity);
	
	if (mlist->len >= SSIZE_MAX)
	{
		return -1; /* can't express return value */
	}
	
	if (mlist->capacity == mlist->len)
	{
		size_t newsize = mlist->capacity < (SIZE_MAX >> 1) ? mlist->capacity << 1 : SIZE_MAX;
		if (mlist_grow (list, newsize, elsize) < 0)
		{
			return -1;
		}
		
		mlist = GET_MLIST (*list);
	}
	
	ASSERT (mlist->len < mlist->capacity);
	
	memcpy ((char*) *list + mlist->len * elsize, pel, elsize);
	mlist->len += 1;
	return mlist->len - 1;
}

void mlist_remove2 (void** list, size_t pos, size_t elsize)
{
	struct mlist_header_t* mlist = GET_MLIST (*list);
	ASSERT (pos < mlist->len);
	ASSERT (mlist->len <= mlist->capacity);
	
	if (pos < mlist->len - 1)
	{
		/* eliminate hole */
		memcpy ((char*) *list + elsize * pos, (char*) *list + elsize * (mlist->len - 1), elsize);
	}
	
	mlist->len -= 1;
	
	if (mlist->len <= (mlist->capacity >> 1) && (mlist->capacity >> 1) >= MLIST_MIN_LEN )
	{
		mlist_grow (list, mlist->capacity >> 1, elsize);
		/* not fatal to fail shrinking */
	}
}

void mlist_reserve2 (void** list, size_t len, size_t elsize)
{
	struct mlist_header_t* mlist = GET_MLIST (*list);
	
	if (mlist->capacity > len || len < mlist->len)
	{
		return;
	}
	
	mlist_grow (list, len, elsize);
}

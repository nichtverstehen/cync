#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "hstack.h"

#define ASSERT assert
#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

struct hstack_header_t
{
	size_t capacity; /* without header */
	size_t head; /* offset to head frame */
	/* data frames consist of header and data itself */
	/* last frame is a dummy frame */
};

struct hstack_frame_t
{
	size_t prev_offset;
	char data[0];
};

hstack_t hstack_new ()
{
	return hstack_new2 (HSTACK_MIN_CAPACITY);
}

hstack_t hstack_new2 (size_t capacity)
{
	struct hstack_header_t* hstack;
	
	ASSERT (capacity > sizeof (struct hstack_frame_t));
	hstack = malloc (sizeof (struct hstack_header_t) + capacity);
	if (!hstack)
	{
		return NULL;
	}
	
	hstack->head = 0;
	hstack->capacity = capacity;
	
	struct hstack_frame_t* dummyframe = (struct hstack_frame_t*) (hstack + 1);
	dummyframe->prev_offset = 0; /* no previous elements */
	return hstack;
}

void hstack_delete(hstack_t hstack)
{
	free (hstack);
}

#define GET_TOPFRAME(hstack) (struct hstack_frame_t*) ((char*) ((struct hstack_header_t*) (hstack) + 1) \
	+ ((struct hstack_header_t*) hstack)->head)
#define GET_PREVFRAME(frame) (struct hstack_frame_t*) ((char*) (frame) \
	- ((struct hstack_frame_t*) (frame))->prev_offset)
#define GET_FRAMEDATA(frame) (&(frame)->data[0])

void* hstack_push2 (hstack_t* phstack, void* elem, size_t elsize)
{
	struct hstack_header_t* hstack = *phstack;

	const size_t need_capacity = hstack->head + 2 * sizeof (struct hstack_frame_t) + elsize;
	/* need check for overflow */
	if (need_capacity > hstack->capacity)
	{
		const size_t new_capacity = MAX(hstack->capacity * 2,
			hstack->head + 3 * sizeof (struct hstack_frame_t) + 2 * elsize);
			/* *2 to have some room for one more, *3 to fit a third dummy frame */
		
		hstack = realloc (hstack, sizeof (struct hstack_header_t) + new_capacity);
		if (!hstack)
		{
			return NULL; /* no mem */
		}
		
		hstack->capacity = new_capacity;
		*phstack = hstack; /* publish */
	}
	
	ASSERT (need_capacity <= hstack->capacity);
	struct hstack_frame_t* topframe = GET_TOPFRAME (hstack);
	size_t framesize = sizeof (struct hstack_frame_t) + elsize;
	struct hstack_frame_t* dummyframe = (struct hstack_frame_t*) ((char*) topframe + framesize);
	
	dummyframe->prev_offset = framesize;
	hstack->head += framesize;
	if (elem)
	{
		memcpy (GET_FRAMEDATA (topframe), elem, elsize);
	}
	
	return GET_FRAMEDATA (topframe);
}

void* hstack_push0 (hstack_t* phstack, size_t elsize)
{
	return hstack_push2 (phstack, NULL, elsize);
}

int hstack_pop2 (hstack_t* phstack)
{
	struct hstack_header_t* hstack = *phstack;
	struct hstack_frame_t* topframe = GET_TOPFRAME (hstack);
	
	if (topframe->prev_offset == 0)
	{
		return -1; /* empty stack */
	}
	
	ASSERT (hstack->head >= topframe->prev_offset);
	hstack->head = hstack->head - topframe->prev_offset;
	return 0;
}

void* hstack_top (chstack_t hstack, /* opt, out */ size_t* elsize)
{	
	const struct hstack_frame_t* topframe = GET_TOPFRAME (hstack);
	const size_t topelement_size = topframe->prev_offset;
	
	if (topelement_size == 0)
	{
		/* no previous frame */
		return NULL;
	}
	
	const struct hstack_frame_t* topelement = GET_PREVFRAME (topframe);
	void* data = (void*) GET_FRAMEDATA (topelement);
	
	if (elsize)
	{
		*elsize = topelement_size - sizeof (struct hstack_frame_t);
	}
	
	return data;
}

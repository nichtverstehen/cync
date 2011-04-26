#include <stdlib.h>
#include <stdio.h>

#include "mlist.h"

#define CHECK_FAIL(x, msg) do { if (!(x)) fail (msg); } while (0)

void fail (char* msg)
{
	fprintf (stderr, "FAILED: %s\n", msg);
	exit (1);
}

int compare_list (int* mlist, int* ref, size_t len)
{
	if (mlist_len(mlist) != len)
	{
		return -1;
	}
	
	return memcmp (mlist, ref, len * sizeof (int)) == 0 ? 0 : -1;
}

void test_mlist ()
{
	int i;
	int* l = mlist_new (int);
	CHECK_FAIL (l, "create list");
	
	CHECK_FAIL (mlist_len (l) == 0, "empty list");
	
	int a = 1;
	CHECK_FAIL (mlist_add (l, &a) == 0, "add element");
	CHECK_FAIL (mlist_len (l) == 1, "added element");
	CHECK_FAIL (l[0] == 1, "added element value");
	
	for (i = 2; i <= 16; ++i)
	{
		mlist_add (l, &i);
	}
	
	int as1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	CHECK_FAIL (compare_list (l, as1, countof (as1)) == 0, "add 16 items");
	
	mlist_remove (l, 0);
	int as4[] = { 16, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	CHECK_FAIL (compare_list (l, as4, countof (as4)) == 0, "remove first (1)");
	
	for (i = 5; i <= 13; ++i)
	{
		mlist_remove (l, 4);
	}
	
	int as2[] = { 16, 2, 3, 4, 7, 6 };
	CHECK_FAIL (compare_list (l, as2, countof (as2)) == 0, "remove 5-13");

	mlist_remove (l, 5);
	int as3[] = { 16, 2, 3, 4, 7 };
	CHECK_FAIL (compare_list (l, as3, countof (as3)) == 0, "remove last (16)");
	
	mlist_delete (l);
	
	struct X { int a, b; };
	struct X* xs = mlist_new (struct X);
	CHECK_FAIL (xs, "struct list");
	struct X x1 = { 1, 2 };
	CHECK_FAIL (0 == mlist_add (xs, &x1), "add struct");
	CHECK_FAIL (1 == mlist_len (xs), "added struct");
	CHECK_FAIL (0 == memcmp (&x1, &xs[0], sizeof (x1)), "struct valid");
	mlist_delete (xs);
}

int main ()
{
	test_mlist ();
	
	fprintf (stderr, "SUCCESS\n");
	exit (0);
}

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "mlist.h"
#include "hstack.h"
#include "async.h"
#include "async_io.h"
#include "async_rl.h"

#define CHECK(x, msg) do { if (x) ok (msg, __FUNCTION__, __LINE__); \
	else fail (msg, __FUNCTION__, __LINE__); } while (0)
#define CHECK_FAIL(x, msg) do { if (!(x)) fail (msg, __FUNCTION__, __LINE__); } while (0)

void ok (const char* msg, const char* fun, int line)
{
	fprintf (stderr, "PASSED: %s(%d). %s\n", fun, line, msg);
}

void fail (const char* msg, const char* fun, int line)
{
	fprintf (stderr, "FAILED: %s(%d). %s\n", fun, line, msg);
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
	CHECK (l, "create list");
	
	CHECK (mlist_len (l) == 0, "empty list");
	
	int a = 1;
	CHECK (mlist_add (l, &a) == 0, "add element");
	CHECK (mlist_len (l) == 1, "added element");
	CHECK (l[0] == 1, "added element value");
	mlist_remove (l, 0);
	CHECK (mlist_len (l) == 0, "remove single element");
	
	for (i = 1; i <= 16; ++i)
	{
		mlist_add (l, &i);
	}
	
	int as1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	CHECK (compare_list (l, as1, countof (as1)) == 0, "add 16 items");
	
	mlist_remove (l, 0);
	int as4[] = { 16, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	CHECK (compare_list (l, as4, countof (as4)) == 0, "remove first (1)");
	
	for (i = 5; i <= 13; ++i)
	{
		mlist_remove (l, 4);
	}
	
	int as2[] = { 16, 2, 3, 4, 7, 6 };
	CHECK (compare_list (l, as2, countof (as2)) == 0, "remove 5-13");
	
	int* l1 = mlist_clone (l);
	CHECK (compare_list (l1, as2, countof (as2)) == 0, "clone");
	mlist_delete (l1);

	mlist_remove (l, 5);
	int as3[] = { 16, 2, 3, 4, 7 };
	CHECK (compare_list (l, as3, countof (as3)) == 0, "remove last (16)");
	
	mlist_delete (l);
	
	struct X { int a, b; };
	struct X* xs = mlist_new (struct X);
	CHECK (xs, "struct list");
	struct X x1 = { 1, 2 };
	mlist_reserve (xs, 10);
	CHECK (10 == mlist_capacity (xs), "reserved 10");
	
	CHECK (0 == mlist_add (xs, &x1), "add struct");
	CHECK (1 == mlist_len (xs), "added struct");
	CHECK (0 == memcmp (&x1, &xs[0], sizeof (x1)), "struct valid");
	
	mlist_reserve (xs, 0);
	CHECK (1 == mlist_len (xs) && 0 == memcmp (&x1, &xs[0], sizeof (x1)), "non destructive reserve");
	
	mlist_delete (xs);
}

void test_hstack ()
{
	int x = { 5 };
	hstack_t st = hstack_new2 (64);
	CHECK (st, "new stack");
	CHECK (NULL == hstack_top (st, NULL), "empty stack top");
	CHECK (-1 == hstack_pop (st), "empty stack pop");
	
	size_t elsize;
	int32_t i1 = 17;
	int64_t i2 = 18;
	int32_t* pi1 = hstack_push (st, &i1);
	CHECK (pi1 && *pi1 == i1, "push i32");
	CHECK (pi1 == hstack_top (st, &elsize) && elsize == sizeof (int32_t), "pushed is top");
	CHECK (hstack_pop (st) == 0, "pop single");
	CHECK (NULL == hstack_top (st, NULL), "popped");
	
	int64_t arr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
	size_t i;
	for (i = 0; i < countof(arr); ++i)
	{
		CHECK_FAIL (hstack_push (st, &arr[i]) != NULL, "push element");
		int64_t* top = hstack_top (st, NULL);
		CHECK_FAIL (*top == arr[i], "pushed element");
	}
	
	for (i = 0; i < countof(arr); ++i)
	{
		int64_t* nth = hstack_nth(st, i, NULL);
		CHECK_FAIL (nth && *nth == arr[countof (arr) - i - 1], "nth");
	}
	
	pi1 = hstack_nth(st, countof(arr), NULL);
	CHECK (!pi1, "nth oob");
	
	for (i = countof(arr) - 1; i --> 0; )
	{
		CHECK_FAIL (hstack_pop (st) == 0, "pop element");
		int64_t* top = hstack_top (st, NULL);
		CHECK_FAIL (top && *top == arr[i], "popped element");
	}
	
	CHECK (hstack_pop (st) == 0, "pop last element");
	CHECK (hstack_top (st, NULL) == NULL, "push/pop array");
	
	hstack_delete (st);
}

/* ASYNC */

struct checker_args_t
{
	int checkpoint;
	hstack_t cont;
	hstack_t check;
};

#define checker_point(point) \
	if (*aArg.status != -1 && point == *aArg.status + 1) *aArg.status = point;

#define checker_step() \
	args = (void*) aRet; \
	checker_point (args->checkpoint); \
	args->check = aStack;

a_Function (async_checker, { int* status; })
{
	static const int expect[] = { 1, 2, 10, 1, 3 };
	struct checker_args_t* args;
	
	a_Begin;
	*aArg.status = 0;
	
	printf ("c1");
	checker_step();
	a_Continue (args->cont, 0);
	
	printf ("c2");
	checker_step();
	a_Continue (args->cont, 0);
	
	printf ("c3");
	checker_step();
	a_Continue (args->cont, 0);
	
	printf ("c4");
	checker_point(4);
	
	a_End;
}

a_Function (async_func, { hstack_t* checker; })
{
	a_Local {
		struct checker_args_t chargs;
	};
	
	a_Begin;
	
	printf ("f1");
	
	aLoc.chargs.checkpoint = 2; aLoc.chargs.cont = aStack;
	a_Continue (*aArg.checker, (intptr_t) &aLoc.chargs);
	*aArg.checker = aLoc.chargs.check;

	printf ("f2");
	
	a_EndR(13);
}

a_Function (async_tester, { hstack_t* checker; })
{
	a_Local {
		struct checker_args_t chargs;
	};
	
	a_Begin;
	
	printf ("m1");
	
	aLoc.chargs.checkpoint = 1; aLoc.chargs.cont = aStack;
	a_Continue (*aArg.checker, (intptr_t) &aLoc.chargs);
	*aArg.checker = aLoc.chargs.check;
	
	printf ("m2");
	
	a_Call (async_func, aArg.checker);
	
	printf ("m3");
	
	aLoc.chargs.checkpoint = aRet == 13 ? 3 : -1; aLoc.chargs.cont = aStack;
	a_Continue (*aArg.checker, (intptr_t) &aLoc.chargs);
	*aArg.checker = aLoc.chargs.check;
	
	printf ("m4");
	
	a_End;
}

void test_async ()
{
	int status = 0;
	hstack_t check_stack = hstack_new();
	async_init_stack (check_stack, async_checker, &status);
	
	hstack_t test_stack = hstack_new();
	async_init_stack (test_stack, async_tester, &check_stack);
	
	int test_status = async_run_stack (test_stack, NULL);
	int check_status = async_run_stack (check_stack, NULL);
	
	printf("\n");
	CHECK (0 == test_status, "async run test");
	CHECK (0 == check_status, "async run checker");
	
	CHECK (status == 4, "async finished");
}

/* IO */

a_Function (async_reader, { int fd; char* dest; int count; })
{
	a_Local {
		int have_read;
	};
	
	a_Begin;
	aLoc.have_read = 0;
	
	while (aLoc.have_read < aArg.count)
	{
		a_Call(async_read, aArg.fd, aArg.dest + aLoc.have_read, aArg.count - aLoc.have_read);
		if (aRet <= 0)
		{
			break;
		}
		aLoc.have_read += aRet;
	}
	
	a_EndR(aRet < 0 ? -1 : aLoc.have_read);
}


a_Function (async_writer, { int fd; const char* buf; int count; })
{
	a_Local {
		int have_written;
	};
	
	a_Begin;
	aLoc.have_written = 0;
	
	while (aLoc.have_written < aArg.count)
	{
		a_Call(async_write, aArg.fd, aArg.buf + aLoc.have_written, aArg.count - aLoc.have_written);
		if (aRet <= 0)
		{
			break;
		}
		aLoc.have_written += aRet;
	}
	
	a_EndR(aRet < 0 ? -1 : aLoc.have_written);
}

void test_io()
{
	static const char testline[] = "123456789\n123";
	int pipes[2];
	pipe (&pipes);
	int child = fork ();
	if (child == 0)
	{
		close (pipes[0]);
		sleep (1);
		write (pipes[1], testline, countof(testline));
		close (pipes[1]);
		exit (0);
	}
	close (pipes[1]);
	int fd = pipes [0];
	
	char str [129];
	hstack_t astack = hstack_new ();
	async_init_stack (astack, async_reader, fd, str, 128);
	
	int r = async_ioloop (astack);
	
	CHECK (r >= 0, "ioloop read");
	//CHECK (strncmp (testline, str, countof(testline)) == 0, "read data");
	
	str[128] = 0;
	printf ("%s\n", str);
	
	close (fd);
	waitpid(child, NULL, 0);
	
	/* writing */
	pipe (&pipes);
	child = fork ();
	if (child == 0)
	{
		close (pipes[1]);
		sleep (1);
		char buf[128];
		int r = read (pipes[0], buf, countof(buf));
		if (r > 0)
		{
			printf("%s\n", buf);
			if (0 != strncmp(testline, buf, r))
			{
				r = -2;
			}
		}
		
		close (pipes[0]);
		exit (r);
	}
	
	astack = hstack_new ();
	async_init_stack (astack, async_writer, pipes[1], testline, countof(testline));
	r = async_ioloop (astack);
	
	CHECK (r >= 0, "ioloop write");
	
	int result;
	waitpid(child, &result, 0);
	r = WEXITSTATUS(result);
	CHECK (r == countof(testline), "written data");
	
	close(fd);
}

/* ARL */
a_Function (async_readliner, { int fd; char* dest; int count; ssize_t* status; })
{
	a_Local {
		AMY* my;
	};
	
	a_Begin;
	aLoc.my = arl_fromfd(aArg.fd);
	
	a_Call(arl_readline, aLoc.my, aArg.dest, aArg.count);
	
	int status = aRet;
	if( aArg.status )
	{
		*aArg.status = status;
	}
	
	a_EndR(status);
}

void test_arl()
{
	static const char testline[] = "12345678901234567890123456789\n123";
	int pipes[2];
	pipe (&pipes);
	int child = fork ();
	if (child == 0)
	{
		close (pipes[0]);
		sleep (1);
		write (pipes[1], testline, countof(testline));
		close (pipes[1]);
		exit (0);
	}
	close (pipes[1]);
	int fd = pipes [0];
	
	char str [129];
	hstack_t astack = hstack_new ();
	ssize_t status;
	async_init_stack (astack, async_readliner, fd, str, 128, &status);
	
	int r = async_ioloop (astack);
	
	CHECK (r >= 0, "ioloop read");
	CHECK (status == 30 && strncmp (testline, str, 30) == 0, "read data");
	
	str[status] = 0;
	printf ("%s\n", str);
	
	close (fd);
	waitpid(child, NULL, 0);
}

int main (int argc, char** argv)
{
	test_mlist ();
	test_hstack ();
	test_async ();
	//test_io ();
	test_arl ();
	
	fprintf (stderr, "SUCCESS\n");
	exit (0);
}


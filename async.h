/* Cyril Nikolaev */
#ifndef async_h__
#define async_h__

#include <stdint.h>
#include "hstack.h"

typedef int line_t;
typedef int (*async_fun_t) (hstack_t*, void*);

struct async_callinfo_t 
{
	line_t line;
	async_fun_t fun;
	intptr_t ret;
	int has_locals;
};

/* empty global fallback locals struct */
struct async_locals_t_ { };

/* run frame on top of stack. acquires the stack
 returns async function result (-1, 0, 1), see below */
int async_run_stack (hstack_t stack, intptr_t* ret);

#define async_init_stack(hstack, fun, ...) do { \
		struct fun##_frame_t_ subframe = { { 0, (async_fun_t) &fun##_async_, 0, 1 }, { __VA_ARGS__ } }; \
		hstack_push (hstack, &subframe); \
		hstack_push0 (hstack, 0); /* dummy locals */ \
	} while (0)

#define aStack (*stack_)
#define aArg (frame_->usr)
#define aLoc (*locals_)
#define aRet (frame_->callee.ret)

#define a_Function(name, args) \
	struct name##_frame_t_ { \
		struct async_callinfo_t callee; \
		struct args usr; }; \
	int name##_async_ (hstack_t* stack_, struct name##_frame_t_* frame_)

#define a_FunctionH(name, args) /* args unused */ \
	int name##_async_ (hstack_t* stack_, struct name##_frame_t_* frame_)

#define a_Local \
	struct async_locals_t_
 
#define a_Begin \
	struct async_locals_t_* locals_ = hstack_top (*stack_, NULL); \
	switch (frame_->callee.line) { case 0:;\
		if (frame_->callee.has_locals) hstack_pop (*stack_); /* dummy locals */ \
		locals_ = hstack_push0 (*stack_, sizeof (struct async_locals_t_)); \
		if (!locals_) return -1;

#define a_Return(/* intptr_t */ x) \
	aRet = x; \
	hstack_pop (*stack_); \
	return 0;

#define a_EndR(/* intptr_t */ x) \
		aRet = x; \
		hstack_pop (*stack_); \
	} return 0;

#define a_End \
		hstack_pop (*stack_); \
	} return 0;

#define a_Call(fun, ...) { \
		frame_->callee.line = __LINE__; \
		struct fun##_frame_t_ subframe = { { 0, (async_fun_t) &fun##_async_, 0, 0 }, { __VA_ARGS__ } }; \
		if (!hstack_push (*stack_, &subframe)) return -1; \
		return 2; \
	}; case __LINE__:

/* take care of your stack */
#define a_Continue(hstack, ret) { \
		if (async_fixret (hstack, 0, ret) < 0) return -1; \
		frame_->callee.line = __LINE__; \
		*stack_ = hstack; \
		return 3; \
	}; case __LINE__:

#define a_Junction() { \
		frame_->callee.line = __LINE__; \
		return 1; \
	}; case __LINE__:

int async_fixret (hstack_t hstack, size_t depth, intptr_t ret);

/* async function return values:
 -1 - fatal error. stack in undefined state. stack is deallocated by engine
 0 - regular exit. frame of callee is still in stack. engine returns to caller
 1 - stop processing stack. assume callee acquired it
 2 - run frame on top of stack
 3 - continue with modified stack */

#endif

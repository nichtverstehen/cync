/* Cyril Nikolaev */
#ifndef async_h__
#define async_h__

#include <stdint.h>
#include "hstack.h"

typedef int line_t;
typedef struct { int status; } async_result_t;
typedef async_result_t (*async_fun_t) (hstack_t*, void*);

struct async_callinfo_t 
{
	line_t line;
	async_fun_t fun;
	intptr_t ret;
	int has_locals;
	const char* fname;
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

#define async_start(fun, /* int* */ pstatus, /* intptr_t* */ preturn, ...) do { \
		hstack_t newstack_ = hstack_new (); \
		async_init_stack(newstack_, fun, __VA_ARGS__); \
		*pstatus = async_run_stack(newstack_, preturn); \
	} while (0)

#define aStack (*stack_)
#define aArg (frame_->usr)
#define aLoc (*locals_)
#define aRet (frame_->callee.ret)

#define a_Function(name, args) \
	struct name##_frame_t_ { \
		struct async_callinfo_t callee; \
		struct args usr; }; \
	async_result_t name##_async_ (hstack_t* stack_, struct name##_frame_t_* frame_)

#define a_FunctionH(name, args) /* args unused */ \
	async_result_t name##_async_ (hstack_t* stack_, struct name##_frame_t_* frame_)

#define a_Local \
	struct async_locals_t_
 
#define a_Begin \
	struct async_locals_t_* locals_ = hstack_top (*stack_, NULL); \
	async_result_t result_ = { 0 }; \
	switch (frame_->callee.line) { case 0:;\
		if (frame_->callee.has_locals) hstack_pop (*stack_); /* dummy locals */ \
		locals_ = hstack_push0 (*stack_, sizeof (struct async_locals_t_)); \
		if (!locals_) { result_.status = -1; return result_; } \
		frame_ = hstack_nth(*stack_, 1, NULL);

#define a_Return(/* intptr_t */ x) \
	aRet = x; \
	hstack_pop (*stack_); \
	result_.status = 0; \
	return result_;

#define a_EndR(/* intptr_t */ x) \
		aRet = x; \
		hstack_pop (*stack_); \
	} \
	result_.status = 0; \
	return result_;

#define a_End \
		hstack_pop (*stack_); \
	} \
	result_.status = 0; \
	return result_;

#define a_Call(fun, ...) { \
		frame_->callee.line = __LINE__; \
		struct fun##_frame_t_ subframe = { { 0, (async_fun_t) &fun##_async_, 0, 0, #fun }, { __VA_ARGS__ } }; \
		if (!hstack_push (*stack_, &subframe)) { result_.status = -1; return result_; } \
		result_.status = 2; \
		return result_; \
	}; case __LINE__:

/* take care of your stack */
#define a_Continue(hstack, ret) { \
		if (async_fixret (hstack, 0, ret) < 0) { result_.status = -1; return result_; } \
		frame_->callee.line = __LINE__; \
		*stack_ = hstack; \
		result_.status = 3; \
		return result_; \
	}; case __LINE__:

#define a_Junction() { \
		frame_->callee.line = __LINE__; \
		result_.status = 1; \
		return result_; \
	}; case __LINE__:

int async_fixret (hstack_t hstack, size_t depth, intptr_t ret);

/* async function return values:
 -1 - fatal error. stack in undefined state. stack is deallocated by engine
 0 - regular exit. frame of callee is still in stack. engine returns to caller
 1 - stop processing stack. assume callee acquired it
 2 - run frame on top of stack
 3 - continue with modified stack */

#endif

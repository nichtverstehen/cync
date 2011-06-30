#include <assert.h>
#include "async.h"

#define ASSERT assert

int async_run_stack (hstack_t stack, intptr_t* ret)
{
	int status = 0;
	intptr_t last_ret = 0;
	
	struct async_callinfo_t* callee = hstack_nth (stack, 1, NULL);

	while (1)
	{
		if (!callee)
		{
			goto exit;
		}
		
		int r = callee->fun (&stack, callee);
		callee = hstack_nth (stack, r == 2 ? 2 : 0, NULL); /* stack may be reallocated */
		
		switch (r)
		{
		case -1:
			status = -1;
			goto exit;
			break;
		case 0: /* return */
			last_ret = callee->ret;
			hstack_pop (stack); /* pop just used frame */
			callee = hstack_nth (stack, 1, NULL);
				
			if (callee)
			{ /* pass the return value */
				callee->ret = last_ret;
			}
				
			break;
		case 1: /* junction */
			status = 1;
			goto exit;
			break;
		case 2: /* call */
			callee = hstack_top (stack, NULL);
			break;
		case 3: /* continue with */
			callee = hstack_nth (stack, 1, NULL);
			break;
		}
	}
	
exit:
	if (status != 1)
	{
		ASSERT (stack);
		hstack_delete(stack);
	}
	
	if (status == 0 && ret)
	{
		*ret = last_ret;
	}
	
	return status;
}

/* place a continuation return value in the top frame */
int async_fixret (hstack_t hstack, size_t depth, intptr_t ret)
{
	struct async_callinfo_t* callee = hstack_nth (hstack, depth*2 + 1, NULL);
	if (!callee)
	{
		return -1;
	}
	
	callee->ret = ret;
	return 0;
}

#include <assert.h>
#include "async.h"

#define ASSERT assert

int async_run_stack (hstack_t stack)
{
	int status = 0;
	
	struct async_callinfo_t* callee = hstack_nth (stack, 1, NULL);

	while (1)
	{
		if (!callee)
		{
			goto exit;
		}
		
		int r = callee->fun (&stack, callee);
		
		switch (r)
		{
		case -1:
			status = -1;
			goto exit;
			break;
		case 0:
			hstack_pop (stack); /* pop top frame */
			callee = hstack_nth (stack, 1, NULL);
			break;
		case 1:
			status = 1;
			goto exit;
			break;
		case 2:
			callee = hstack_top (stack, NULL);
			break;
		case 3:
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
	
	return status;
}

/* place a continuation return value in the top frame */
int async_fixret (hstack_t hstack, intptr_t ret)
{
	struct async_callinfo_t* callee = hstack_nth (hstack, 1, NULL);
	if (!callee)
	{
		return -1;
	}
	
	callee->ret = ret;
	return 0;
}

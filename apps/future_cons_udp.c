#include <xinu.h>
#include <future.h>

int future_cons_udp(future *fut)
{
	int i, status;
	while(1)
	{
		while(fut->state == FUTURE_EMPTY || fut->state == FUTURE_WAITING)
		{
		}
		
		status = future_get(fut, &i);
		if (status < 1) {
			printf("future_get failed\n");
			return SYSERR;
		}

		if(i == -1)
		{
			break;
		}
		
	  	printf("it consumed %d\n", i);
	}	
	if(!future_free(fut))
	{
		return SYSERR;
	}
	return OK;
}

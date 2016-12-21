#include <xinu.h>
#include <future.h>

syscall future_set(future *f, int *value)
{
	if(f == NULL || (f->state == FUTURE_VALID && (f->flag == FUTURE_EXCLUSIVE || f->flag == FUTURE_SHARED)))
	{
		return SYSERR;
	}
	else if(f->state == FUTURE_EMPTY && (f->flag == FUTURE_EXCLUSIVE || f->flag == FUTURE_SHARED))
	{
		f->value = value; 
		f->state = FUTURE_VALID;
	}
	else if(f->state == FUTURE_WAITING && (f->flag == FUTURE_EXCLUSIVE || f->flag == FUTURE_SHARED))
	{
		f->value = value;
		f->state = FUTURE_VALID;
		if(f->flag == FUTURE_EXCLUSIVE)
		{
			resume(f->pid);
		}
		else if(f->flag == FUTURE_SHARED)
		{
			while(!future_isempty(f->get_queue))
			{
				resume(future_dequeue(f->get_queue));
			}
		}
	}
	else if(f->flag == FUTURE_QUEUE)
	{
		if(f->state == FUTURE_EMPTY)
		{
			if(future_isempty(f->get_queue))
			{
				pid32 pid = getpid();
				future_enqueue(f->set_queue ,pid);
				suspend(pid);
				f->value = value;
			}
		}
		else if(f->state == FUTURE_WAITING)
		{
			f->value = value;
			resume(future_dequeue(f->get_queue));
		}
	}
	return OK;
}

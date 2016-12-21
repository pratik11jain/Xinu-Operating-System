#include <xinu.h>
#include <future.h>

syscall future_get(future *f, int *value)
{
	if(f == NULL || (f->state == FUTURE_WAITING && f->flag == FUTURE_EXCLUSIVE))
	{
		return SYSERR;
	}
	else if(f->state == FUTURE_VALID && (f->flag == FUTURE_EXCLUSIVE || f->flag == FUTURE_SHARED))
	{
		*value = *(f->value);
		f->state = FUTURE_EMPTY;
	}
	else if(f->state == FUTURE_EMPTY && f->flag == FUTURE_EXCLUSIVE)
	{
		f->pid = getpid();
		f->state = FUTURE_WAITING;
		suspend(f->pid);
		*value = *(f->value);
		f->state = FUTURE_EMPTY;
	}
	else if((f->state == FUTURE_WAITING || f->state == FUTURE_EMPTY) && f->flag == FUTURE_SHARED)
	{
		pid32 pid = getpid();
		future_enqueue(f->get_queue, pid);
		f->state = FUTURE_WAITING;
		suspend(pid);
		*value = *(f->value);
		if(future_isempty(f->get_queue)) f->state = FUTURE_EMPTY;
	}
	else if(f->flag == FUTURE_QUEUE)
	{
		if(future_isempty(f->set_queue))
		{
			pid32 pid = getpid();
			future_enqueue(f->get_queue, pid);
			f->state = FUTURE_WAITING;
			suspend(pid);
			*value = *(f->value);
			if(future_isempty(f->get_queue)) f->state = FUTURE_EMPTY;
		}
		else
		{
			resume(dequeue(f->set_queue));
			*value = *(f->value);
			if(future_isempty(f->get_queue)) f->state = FUTURE_EMPTY;
		}
	}
	return OK;
}

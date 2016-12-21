#include <xinu.h>
#include <future.h>
#include <future_queue.h>

future* future_alloc(int future_flag)
{
	future* f = (future*)getmem(sizeof(future));
	if(f == SYSERR)
	{
		return NULL;
	}
	f->flag = future_flag;
	f->state = FUTURE_EMPTY;
	if(future_flag == FUTURE_QUEUE)
	{
		f->set_queue = future_newqueue();
		f->get_queue = future_newqueue();
	}
	else if(future_flag == FUTURE_SHARED)
	{
		f->set_queue = NULL;
		f->get_queue = future_newqueue();
	}
	else
	{
		f->set_queue = NULL;
		f->get_queue = NULL;
	}
	return f;	
}

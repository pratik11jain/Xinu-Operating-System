#include <xinu.h>
#include <future.h>
#include <future_queue.h>

syscall future_free(future* f)
{
	if(f->set_queue != NULL)
	{
		freemem(f->set_queue, sizeof(struct future_qentry));
	}
	if(f->get_queue != NULL)
	{
		freemem(f->get_queue, sizeof(struct future_qentry));
	}
	return freemem(f, sizeof(future));
}

#include <xinu.h>
#include <future_queue.h>

queue future_newqueue(void)
{
	queue HEAD = (queue)getmem(sizeof(struct future_qentry));
	HEAD->process_id = MAXKEY;
	HEAD->previous = NULL;
	queue TAIL = (queue)getmem(sizeof(struct future_qentry));
	TAIL->process_id = MINKEY;
	TAIL->next = NULL;
	HEAD->next = TAIL;
	TAIL->previous = HEAD;
	return HEAD;
}

int future_isempty(queue HEAD)
{
	if(HEAD->next->process_id == MINKEY)
	{
		return 1;
	}
	return 0;
}

void future_enqueue(queue HEAD, pid32 pid)
{
	queue NEW_NODE = (queue)getmem(sizeof(struct future_qentry));
	NEW_NODE->process_id = pid;
	queue TAIL = HEAD;
	while(TAIL->next->process_id != MINKEY)
	{
		TAIL = TAIL->next;
	}	
	NEW_NODE->next = TAIL->next;
	NEW_NODE->previous = TAIL;
	TAIL->next->previous = NEW_NODE;
	TAIL->next = NEW_NODE;
}

pid32 future_dequeue(queue HEAD)
{
	if(future_isempty(HEAD)) return SYSERR;
	queue TEMP = HEAD->next;
	pid32 pid = TEMP->process_id;
	TEMP->next->previous = HEAD;
	HEAD->next = TEMP->next;
	freemem(TEMP,(sizeof(struct future_qentry)));
	return pid;
}

#include <xinu.h>

#ifndef _FUTURE_QUEUE_H_
#define _FUTURE_QUEUE_H_

struct future_qentry
{
	pid32 process_id;
	struct future_qentry* next;
	struct future_qentry* previous;
};

typedef struct future_qentry* queue;

queue future_newqueue(void);
int future_isempty(queue);
void future_enqueue(queue, pid32);
pid32 future_dequeue(queue);

#endif

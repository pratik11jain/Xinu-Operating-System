#include <xinu.h>
#include <prodcons.h>

void producer(int32 count) {
        for(int32 i = 0; i <= count; i++)
{
	wait(consumed);
	n = i;
        printf("Produce : %d\n", n);	
	signal(produced);
}
}




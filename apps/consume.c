#include <xinu.h>
#include <prodcons.h>

void consumer(int32 count) {
	for(int32 i = 0; i <= count; i++)
{
        wait(produced);
	printf("Consume : %d\n", n);	
        signal(consumed);
}
}

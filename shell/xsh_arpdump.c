#include <xinu.h>

shellcmd xsh_arpdump(int nargs, char *args[])
{
	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		printf("Usage: %s\n\n", args[0]);
		printf("Description:\n");
		printf("\tDisplays information from the ARP cache\n");
		printf("Options (one per invocation):\n");
		printf("\t--help\tdisplay this help and exit\n");
		return 0;
	}

	// code to print in form of a table
	printf("ARP cache:\n");
	printf("   State Pid    IP Address    Hardware Address	Timestamp\n");
	printf("   ----- ---  --------------  ----------------  ---------\n");
	
	// loop over all the entries in arpcache and print those which are not free
	for (int32 slot = 0; slot < ARP_SIZ; slot++)
	{		
		struct arpentry* arptr = &arpcache[slot];
		
		// check if the entry is free, if yes, continue to next entry
		if(arptr->arstate == AR_FREE)
		{
			continue;
		}
		else if(arptr->arstate == AR_PENDING)
		{
			printf("   PEND "); // pending state
		}
		else if(arptr->arstate == AR_RESOLVED)
		{
			printf("   RESLV"); // resolved state
		}
		else
		{
			printf("   UNKWN"); // unknown state
		}
		
		// check is there is any waiting process
		if(arptr->arstate == AR_PENDING)
		{
			printf("%4d ", arptr->arpid);
		} 
		else
		{
			printf("     ");
		}
		
		// print ip address of the entry
		printf("%3d.", (arptr->arpaddr & 0xFF000000) >> 24);
		printf("%3d.", (arptr->arpaddr & 0x00FF0000) >> 16);
		printf("%3d.", (arptr->arpaddr & 0x0000FF00) >> 8);
		printf("%3d",  (arptr->arpaddr & 0x000000FF));

		// print ethernet address of the entry
		printf(" %02X", arptr->arhaddr[0]);
		for(int32 ethadd = 1; ethadd < ARP_HALEN; ethadd++)
		{
			printf(":%02X", arptr->arhaddr[ethadd]);
		}
		
		printf("     ");
		printf("%d", arptr->timestamp);
		printf("\n");
	}

	// return 0 for successful termination
	return 0;
}

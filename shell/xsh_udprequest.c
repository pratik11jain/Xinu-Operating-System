#include <xinu.h>
#include <prodcons.h>
#include <future.h>

shellcmd xsh_udprequest(int nargs, char *args[])
{
	if((nargs == 2) && strncmp(args[1],"--help",7)== 0)
	{
		printf("Usage: %s\n\n", args[0]);
		printf("Description:\n");
		printf("\tCommand To Communicate With Linux Server Using UDP (User Datagram Protocol)\n");
		printf("Options (one per invocation):\n");
		printf("\tserverip :\tip address of the server\n");
		printf("\tportnumber :\tport number of the server\n");	
		printf("\t-f :\tfuture mode\n");
		printf("\t--help\tdisplay this help and exit\n");
		return 0;
	}
	
	if(nargs < 3)
	{
		fprintf(stderr, "%s: too few arguments\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n", args[0]);
		return 1;
	}
	
	if(nargs == 3 && strcmp(args[2], "-f") == 0)
	{
		fprintf(stderr, "%s: too few arguments for futures mode\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n", args[0]);
		return 1;
	}

	int future_flag = 0;
	if(nargs == 4 && strcmp(args[3], "-f") == 0)
	{
		future_flag = 1;
	}

	if(nargs > 4 || (nargs == 4 && strcmp(args[3],"-f") != 0))
	{
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n", args[0]);
		return 1;
	}	

	uint32 serverIp;
	if(dot2ip(args[1], &serverIp) == SYSERR)
	{
		printf( "%s: invalid IP address\n", args[0]);
		return 1;
	}
		
	int portNo = atoi(args[2]);

	int localPortNo = 3000;
	int slot = udp_register(serverIp, portNo, localPortNo);
	
	if (slot == SYSERR) 	
	{
		printf("udprequest : Error In Binding To Local Port No. %d\n", localPortNo);
		return 1;
	}

	if(future_flag)	
	{	
		future* fExlusive = future_alloc(FUTURE_EXCLUSIVE);

		if(fExlusive != NULL )
		{
			resume(create(future_cons_udp, 1024, 20, "fcons", 1, fExlusive));
	
			while(1)
			{
				while(fExlusive->state == FUTURE_VALID)
				{
				}
				
				printf(">");
				
				char message[512];
				fgets(message, 512, CONSOLE);
				
				int msgLength = strlen(message);
				message[(msgLength - 1)] = '\0';

				if(strcmp(message, "quit") == 0)
				{
					break;
				}
				
				int returnValue = udp_send(slot, message, msgLength);
				if(returnValue == SYSERR)
				{
					printf("udprequest : Error In Sending Datagram Packet\n");
					udp_release(slot);
					return SYSERR;
				}
	
				char buffer[512];
				returnValue = udp_recv(slot, buffer, sizeof(buffer), 3000);
				if(returnValue == SYSERR || returnValue == TIMEOUT)
				{ 	
					printf("udprequest : Error In Receiving Datagram Packet\n");
					udp_release(slot);
					return SYSERR;
				}
				
				int intValue = atoi(buffer);
				printf("Producer : %d\n", intValue);
				
				int status = future_set(fExlusive, &intValue);
  				if (status == SYSERR)
  				{
    				printf("udprequest : Future Set Failed\n");
    				udp_release(slot);
					return SYSERR;
 				}
				
				memset(buffer, '\0', sizeof(buffer));
				memset(message, '\0', sizeof(message));
			}
		}
		udp_release(slot);

		if(!(future_free(fExlusive)))
		{
			return SYSERR;
		}
	}	
	else
	{
		while(1)
		{
			printf(">");
			char message[512];
			fgets(message, 512, CONSOLE);
			
			int msgLength = strlen(message);
			message[(msgLength - 1)] = '\0';

			if(strcmp(message, "quit") == 0)
			{
				break;
			}
			
			int returnValue = udp_send(slot, message, msgLength);
			if(returnValue == SYSERR)
			{
				printf("udprequest : Error In Sending Datagram Packet\n");
				udp_release(slot);
				return SYSERR;
			}
				
			char buffer[512];
			returnValue = udp_recv(slot, buffer, sizeof(buffer), 3000);
			if(returnValue == SYSERR || returnValue == TIMEOUT)
			{ 	
				printf("udprequest : Error In Receiving Datagram Packet\n");
				udp_release(slot);
				return SYSERR;
			}

			printf("Length of %s is  %s \n", message, buffer);
			memset(buffer, '\0', sizeof(buffer));
			memset(message, '\0', sizeof(message));
		}
		udp_release(slot);
	}	
	return 0;
}

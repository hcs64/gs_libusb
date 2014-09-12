#ifndef __COMMS_H__
#define __COMMS_H__

#include <stdio.h>
#include <stdlib.h>
#include "gscomms.h"

/*common errors*/
int erratta(int code);
int SendCommand(gscomms * g, char command);

/*generic commands*/
#define AR_DISCONNECT	'd'
#define AR_END		'e'
#define AR_RAM_DUMP		127

int SendCommand(gscomms * g, char command) {
	if (!InitGSComms(g, RETRIES)) return 1;
  if (!Handshake(g, 1)) return 1;
	ReadWriteByte(g, command);
	return 0;
}

int erratta(int code)
{if(code<1) return 0;
else if(code==1) fprintf(stderr,"\nTimeout!  Can not interface with I/O port.\n");
else if(code==2) fprintf(stderr,"\nGS menu detected.  Start normal play or exit the GS menus, then try again\n");
else if(code==3) fprintf(stderr,"\nUnable to close comms.\n");
else fprintf(stderr,"\nAborting...\n");
exit(code);
}
#endif

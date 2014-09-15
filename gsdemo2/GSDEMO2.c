#include <stdio.h>
#include <stdlib.h>
#include "gscomms.h"
#include "COMMS.H"
#include "TRAINER.H"

/*demo upload/download of gs active codelist*/

int main(){
unsigned short val;
unsigned long target;
unsigned long num;
FILE *tmp;

gscomms * g = NULL;

g = setup_gscomms();
set_mode(g, GSCOMMS_MODE_CAREFUL);

#if 0
if (!InitGSCommsNoisy(g, RETRIES, 1)) {
  printf("Init failed\n");
  do_clear(g);
  return 1;
}
#endif

printf("Adding four active codes to list.\nPress [Enter] to continue...");
getchar();
if(AddActiveCode(g,0x81400000,0x0123)) erratta(1);
if(AddActiveCode(g,0x81400002,0x4567)) erratta(1);
if(AddActiveCode(g,0x81400004,0x89AB)) erratta(1);
if(AddActiveCode(g,0x81400006,0xCDEF)) erratta(1);
num=NumActiveCodes(g);
printf("\nDone.  %li codes in list.\n",num);

printf("\n\nDemonstrating a delete function.\n");
printf("#%d will be deleted.\nPress [Enter] to continue...",(int)(num-2));
getchar();
if(RubActiveCode(g,0x81400002)) printf("\nError!  RubActiveCode returned something funny...");
num=NumActiveCodes(g);
printf("\nDone.  %li codes in list.\n",num);

printf("\nNow to read active codes.\nPress [Enter] to continue...");
getchar();
tmp=tmpfile();
target=PrintActiveCodes(g,tmp);
if(target!=num) printf("\nError!  PrintActiveCodes returned %li, not %li.\n",target,num);
for(val=0;val<target;val++) {
	fseek(tmp,val*8,SEEK_SET);
	fread(&num,4,1,tmp);
	printf("\n\t%i\t%08lX ",val+1,num);
	fread(&num,4,1,tmp);
	printf("%04lX",num&0xFFFF);
}
printf("\nDone.  Press [Enter] to continue.");
getchar();

printf("\n\nNext to toggle the code lists.");
printf("\nThey were %s", GetCodeState(g) ? "off":"on");
printf(" and now they are %s", SetCodeState(g,-1) ? "off":"on");

// Disable this test for now as the editor doesn't seem to work with this
// modification.
#if 0
/*extend GSmemory editor range and set initial location*/
if(!InitGSComms(g, RETRIES)) erratta(1);
if(!Handshake(g, 1)) erratta(1);
ReadWriteByte(g, 2);
WriteAddr16(g,0xA0791DF6,0);
WriteAddr16(g,0xA0791E02,0);
WriteAddr16(g,0xA0791E32,0);
WriteAddr16(g,0xA0791E36,0);
WriteAddr16(g,0xA0791E06,0xFFFF);
WriteAddr16(g,0xA0791E3A,0xFFFF);
WriteAddr32(g,0xA07E9C90,0x80400006);
WriteAddr32(g,0xA07E9CA8,0x80400006);
WriteAddr16(g,0x80400006,0);
ReadWrite32(g,0);
ReadWrite32(g,0);
ReadWriteByte(g,0);
Disconnect(g);
printf("\nThe PC has been disconnected.  Enter Memory Editor from the GS menu.\n");
printf("Go to 80400006.  It should read '0000', not 'CDEF'.\n");
printf("Press [Enter] after closing the GS menus to resume testing.\n");
getchar();
#endif

printf("\n\nFinally, mop up by clearing the active code list.");
if(EraseAllCodes(g)) erratta(1);
Disconnect(g);
printf("\nDone.  The code list has been deleted.");
printf("\nPress [Enter] to quit.");
getchar();

return 0;}

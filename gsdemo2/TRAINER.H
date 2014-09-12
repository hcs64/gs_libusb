#ifndef __GS_AR_TRAINER_H__
#define __GS_AR_TRAINER_H__

#include <stdio.h>
#include <stdlib.h>
#include "gscomms.h"

/*Defines for each of the commands*/
#define TR_READ		1
#define TR_WRITE		2
#define TR_ADD_CODE		'i'
#define TR_NUM_CODE		'j'
#define TR_SUB_CODE		'k'
#define TR_PRINT_CODES	'p'

/*returns from the Add Active Code feature*/
#define AAC_LIMIT	0xFE
#define AAC_FOUND	0xFF
#define AAC_OK	0

/*these are simple read/write types that do not initiate or close the comms
between read/write, send address & length 0 and reinitiate*/
unsigned long ReadAddr32(gscomms * g, unsigned long address);
unsigned short ReadAddr16(gscomms * g, unsigned long address);
unsigned char ReadAddrByte(gscomms * g, unsigned long address);
int WriteAddr32(gscomms * g, unsigned long address, unsigned long value);
int WriteAddr16(gscomms * g, unsigned long address, unsigned short value);
int WriteAddrByte(gscomms * g, unsigned long address, unsigned char value);

/*Gameshark™ Code Generator List functions*/
char AddActiveCode(gscomms * g, unsigned long address, unsigned long data);
char NumActiveCodes(gscomms * g);
int  RubActiveCode(gscomms * g, unsigned long address);
int  EraseAllCodes(gscomms * g);
int  PrintActiveCodes(gscomms * g, FILE * infile);
/*int screenshot(gscomms * g, FILE * image);*/
int GetCodeState(gscomms * g);
int SetCodeState(gscomms * g, int mode);	/*0-on, 1-off, -1-toggle; returns new state*/


/*Open for reading/writing*/
#define InitRead(g) SendCommand(g, TR_READ)
#define InitWrite(g) SendCommand(g, TR_WRITE)

/*these are simple read/write types as above, but don't initiate or close the comms
  These are slightly faster, but the others are more robust*/
unsigned long ReadAddr32(gscomms * g, unsigned long address) {
	ReadWrite32(g, address);
	ReadWrite32(g, 4);
	return ReadWrite32(g, 0);
}


unsigned short ReadAddr16(gscomms * g, unsigned long address) {
	ReadWrite32(g, address);
	ReadWrite32(g, 2);
	return ReadWrite16(g, 0);
}

unsigned char ReadAddrByte(gscomms * g, unsigned long address)
{ReadWrite32(g, address);
ReadWrite32(g, 1);
return ReadWriteByte(g, 0);
}


int WriteAddr32(gscomms * g, unsigned long address,unsigned long value) {
	ReadWrite32(g, address);
	ReadWrite32(g, 4);
	ReadWrite32(g, value);
	return 0;
}

int WriteAddr16(gscomms * g, unsigned long address,unsigned short value) {
	ReadWrite32(g, address);
	ReadWrite32(g, 2);
	ReadWrite16(g, value);
	return 0;
}

/*add active code to active code list
  note that data is sent as a word, though actual data is a short*/
char AddActiveCode(gscomms * g, unsigned long address, unsigned long data) {
	if(SendCommand(g, TR_ADD_CODE)) return -1;
	ReadWrite32(g, address);
	ReadWrite32(g, data);
	return ReadWriteByte(g, 0);
}

/*these are defined as macros, though you really don't need to*/
char NumActiveCodes(gscomms * g) {
	if(SendCommand(g, TR_NUM_CODE)) return -1;
	return ReadWriteByte(g, 0);
}

int RubActiveCode(gscomms * g, unsigned long address) {
	if(SendCommand(g, TR_SUB_CODE)) return -1;
	ReadWrite32(g, address);
	return 0;
	}

/*erases all active codes from list, because man it's iritating otherwise*/
int EraseAllCodes(gscomms * g) {
	if(InitWrite(g)) return -1;
	WriteAddr32(g, 0xA07E9D60,0);
	ReadWrite32(g, 0);
	ReadWrite32(g, 0);
  ReadWriteByte(g, 0);
	return 0;
}


/*Prints active codelist as binary; returns number of codes
  might want a more generic stream vresion of this*/
int PrintActiveCodes(gscomms * g, FILE * infile){
	unsigned long c=0,x,y,z;
	
	if(SendCommand(g, TR_PRINT_CODES)) return -1;
	
	c=ReadWrite32(g, 0);
	/*leave it to caller for general init. and command*/
	for(z=c;c>0;c--){
		x=ReadWrite32(g, 0);
		y=ReadWrite32(g, 0);
		fwrite(&x,4,1,infile);
		fwrite(&y,4,1,infile);
		}
	return z;
}

/*TODO: int screenshot(gscomms * g, FILE * image)*/

/*used to return state of codelist
  0-on, 1-off.  -1 is a read error*/
int GetCodeState(gscomms * g){
int mode;
if (InitRead(g)) return -1;
	mode=ReadAddr32(g, 0xA07E9C04);
	ReadWrite32(g, 0);
return mode;}

/*Sets state of codelist 0-on, 1-off, -1-toggle current mode
  returns the new state or -1 if an error occured*/
int SetCodeState(gscomms * g, int mode){
if((mode<0) | (mode>1))
	{mode=GetCodeState(g);
	if(mode<0) return -1;
	mode^=1;
	}

if (InitWrite(g)) return -1;
	WriteAddr32(g,0xA07E9C04,mode);
	ReadWrite32(g,0);
	ReadWriteByte(g,0);

return GetCodeState(g);}

#endif

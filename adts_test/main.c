/*
**
** filename: main.c
**
*/



#include <stdio.h>
#include "adts.h"
#include <stdint.h>




int main()
{

	ADTSContext ctx;
	uint8_t header[7];

	unsigned char profile;
	unsigned char sambling_index;
	unsigned char channel;

	setADTSContext(&ctx, 1, 8000, 1);

	adts_write_frame_header(&ctx, header, 500, 0);
	
	//header[0] ?= 0xff;
	if(header[0] == 0xff)
	{
		printf("header[0] = 0xff\r\n");
	}
	else
	{
		printf("header[0] =%x\r\n",header[0]);
	}

	if((header[1]&0xF0) == 0xf0)
	{
		printf("header[1]&0xf0 = 0xf0\r\n");
	}
	else
	{
		printf("header[1]&0xf0 = %x\r\n",(header[1]&0xf0));
	}

	profile = (header[2]&0xC0)>>6;

	printf("profile=%x\r\n",profile);// 1

	sambling_index = (header[2]&0x3C) >> 2;
	printf("sample index=%x\r\n",sambling_index);
	channel = ((header[2]&0x01)<<2) | ((header[3]&0xC0)>>6);

	printf("channel=%x\r\n",channel);

	unsigned short frame_length
		= ((header[3]&0x03)<<11)| (header[4]<<3)|((header[5]&0xE0)>>5);

	
	printf("frame_lenght=%u\r\n",frame_length);
	


	
	
	

	return 0;
}

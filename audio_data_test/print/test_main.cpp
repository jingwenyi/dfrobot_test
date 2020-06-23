

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "audio_analyses_algorithm.h"

int file_count = 1;

void callfunc(unsigned char *buffer, int len)
{
	char buf[50] = {0};
	sprintf(buf, "file1_cut_out_voice%d.pcm", file_count);
	FILE *fd = fopen(buf, "wb+");
	fwrite(buffer, 1, len, fd);
	fclose(fd);
	file_count++;
}

int main()
{
	unsigned char buf[320];
	printf("input filename:\r\n");
	char pcm_buffer[200];
	scanf("%s",pcm_buffer);
	printf("user input:%s\r\n",pcm_buffer);
	//FILE *pcm_file = fopen("test16k6.pcm", "rb+");
	FILE *pcm_file = fopen(pcm_buffer, "rb+");
	audio_analyze_init(&callfunc);

	while(!feof(pcm_file))
	{
		fread(buf, 1, 320, pcm_file);
		audio_frame_analyze1(buf, 320);
	}

	audio_analyze_exit();
	
	return 0;
}





#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>






int main()
{
	FILE *pcm_file = fopen("test16k6.pcm", "rb+");
	FILE *fd = fopen("test16k6.txt", "wb+");

	unsigned char *sample = (unsigned char *)malloc(2);
	char buffer[50];
	
	while(!feof(pcm_file))
	{
		short int *samplenum = NULL;
		fread(sample, 1,2, pcm_file);

		samplenum = (short *)sample;
		//printf("%d\t",*samplenum);
		//usleep(500);
		sprintf(buffer,"%d\r\n",*samplenum);
		fwrite(buffer, 1, strlen(buffer), fd);
		
	}
	printf("\r\n");
	free(sample);
	fclose(pcm_file);
	fclose(fd);
	return 0;
}




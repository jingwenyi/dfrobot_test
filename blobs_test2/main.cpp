#include <stdio.h>
#include "blobs.h"
#include "qqueue.h"
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>



#define RAW_RGB_1280X720	(1280*720*3)


Qqueue *g_qqueue;
Blobs *g_blobs;




static Frame8 g_rawFrame;

uint8_t *tmp;





int main()
{

	g_qqueue = new Qqueue();
	g_blobs = new Blobs(g_qqueue);

	tmp = new uint8_t[RAW_RGB_1280X720+54];

	long fd = open("red1280x720.bmp",O_RDONLY);
	if(fd<=0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	read(fd, tmp, RAW_RGB_1280X720+54);
	close(fd);
	g_rawFrame.m_pixels = &tmp[54];
	g_rawFrame.m_height = 360;
	g_rawFrame.m_width = 640;

	struct timeval tvs;
	unsigned long t0, t1;
	
	gettimeofday(&tvs, NULL);
	t0 = tvs.tv_sec*1000+tvs.tv_usec/1000;
	
	g_blobs->rls(&g_rawFrame);
    g_blobs->unpack();
	g_blobs->getBlob();
	BlobA *blobs;
	uint32_t numBlobs;
	g_blobs->getBlobs(&blobs, &numBlobs);

	gettimeofday(&tvs, NULL);
	t1 = tvs.tv_sec*1000+tvs.tv_usec/1000;
	printf("rgb time :%ldms\r\n",(t1-t0));


	delete g_qqueue;
	delete g_blobs;
	delete tmp;
	return 0;
}

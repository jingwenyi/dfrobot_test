//auther jingwenyi 2016.5.4

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>
#include "blob.h"
#include "blobs.h"
#include "qqueue.h"
#include "pixytypes.h"

#define YUV420P_1280X720  1280*720*3/2  //1382400

Qqueue *g_qqueue;
Blobs *g_blobs;


int main()
{
	uint8_t* yuv_frame;
	uint8_t signum;

	Frame8 g_rawFrame;

	ColorSignature *sig;

	int i;

	g_qqueue = new Qqueue();
	g_blobs = new Blobs(g_qqueue);

	yuv_frame = new uint8_t[YUV420P_1280X720];
	long fd = open("DC004101.yuv",O_RDONLY);
	if(fd<=0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	read(fd,yuv_frame,YUV420P_1280X720);
	close(fd);

	//伪造一个像素矩形
	RectA region(540,225,161,164);
	//RectA region(2,10,161,164);

	g_rawFrame.m_pixels = yuv_frame;
	g_rawFrame.m_width = 1280;
	g_rawFrame.m_height = 720;

	//产生一个识别标志
	signum = 1;  //把红色加载到1号
	g_blobs->m_clut->generateSignature(g_rawFrame,region,signum);
	sig = g_blobs->m_clut->getSignature(signum);
	
	//标记rgb 颜色立方体
	g_blobs->m_clut->setSignature(signum,*sig);
	g_blobs->m_clut->generateLUT();

	//开始识别
	g_blobs->rls_yuv(&g_rawFrame);
	g_blobs->unpack();
	g_blobs->getBlob();

	BlobA *blobs;
	uint32_t numBlobs;
	g_blobs->getBlobs(&blobs,&numBlobs);

	for(i=0;i<numBlobs;i++)
	{
		printf("sig=%d,left=%d,right=%d,top=%d,bottom=%d\r\n",
			blobs[i].m_model,blobs[i].m_left,blobs[i].m_right,
			blobs[i].m_top,blobs[i].m_bottom);
	}

	return 0;
}



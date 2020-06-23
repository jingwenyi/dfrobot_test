/*
** author jingwenyi create 2016.04.06 for test blobs
*/


#include <stdio.h>
#include "colorlut.h"
#include "pixytypes.h"
#include "SDL/SDL.h"
#include "qqueue.h"
#include "blob.h"
#include "blobs.h"


Qqueue *g_qqueue;
Blobs *g_blobs;


void print_colorSignature(ColorSignature* sig);



int main()
{

	uint8_t signum;
	Frame8 g_rawFrame;
	ColorSignature *sig;
	int i;

	g_qqueue = new Qqueue();
	g_blobs = new Blobs(g_qqueue);

	

	SDL_Surface* bmp_data = NULL;
	
	//伪造一个像素矩形
	RectA region(241,221,46,38);
	//start SDL
	SDL_Init(SDL_INIT_EVERYTHING);

#if 1
	
	//load image
	bmp_data= SDL_LoadBMP("redall.bmp");

	
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	if(g_rawFrame.m_pixels == NULL)
	{
		printf("No raw frame in memory!\r\n");
		goto out;
	}
	
	//产生一个识别标志
	signum = 1;  //把红色加载到1号
	g_blobs->m_clut->generateSignature(g_rawFrame,region,signum);
	sig = g_blobs->m_clut->getSignature(signum);

	//print_colorSignature(sig);

	//标记rgb 颜色立方体
	g_blobs->m_clut->setSignature(signum,*sig);
	g_blobs->m_clut->generateLUT();
#endif

#if 1

	//把绿色加载在2 号颜色

	bmp_data= SDL_LoadBMP("blueall.bmp");

	
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	if(g_rawFrame.m_pixels == NULL)
	{
		printf("11No raw frame in memory!\r\n");
		goto out;
	}

	signum = 2;  
	g_blobs->m_clut->generateSignature(g_rawFrame,region,signum);
	sig = g_blobs->m_clut->getSignature(signum);

	//print_colorSignature(sig);

	//标记rgb 颜色立方体
	g_blobs->m_clut->setSignature(signum,*sig);
	g_blobs->m_clut->generateLUT();
#endif

#if 0 //test
	i=4096;
	while(i--)
	{
		if(g_blobs->m_lut[i]!=0)
		{
			printf("m_lut[%d]=%x\r\n",i,g_blobs->m_lut[i]);
		}
	}
#endif
	//加载要识别的图片
	//bmp_data= SDL_LoadBMP("red-3.bmp");
	bmp_data= SDL_LoadBMP("hunhe.bmp");
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	if(g_rawFrame.m_pixels == NULL)
	{
		printf("22No raw frame in memory!\r\n");
		goto out;
	}

	//开始识别
	g_blobs->rls(&g_rawFrame);
	g_blobs->unpack();
	//g_blobs->print_blob();
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
	
	
out:
	SDL_FreeSurface(bmp_data);
	SDL_Quit();
		
	delete g_qqueue;
	delete g_blobs;
	
	return 0;
}



void print_colorSignature(ColorSignature* sig)
{
	printf("m_uMin:%x\r\n",sig->m_uMin);
	printf("m_uMax:%x\r\n",sig->m_uMax);
	printf("m_uMean:%x\r\n",sig->m_uMean);
	printf("m_vMin:%x\r\n",sig->m_vMin);
	printf("m_vMax:%x\r\n",sig->m_vMax);
	printf("m_vMean:%x\r\n",sig->m_vMean);
	printf("m_rgb:%x\r\n",sig->m_rgb);
	printf("m_type:%x\r\n",sig->m_type);
}



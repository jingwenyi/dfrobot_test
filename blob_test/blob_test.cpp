/*
** author jingwenyi create 2016.3.22 for test blob
**
*/



#include <stdio.h>
#include "blob.h"
#include <unistd.h>
#include "SDL/SDL.h"
#include <pthread.h>
#include "pixytypes.h"



#define NUM_MODELS            3





int main()
{
	int i;
	Frame8 g_rawFrame;
	SDL_Surface* bmp_data = NULL;
	SDL_Init(SDL_INIT_EVERYTHING);
	
	//load image
	bmp_data= SDL_LoadBMP("red-3.bmp");
	//bmp_data= SDL_LoadBMP("red-1.bmp");

	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	
#if 0 //jwy 测试下内存数据
			RGBPixel rgb;
			uint8_t* tmp = g_rawFrame.m_pixels;
			/*
			//uint8_t alpha=0;
			for(i=0; i<(g_rawFrame.m_width*g_rawFrame.m_height); i++)
			{
				alpha=0;
				rgb.m_b = *(tmp++);
				rgb.m_g = *(tmp++);
				rgb.m_r = *(tmp++);
				//alpha=*(tmp++);
				if((rgb.m_b != 0xff) && (rgb.m_g));
				printf("r:%x,g:%x,b:%x\r\n",rgb.m_r,rgb.m_g,rgb.m_b);
				
			}
			*/

			int x,y;
			for(y=0;y<g_rawFrame.m_height;y++)
			{
				for(x=0;x<g_rawFrame.m_width;x++)
				{
					rgb.m_b = *(tmp++);
					rgb.m_g = *(tmp++);
					rgb.m_r = *(tmp++);
					if((rgb.m_b != 0xff) && (rgb.m_g!=0xff) && (rgb.m_r!=0xff))
						printf("r:%x,g:%x,b:%x;x=%d,y=%d\r\n",rgb.m_r,rgb.m_g,rgb.m_b,x,y);
				}
			}
			
			
#endif	 //测试内存

#if 1   //测试blob

	SSegment s_red;
	int signum = 1;

	CBlobAssembler m_assembler[NUM_MODELS];

	for (i=0; i<NUM_MODELS; i++)  
        m_assembler[i].Reset();

	RGBPixel sig_blobs[NUM_MODELS]; 
	//添加识别红色
	RGBPixel rgb(0xED,0x1C,0x24);
	//set_sig_blobs(&rgb,signum);
	sig_blobs[signum-1].m_b = rgb.m_b;
	sig_blobs[signum-1].m_g = rgb.m_g;
	sig_blobs[signum-1].m_r = rgb.m_r;


	
	
	//printf("r=%x,g=%x,b=%x\r\n",sig_blobs[0].m_r,sig_blobs[0].m_g,sig_blobs[0].m_b);

	RGBPixel rgb_tmp;
	uint8_t* tmp = g_rawFrame.m_pixels;
	int x,y;
	s_red.model=1;
	int flag=0;
	
	for(y=0;y<g_rawFrame.m_height;y++)
	{
		s_red.row = 0;
		s_red.startCol = 0;
		s_red.endCol = 0xffff;
		flag =0;
		for(x=0;x<g_rawFrame.m_width;x++)
		{
			rgb_tmp.m_b = *(tmp++);
			rgb_tmp.m_g = *(tmp++);
			rgb_tmp.m_r = *(tmp++);

			if((rgb_tmp.m_r == 0xED) && (rgb_tmp.m_g==0x1C) && (rgb_tmp.m_b==0x24))
			{
				if(flag==0)
				{
					flag=1;
					s_red.row=y;
					s_red.startCol = x;
					s_red.endCol = x;
				}
				s_red.endCol=x;
			}
			else
			{
				if(flag==1)
				{
					flag=0;
					//printf("s_red.row=%d,s_red.startCol=%d,s_red.endCol=%d\r\n",s_red.row,s_red.startCol,s_red.endCol);
					m_assembler[s_red.model-1].Add(s_red);
				}
			}
			
			
			//if((rgb.m_b != 0xff) && (rgb.m_g!=0xff) && (rgb.m_r!=0xff))
						//printf("r:%x,g:%x,b:%x;x=%d,y=%d\r\n",rgb.m_r,rgb.m_g,rgb.m_b,x,y);
			
		}
	}
			
	for (i=0; i<NUM_MODELS; i++)
    {
    	//把当前活跃的blob 放到完成的list 中
        m_assembler[i].EndFrame();
		//对完成的数据进行分类
        m_assembler[i].SortFinished();
    }

	//取出识别到的blob
	CBlob *blob;
	uint16_t left, top, right, bottom;

	for(i=0; i<NUM_MODELS; i++)
	{
		blob=m_assembler[i].finishedBlobs;
		while(blob)
		{
			
			blob->getBBox((short &)left, (short &)top, (short &)right, (short &)bottom);
			printf("i+1=%d,left=%d,top=%d,right=%d,bottom=%d\r\n",i+1,left,top,right,bottom);
			blob=blob->next;
		}
		
	}

#endif

	SDL_FreeSurface(bmp_data);
	SDL_Quit();
	
	return 0;
}





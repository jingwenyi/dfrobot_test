/*
** author jingwenyi create 2016.03.18
**
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "SDL/SDL.h"
#include "types.h"

Frame8 g_rawFrame;

int log_fd = 0;


#define BT_CENTER_SIZE 6



void interpolateBayer(uint32_t width, uint32_t x, uint32_t y, uint8_t *pixel, uint32_t &r, uint32_t &g, uint32_t &b)
{
    if (y&1)
    {
        if (x&1)
        {
            r = *pixel;
            g = *(pixel-1);
            b = *(pixel-width-1);
        }
        else
        {
            r = *(pixel-1);
            g = *pixel;
            b = *(pixel-width);
        }
    }
    else
    {
        if (x&1)
        {
            r = *(pixel-width);
            g = *pixel;
            b = *(pixel-1);
        }
        else
        {
            r = *(pixel-width-1);
            g = *(pixel-1);
            b = *pixel;
        }
    }
}

#if 1


void getColor(uint8_t *r, uint8_t *g, uint8_t *b)
{

	char buf[100];
	
	uint32_t x, y, R, G, B, rsum, gsum, bsum, count;
	uint8_t *frame = g_rawFrame.m_pixels;  // use the correct pointer

	//jingwenyi 这个for 循环是遍历所有的像素点，把rgb值累加
	for (rsum=0, gsum=0, bsum=0, count=0, y=0; y<g_rawFrame.m_height; y++)
	{
		for (x=0; x<g_rawFrame.m_width; x++, count++)
		{
			interpolateBayer(g_rawFrame.m_width, x, y, frame+g_rawFrame.m_width*y+x, R, G, B);
		 	rsum += R;
			gsum += G;
			bsum += B;

			memset(buf,0,100);
			sprintf(buf,"r:%x,g:%x,b:%x\r\n",R,G,B);
			write(log_fd,buf,strlen(buf));
			
		}
	}
	//求出平均的rgb 值，所以需要颜色纯净点物体，识别效果最好
	*r = rsum/count;
	*g = gsum/count;										 
	*b = bsum/count;	 	
}


#endif


//该函数实现循环打印rgb的值

void print_rgb()
{
	RGBPixel rgb;
	uint8_t* tmp = g_rawFrame.m_pixels;
	int i=0;
	char buf[100];

#if 0
	//int y,x;
	uint32_t x, y, R, G, B;
	
	for(y=1;y<g_rawFrame.m_height-1;y++)
	{
		tmp++;
		for(x=1;x<g_rawFrame.m_width-1;x++,tmp++)
		{
			 interpolateBayer((uint32_t)g_rawFrame.m_width, x, y, tmp, R, G, B);
			memset(buf,0,100);
			sprintf(buf,"r:%x,g:%x,b:%x\r\n",R,G,B);
			write(log_fd,buf,strlen(buf));
		}
		tmp++;
	}

#endif

#if 1
	for(i=0; i<(g_rawFrame.m_width*g_rawFrame.m_height); i++)
	{
		rgb.m_b = *(tmp++);
		rgb.m_g = *(tmp++);
		rgb.m_r = *(tmp++);

		if((rgb.m_b!=0xff) ||(rgb.m_g != 0xff) || (rgb.m_r != 0xff) )
		{
			memset(buf,0,100);
			sprintf(buf,"r:%x,g:%x,b:%x\r\n",rgb.m_r,rgb.m_g,rgb.m_b);
			write(log_fd,buf,strlen(buf));
		}
		//printf("r:%x,g:%x,b:%x\r\n",rgb.m_r,rgb.m_g,rgb.m_b);
	}

#endif

#if 0
	for(i=0;i<(g_rawFrame.m_width*g_rawFrame.m_height);i++)
	{
		memset(buf,0,100);
		sprintf(buf,"%x\t",*(tmp++));
		write(log_fd,buf,strlen(buf));
	}
#endif
}



int main()
{

	int i=0;
	
	SDL_Surface* bmp_data = NULL;
	SDL_Surface* screen = NULL;

	log_fd = open("log.txt",O_CREAT|O_RDWR,0777);
	if(log_fd<0)
	{
		printf("open log file failed\r\n");
	}

	//start SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	//set up screen
	screen = SDL_SetVideoMode(1152,648,32,SDL_SWSURFACE);

	
	//load image
	bmp_data= SDL_LoadBMP("red.bmp");

	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	

	

	
	
	printf("width:%d,height:%d\r\n",g_rawFrame.m_width,g_rawFrame.m_height);
	
	//uint8_t r, g, b;

	//getColor(&r, &g, &b);
	//printf("----r:%x,g:%x,b:%x\r\n",r,g,b);
	print_rgb();

	//把红色该为绿色并显示
#if 0
	uint8_t* tmp = (uint8_t *)(bmp_data->pixels);
	for(i=0; i<(g_rawFrame.m_width*g_rawFrame.m_height); i++)
	{
		*(tmp++) = 0x4c;
		*(tmp++) = 0xb1;
		*(tmp++) = 0x22;
	}

#endif
	
	

	//apply image to screen
	SDL_BlitSurface(bmp_data,NULL,screen,NULL);

	//update screen
	SDL_Flip(screen);
	
	SDL_Delay(2000);

	SDL_FreeSurface(bmp_data);
	SDL_Quit();

	close(log_fd);
	return 0;
}





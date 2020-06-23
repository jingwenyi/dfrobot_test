
// author jingwenyi 2016.04.11


#include <stdio.h>
#include "SDL/SDL.h"
#include <sys/file.h>
#include <stdlib.h>
#include <unistd.h>
#include "yuv420pTorgb24.h"




#define YUV420P_1280X720  1280*720*3/2  //1382400

#define RGB24_1280X720    1280*720*3




int main()
{
	int ret=0;
	uint8_t* rgb24;
	uint8_t* one_frame;

	int i;
	
	rgb24 = new uint8_t[RGB24_1280X720];
	one_frame = new uint8_t[YUV420P_1280X720];

	

	SDL_Surface* bmp_data = NULL;
	SDL_Surface* screen = NULL;
	
	//start SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	

	//set up screen
	screen = SDL_SetVideoMode(1280,720,24,SDL_SWSURFACE);

	//load image
	bmp_data= SDL_LoadBMP("red1280x720.bmp");
	
	


	
	//long fd = open("DC011923.yuv",O_RDONLY);
	long fd = open("DC000237.yuv",O_RDONLY);
	if(fd<=0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	read(fd,one_frame,YUV420P_1280X720);
	close(fd);

	ret = yuv420p2rgb24(one_frame, rgb24, 1280, 720);

	
#if 0  //²âÊÔÊý¾Ý
	uint8_t* tmp = rgb24;
	for(i=0; i<(1280*720); i++)
	{
		printf("b=%d\t",*(tmp++));
		printf("g=%d\t",*(tmp++));
		printf("r=%d\n",*(tmp++));
	}

#endif

	uint8_t* tmp_rgb = rgb24;
	uint8_t* tmp_bmp = (uint8_t*)bmp_data->pixels;

	for(i=0; i<1280*720*3; i++)
	{
		*(tmp_bmp++) = *(tmp_rgb++);
	}
	

	
	
	
	//apply image to screen
	SDL_BlitSurface(bmp_data,NULL,screen,NULL);

	//update screen
	SDL_Flip(screen);
	SDL_Delay(2000);


	SDL_FreeSurface(bmp_data);
	SDL_FreeSurface(screen);
	
	SDL_Quit();
	delete []rgb24;
	delete []one_frame;
	
	printf("hello world\r\n");
	return 0;
}

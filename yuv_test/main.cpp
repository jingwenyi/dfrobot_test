/*
** author jingwenyi create 2016.4.8
*/


#include <stdio.h>
#include "SDL/SDL.h" 
#include <sys/file.h>
#include <stdlib.h>
#include <unistd.h>

#define YUV420P_1280X720  1280*720*3/2  //1382400




uint8_t one_frame[YUV420P_1280X720] = {0};

int main()
{
	int ret=0;
	
	//long fd = open("DC011923.yuv",O_RDONLY);
	long fd = open("DC000237.yuv",O_RDONLY);
	if(fd<=0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	read(fd,one_frame,YUV420P_1280X720);
	close(fd);

	ret = SDL_Init(SDL_INIT_VIDEO);
	if(ret == -1)
	{
		printf("could not initialize sdl\r\n");
		return -1;
	}

	SDL_Surface *sdl_screen = NULL;
	sdl_screen = SDL_SetVideoMode(1280,720,0,0);
	if(sdl_screen == NULL)
	{
		printf("sdl screen set video mode\r\n");
	}
	SDL_Overlay *sdl_overlay = SDL_CreateYUVOverlay(1280,720,SDL_YV12_OVERLAY,sdl_screen);
	if(sdl_overlay == NULL)
	{
		printf("create sdl voerlay error\r\n");
	}

	SDL_Rect video_rect;
	video_rect.x = 0;
	video_rect.y = 0;
	video_rect.w = 1280;
	video_rect.h = 720;


	
	SDL_LockYUVOverlay(sdl_overlay);
	
	// YYYYYYYYYYY......VVVVVVVV........UUUUUUUUU
	memcpy(sdl_overlay->pixels[0], one_frame, 1280*720);//y
	memcpy(sdl_overlay->pixels[1], &one_frame[1280*720+1280*720/4], 1280*720/4); //u
	memcpy(sdl_overlay->pixels[2], &one_frame[1280*720], 1280*720/4); //v
	
	
	SDL_UnlockYUVOverlay(sdl_overlay);
	printf("22222222222222\r\n");

	SDL_DisplayYUVOverlay(sdl_overlay,&video_rect);

	SDL_Delay(10000);

	SDL_Flip(sdl_screen);
	printf("3333333333\r\n");

	

	SDL_FreeYUVOverlay(sdl_overlay);

	
	printf("hello world\r\n");
	return 0;
}


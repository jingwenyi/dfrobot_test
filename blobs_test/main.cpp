/*
** author jingwenyi create 2016.03.23 for test blobs
**
*/

#include <stdio.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include <pthread.h>
#include "qqueue.h"
#include "blobs.h"





SDL_Surface* bmp_data = NULL;
SDL_Surface* bmp_data1 = NULL;


SDL_Surface* screen = NULL; //画布

Qqueue *g_qqueue;
Blobs *g_blobs;
Frame8 g_rawFrame;
Frame8 g_rawFrame1;//加载第二幅图的图片



int cp_picture_mem_to_qqueue();

#if 0

void handleLine(uint8_t *line, uint16_t width)
{
    uint32_t index, sig, sig2, usum, vsum, ysum;
    int32_t x, r, g1, g2, b, u, v, u0, v0;
    Qval newline;
    // new line
    g_qqueue->enqueue(&newline);
    x = 1;

next:
    usum = vsum = ysum = 0;
    r = line[x];
    g1 = line[x-1];
    g2 = line[x-width];
    b = line[x-width-1];
    u = r-g1;
    v = b-g2;
    ysum += r + (g1+g2)/2 + b;
    usum += u;
    vsum += v;

    u0 = u>>(9-CL_LUT_COMPONENT_SCALE);
    v0 = v>>(9-CL_LUT_COMPONENT_SCALE);
    u0 &= (1<<CL_LUT_COMPONENT_SCALE)-1;
    v0 &= (1<<CL_LUT_COMPONENT_SCALE)-1;
    index = (u0<<CL_LUT_COMPONENT_SCALE) | v0;
    sig = m_lut[index];

    x += 2;
    if (x>=width)
        return;

    if (sig==0)
        goto next;

    r = line[x];
    g1 = line[x-1];
    g2 = line[x-width];
    b = line[x-width-1];
    u = r-g1;
    v = b-g2;
    ysum += r + (g1+g2)/2 + b;
    usum += u;
    vsum += v;

    u0 = u>>(9-CL_LUT_COMPONENT_SCALE);
    v0 = v>>(9-CL_LUT_COMPONENT_SCALE);
    u0 &= (1<<CL_LUT_COMPONENT_SCALE)-1;
    v0 &=(1<<CL_LUT_COMPONENT_SCALE)-1;
    index = (u0<<CL_LUT_COMPONENT_SCALE) | v0;
    sig2 = m_lut[index];

    x += 2;
    if (x>=width)
        return;

    if (sig==sig2)
        goto save;

    goto next;

save:
    Qval qval(usum, vsum, ysum, (x/2<<3) | sig);
    g_qqueue->enqueue(&qval);
    x += 2;
    if (x>=width)
        return;
    goto next;
}

#endif



//加载识别标志，我这里先直接从内存传给它
int cc_loadLut(ColorSignature *psig)
{
	int signum = 1;
	g_blobs->m_clut->setSignature(signum, *psig);
	g_blobs->m_clut->generateLUT();
	// go ahead and flush since we've changed things
	g_qqueue->flush();
	return 0;
}


int cc_init(ColorSignature *psig)
{
	
	g_qqueue = new Qqueue();
	g_blobs = new Blobs(g_qqueue);
	cc_loadLut(psig);
	return 0;
}





//把图片显示在画布上
void show_picture(SDL_Surface* data)
{
	//apply image to screen
	SDL_BlitSurface(data,NULL,screen,NULL);
	SDL_Flip(screen);

	SDL_Delay(2000); //延时才能看到画面
	//SDL_FreeSurface(screen);
}

void init_sdl()
{
	//start SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	screen = SDL_SetVideoMode(1152,648,32,SDL_SWSURFACE);
}


void* pack_fun(void *arg)
{
	uint8_t * mem;
	int memSize = 0xc0000;
	Qval *qval = NULL;
	Qval end(0, 0, 0, 0xffff);
	int i = 0;
	int size=g_rawFrame1.m_height*g_rawFrame1.m_width;
	mem =  g_rawFrame1.m_pixels;
#if 0
	g_qqueue->flush();
	for(i=0;i<size;i+sizeof(Qval))
	{
		qval = (Qval*)(mem+i);
		g_qqueue->enqueue(qval);
	}
	g_qqueue->enqueue(&end); //给一个强制结束标志
#endif

	int y=0;
	Qval eof(0,0,0,0xffff);
	for(y=1;y<(uint32_t)g_rawFrame1.m_height;y+=2)
	{
		printf("y=%d\r\n",y);
		g_blobs->handleLine(g_rawFrame1.m_pixels+y*g_rawFrame1.m_width, g_rawFrame1.m_width);
	}
	g_blobs->m_qq->enqueue(&eof);
	
}


void* work_fun(void *arg)
{
	BlobA *blobs;
	uint32_t numBlobs;
	int i=0;
	//while(1)
	{
		//printf("work_fun\r\n");
		// create blobs
		g_blobs->blobify();
		// send blobs
		g_blobs->getBlobs(&blobs, &numBlobs);
		printf("numBlobs:%d\r\n",numBlobs);
		//for(i=0;i<numBlobs;i++)
		{
			
		}
	}
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


//将图片内存数据拷贝到qqueue 中
int cp_picture_mem_to_qqueue()
{
	uint8_t * mem;
	int memSize = 0xc0000;
	mem =  g_rawFrame1.m_pixels;
	//g_qqueue->flush();
	g_qqueue->readAll((Qval *)mem, memSize);
	
}


int main()
{
	pthread_t pack_queue;
	pthread_t work_thread;
	int ret;
	int *thread_ret = NULL;
	ColorSignature *sig;
	int i;

	
	
	init_sdl();

	
	//load image
	bmp_data= SDL_LoadBMP("redall.bmp");



	
#if 1	//先生成一个colorSignature
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

#if 0 //jwy 测试下内存数据
	RGBPixel rgb;
	uint8_t* tmp = g_rawFrame.m_pixels;
	uint8_t alpha=0;
	for(i=0; i<(g_rawFrame.m_width*g_rawFrame.m_height); i++)
	{
		alpha=0;
		rgb.m_b = *(tmp++);
		rgb.m_g = *(tmp++);
		rgb.m_r = *(tmp++);
		alpha=*(tmp++);
		printf("r:%x,g:%x,b:%x,alpha:%x\r\n",rgb.m_r,rgb.m_g,rgb.m_b,alpha);
		
	}
	
#endif	 //测试内存

	if(g_rawFrame.m_pixels == NULL)
	{
		printf("No raw frame in memory!\r\n");
		return -1;
	}
	//伪造一个像素矩形
	RectA region(240,220,46,38);
	uint8_t *m_lut;
	ColorLUT *m_clut;
	uint8_t signum;
	uint32_t type;
	signum = 1;
	type = CL_MODEL_TYPE_COLORCODE;

	m_lut = new uint8_t[CL_LUT_SIZE];
	m_clut = new ColorLUT(m_lut);

	//create lut
	m_clut->generateSignature(g_rawFrame,region,signum);
	sig = m_clut->getSignature(signum);
	sig->m_type = type;
	
	
	// find average RGB value
	IterPixel ip(g_rawFrame, region);
	sig->m_rgb = ip.averageRgb();

	print_colorSignature(sig);

#endif //colorSignature

	cc_init(sig);

	//加载要识别的图片
	bmp_data1= SDL_LoadBMP("hunhe.bmp");
	g_rawFrame1.m_pixels = (uint8_t *)(bmp_data1->pixels);
	g_rawFrame1.m_width = bmp_data1->w;
	g_rawFrame1.m_height = bmp_data1->h;


#if 0  //jwy test show picture

	SDL_Surface* redbmp_data = NULL;
	redbmp_data= SDL_LoadBMP("redall32.bmp");
	show_picture(redbmp_data);

	//SDL_Delay(5000);
	SDL_Surface* bluebmp_data = NULL;
	bluebmp_data= SDL_LoadBMP("blueall32.bmp");
	show_picture(bluebmp_data);

	SDL_FreeSurface(redbmp_data);
	
	SDL_FreeSurface(bluebmp_data);

#endif //show picture


	//开两个线程，一个线程把图片数据读入qqueue 中，另一个线程处理

	ret = pthread_create(&pack_queue,NULL,pack_fun,NULL);
	if(ret != 0)
	{
		printf("Create thread1 error!\r\n");
		return -1;
	}
	sleep(1); //等待pack_fun 运行
	ret = pthread_create(&work_thread,NULL,work_fun,NULL);
	if(ret != 0)
	{
		printf("Create thread2 error!\r\n");
		return -1;
	}

	//pause();
	show_picture(bmp_data);
	show_picture(bmp_data1);

	

	pthread_join(pack_queue,(void**)&thread_ret);
	pthread_join(work_thread,(void**)&thread_ret);
	SDL_FreeSurface(screen);
	SDL_FreeSurface(bmp_data);
	SDL_Quit();


	delete g_qqueue;
	delete g_blobs;
	
	return 0;
}




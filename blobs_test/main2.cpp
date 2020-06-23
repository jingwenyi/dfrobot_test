#include <stdio.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include <pthread.h>
#include "qqueue.h"
#include "blobs.h"
#include "main.h"

Qqueue *g_qqueue;
Blobs *g_blobs;

Frame8 g_rawFrame;
Frame8 g_rawFrame1;

#define pixels_size  	(1152*648*4)
#define restore_w		(1152*2)
#define restore_h		(648*2)

void show_picture(SDL_Surface* data,SDL_Surface* screen);
void print_colorSignature(ColorSignature* sig);
int cc_loadLut(ColorSignature *psig);

void interpolateBayer(uint32_t width, uint32_t x, uint32_t y, uint8_t *pixel, uint32_t &r, uint32_t &g, uint32_t &b);


//������Ҫ��һ���ܴ�Ŀռ������RGB �����˳��
uint8_t *tmp_new_rgb = new uint8_t[pixels_size];

uint8_t *tmp_new_rgb_test = new uint8_t[pixels_size];




/*
**		this is pixy pixels store style
**			b  	|g2	|b	|g2......
**			g1 	|r	|g1	|r........
**		wo do not know that what is g2,so i think g1=g,g2=0.
*/

void install_new_rgb(uint8_t *tmp_new_rgb,uint32_t x, uint32_t y,uint32_t r, uint32_t g, uint32_t b)
{
	//��r��ֵ
	*(tmp_new_rgb+restore_w*y+x) = r;
	//��g1��ֵ
	*(tmp_new_rgb+restore_w*y+x - 1)= g;
	//��g2��ֵ
	*(tmp_new_rgb+restore_w*(y-1)+x)= g;
	//��b ��ֵ
	*(tmp_new_rgb+restore_w*(y-1)+x -1)= b;
}

void restore_rgb(Frame8 *rawFrame,uint8_t *tmp_new_rgb)
{
	uint32_t x,y;
	RGBPixel rgb;
	int i=0;
	uint8_t* tmp = rawFrame->m_pixels;
	for(i=0,y=1,x=1; i<(rawFrame->m_width*rawFrame->m_height); i++)
	{
		//��ȡһ��pixels
		rgb.m_b = *(tmp++);
		rgb.m_g = *(tmp++);
		rgb.m_r = *(tmp++);
		//�Ѷ�ȡ��pixels ����Ϊpixy ��rgb ģʽ
		
		install_new_rgb(tmp_new_rgb,x,y,rgb.m_r,rgb.m_g,rgb.m_b);
		x+=2;
		if(x>=restore_w)
		{
			x=1;
			y+=2;
			if(y>restore_h)
				printf("error --raw:%d is too many!\r\n",y);
		}
	}
	printf("raw=%d\r\n",y);
	

}


int main()
{
	uint32_t numBlobs, numCCBlobs;
    BlobA *blobs;
    BlobB *ccBlobs;
    uint32_t numQvals, *qVals;

	ColorSignature *sig;

	g_qqueue = new Qqueue();
	g_blobs = new Blobs(g_qqueue);

	SDL_Surface* bmp_data = NULL;
	SDL_Surface* bmp_data1 = NULL;
	SDL_Surface* screen = NULL; //����

	SDL_Init(SDL_INIT_EVERYTHING);
	//screen = SDL_SetVideoMode(1152,648,24,SDL_SWSURFACE);

	//load image
	bmp_data= SDL_LoadBMP("redall.bmp");

#if 0
	//�����rgb �Ĳ���yuv �ģ�ֻ��Ϊ�˲����´���
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w; //1152
	g_rawFrame.m_height = bmp_data->h; //648
	printf("w=%d,h=%d\r\n",g_rawFrame.m_width,g_rawFrame.m_height);
	//���԰�rgb ���ݰ���pixy �����ݴ�������
	
	restore_rgb(&g_rawFrame,tmp_new_rgb);
#endif

#if 1  //zongde

#if 1	//������һ��colorSignature
		g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
		g_rawFrame.m_width = bmp_data->w;
		g_rawFrame.m_height = bmp_data->h;

		restore_rgb(&g_rawFrame,tmp_new_rgb);
	
#if 0 //jwy �������ڴ�����
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
		
#endif	 //�����ڴ�
	
		if(tmp_new_rgb == NULL)
		{
			printf("No raw frame in memory!\r\n");
			return -1;
		}
		//α��һ�����ؾ���
		RectA region(200,110,30,50);
		uint8_t *m_lut;
		ColorLUT *m_clut;
		uint8_t signum;
		uint32_t type;
		signum = 1;
		//type = 0;//CL_MODEL_TYPE_COLORCODE;
	
		m_lut = new uint8_t[CL_LUT_SIZE];
		m_clut = new ColorLUT(m_lut);

		Frame8 temprawFrame;
		temprawFrame.m_pixels = tmp_new_rgb;
		temprawFrame.m_height = restore_h;
		temprawFrame.m_width = restore_w;
	
		//create lut
		m_clut->generateSignature(temprawFrame,region,signum);
		sig = m_clut->getSignature(signum);
		//sig->m_type = type;
		
		
		// find average RGB value
		IterPixel ip(temprawFrame, region);
		sig->m_rgb = ip.averageRgb();
	
		print_colorSignature(sig);
	
#endif //colorSignature


	//��ʼ����ʶ��Ķ���
	cc_loadLut(sig);	

	//����Ҫʶ���ͼƬ
	bmp_data1= SDL_LoadBMP("redall.bmp");
	g_rawFrame1.m_pixels = (uint8_t *)(bmp_data1->pixels);
	g_rawFrame1.m_width = bmp_data1->w;
	g_rawFrame1.m_height = bmp_data1->h;

	restore_rgb(&g_rawFrame1,tmp_new_rgb_test);

	if(tmp_new_rgb_test == NULL)
	{
		printf("No raw frame in memory!\r\n");
		return -1;
	}
	
	Frame8 temprawFrame1;
	temprawFrame1.m_pixels = tmp_new_rgb_test;
	temprawFrame1.m_height = restore_h;
	temprawFrame1.m_width = restore_w;
	

	//ʶ��Ĵ���
	g_blobs->rls(&temprawFrame1);
	g_blobs->blobify();
	g_blobs->getBlobs(&blobs,&numBlobs,&ccBlobs,&numCCBlobs);
	//g_blobs->getRunlengths(&qVals,&numQvals);
	printf("numBlobs=%d\r\n",numBlobs);
#endif //zongde

	//��ʾͼƬ
	//show_picture(bmp_data,screen);
	

out:  //�ͷ���Դ
	
	//SDL_FreeSurface(screen);
	SDL_FreeSurface(bmp_data);
	SDL_Quit();
	
	delete g_qqueue;
	delete g_blobs;

	return 0;
}



//��ͼƬ��ʾ�ڻ�����
void show_picture(SDL_Surface* data,SDL_Surface* screen)
{
	//apply image to screen
	SDL_BlitSurface(data,NULL,screen,NULL);
	SDL_Flip(screen);

	SDL_Delay(2000); //��ʱ���ܿ�������
	//SDL_FreeSurface(screen);
}



//����ʶ���־����������ֱ�Ӵ��ڴ洫����
int cc_loadLut(ColorSignature *psig)
{
	int signum = 1;
	g_blobs->m_clut->setSignature(signum, *psig);
	g_blobs->m_clut->generateLUT();
	// go ahead and flush since we've changed things
	g_qqueue->flush();
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



void interpolateBayer(uint32_t width, uint32_t x, uint32_t y, uint8_t *pixel, uint32_t &r, uint32_t &g, uint32_t &b)
{
    if (y&1)//������
    {
        if (x&1) //������
        {
            r = *pixel;
            g = *(pixel-1);
            b = *(pixel-width-1);
        }
        else//ż����
        {
            r = *(pixel-1);
            g = *pixel;
            b = *(pixel-width);
        }
    }
    else //ż����
    {
        if (x&1)//������
        {
            r = *(pixel-width);
            g = *pixel;
            b = *(pixel-1);
        }
        else //ż����
        {
            r = *(pixel-width-1);
            g = *(pixel-1);
            b = *pixel;
        }
    }
}





/*
** author jingwenyi create 2016.03.22 for test colorLUT
*/

#include <stdio.h>
#include "colorlut.h"
#include "pixytypes.h"
#include "SDL/SDL.h"
#include "qqueue.h"
#include "blob.h"

#define CL_LUT_COMPONENT_SCALE          6
#define CL_LUT_SIZE                     (1<<(CL_LUT_COMPONENT_SCALE*2))
#define CL_MODEL_TYPE_COLORCODE         1

#define NUM_MODELS            7


void handleLine(uint8_t *line, uint16_t width);
void rls(const Frame8 *frame);
void unpack();




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


uint8_t *m_lut;
Frame8 g_rawFrame;
Qqueue *g_qqueue;
CBlobAssembler m_assembler[NUM_MODELS];




int main()
{
	int i=0;
	
	ColorLUT *m_clut;
	uint8_t signum;
	uint32_t type;

	g_qqueue = new Qqueue();

	for (i=0; i<NUM_MODELS; i++)  
        m_assembler[i].Reset();

	signum = 1;
	type = CL_MODEL_TYPE_COLORCODE;


	SDL_Surface* bmp_data = NULL;
	SDL_Surface* screen = NULL;

	m_lut = new uint8_t[CL_LUT_SIZE];
	m_clut = new ColorLUT(m_lut);

	//伪造一个像素矩形
	RectA region(241,221,46,38);

	//start SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	
	//set up screen
	//screen = SDL_SetVideoMode(1152,648,32,SDL_SWSURFACE);

	//load image
	bmp_data= SDL_LoadBMP("redall.bmp");

	
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	if(g_rawFrame.m_pixels == NULL)
	{
		printf("No raw frame in memory!\r\n");
		return -1;
	}
	ColorSignature *sig;

	//create lut
	m_clut->generateSignature(g_rawFrame,region,signum);
	sig = m_clut->getSignature(signum);
	sig->m_type = type;

	print_colorSignature(sig);


	m_lut = new uint8_t[CL_LUT_SIZE];
    m_clut = new ColorLUT(m_lut);

	m_clut->setSignature(signum,*sig);
	m_clut->generateLUT();

#if 0 //test
	i=4096;
	while(i--)
	{
		if(m_lut[i]!=0)
		{
			printf("m_lut[%d]=%x\r\n",i,m_lut[i]);
		}
	}
#endif

	bmp_data= SDL_LoadBMP("red-3.bmp");
	g_rawFrame.m_pixels = (uint8_t *)(bmp_data->pixels);
	g_rawFrame.m_width = bmp_data->w;
	g_rawFrame.m_height = bmp_data->h;

	rls(&g_rawFrame);

	unpack();

	//取出识别到的blob
	CBlob *blob;
	uint16_t left, top, right, bottom;

	for(i=0; i<NUM_MODELS; i++)
	{
		blob=m_assembler[i].finishedBlobs;
		while(blob)
		{
			
			blob->getBBox((short &)left, (short &)top, (short &)right, (short &)bottom);
			printf("left=%d,top=%d,right=%d,bottom=%d\r\n",left,top,right,bottom);
			blob=blob->next;
		}
		
	}

	
	
	

	
	//apply image to screen
	//SDL_BlitSurface(bmp_data,NULL,screen,NULL);
	
	//update screen
	//SDL_Flip(screen);
		
	//SDL_Delay(2000);
	
	SDL_FreeSurface(bmp_data);
	SDL_Quit();
	
	delete [] m_lut;
	delete m_clut;
	delete g_qqueue;
	
	
	return 0;
}



//qval
//	11 bits  | 11 bits		| 3 bits |
//    endCol |  startCol    | model |

void handleLine(uint8_t *line, uint16_t width)
{
	uint32_t index,sig,sig2,u, v,startcol,endcol;
	int32_t x,r,g,b;
	Qval newline=0;
	// new line
    g_qqueue->enqueue(&newline);
	x=0;
	sig=0;
	startcol=0;
	endcol=0;

next:
	u = v = 0;
	b=line[x];
	g=line[x+1];
	r=line[x+2];

	u = r-g;
    u >>= 9-CL_LUT_COMPONENT_SCALE;
    u &= (1<<CL_LUT_COMPONENT_SCALE)-1;
    v = b-g;
    v >>= 9-CL_LUT_COMPONENT_SCALE;
    v &= (1<<CL_LUT_COMPONENT_SCALE)-1;
	index = (u<<CL_LUT_COMPONENT_SCALE)| v;
	sig2 = m_lut[index];

	

	if(sig==0)
	{
		if(sig2!=0)
		{
			sig = sig2;
			endcol = startcol = x;
		}
		else
		{
			goto add_x;
		}
	}
	else
	{
		if(sig == sig2)
		{
			endcol = x;
		}
		else
		{
			goto save;
		}
		
	}
	
	


add_x:
	x+=3;
	if(x>=g_rawFrame.m_width*3)
		return;

	goto next;

	
save:
	endcol = endcol/3;
	startcol = startcol/3;
	Qval qval = (endcol<<14) | (startcol<<3) |sig;
	//printf("endcol=%d,startcol=%d,sig=%d,qval=%x\r\n",
		//		endcol,startcol,sig,qval);
	g_qqueue->enqueue(&qval);

	if(sig2 != 0)
	{
		sig = sig2;
		endcol = startcol = x;
	}
	else
	{
		sig = 0;
		endcol = startcol = 0;
	}

	goto add_x;
	
}


void rls(const Frame8 *frame)
{
	uint32_t y;
	Qval eof=0xffffffff;
	for(y=0;y<(uint32_t)frame->m_height;y++)
	{
		//printf("y=%d\r\n",y);
		handleLine(frame->m_pixels+y*frame->m_width*3,frame->m_width*3);
	}

	// indicate end of frame
    g_qqueue->enqueue(&eof);
		
	
}

void unpack()
{
	SSegment s;
    int32_t row;
    bool memfull;
    uint32_t i;
    Qval qval;

	
	//qval
	//	11 bits  | 11 bits		| 3 bits |
	//	  endCol |	startCol	| model |
	row = -1;
	memfull = false;
	i = 0;

	while(1)
	{
		while(g_qqueue->dequeue(&qval)==0);
		if (qval==0xffffffff)
            break;
		i++;
		if (qval==0)
        {
            row++;
            continue;
        }
		s.model = qval&0x7;
		if(s.model>0 && !memfull)
		{
			s.row = row;
			qval >>= 3;
			s.startCol = qval&0x7ff;  
			qval >>=11;
			s.endCol = qval&0x7ff;
			if(m_assembler[s.model-1].Add(s)<0)
			{
				memfull = true;
				printf("heap full %d\r\n",i);
			}
		}
	}
	
	for (i=0; i<NUM_MODELS; i++)
    {
        m_assembler[i].EndFrame();
        m_assembler[i].SortFinished();
    }
	
}






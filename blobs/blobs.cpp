//jwy create 2016.4.6  blobs class

#include <stdio.h>
#include "colorlut.h"
#include "pixytypes.h"
#include "qqueue.h"
#include "blob.h"
#include "blobs.h"


Blobs::Blobs(Qqueue *qq)
{
	int i;
	
	m_mutex = false;
	m_qq = qq;

	m_lut = new uint8_t[CL_LUT_SIZE];
    m_clut = new ColorLUT(m_lut);

	m_blobs = new uint16_t[MAX_BLOBS*5];
    m_numBlobs = 0;

	// reset blob assemblers
    for (i=0; i<CL_NUM_SIGNATURES; i++)
        m_assembler[i].Reset();
	
}


Blobs::~Blobs()
{
	delete [] m_blobs;
	delete [] m_lut;
	delete m_clut;
}

void Blobs::rls(const Frame8 *frame)
{
	uint32_t y;
	Qval eof=0xffffffff;
	for(y=0;y<(uint32_t)frame->m_height;y++)
	{
		//printf("y=%d\r\n",y);
		handleLine(frame->m_pixels+y*frame->m_width*3,frame->m_width*3);
	}

	// indicate end of frame
    m_qq->enqueue(&eof);
		
}


//qval
//	11 bits  | 11 bits		| 3 bits |
//    endCol |  startCol    | model |

void Blobs::handleLine(uint8_t * line,uint16_t width)
{
	uint32_t index,sig,sig2,u, v,startcol,endcol;
	int32_t x,r,g,b;
	Qval newline=0;
	// new line
	m_qq->enqueue(&newline);
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
	if(x>=width)
		return;
	
	goto next;
	
		
save:
	endcol = endcol/3;
	startcol = startcol/3;
	Qval qval = (endcol<<14) | (startcol<<3) |sig;
	//printf("endcol=%d,startcol=%d,sig=%d,qval=%x\r\n",
		//		endcol,startcol,sig,qval);
	m_qq->enqueue(&qval);
	
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



void Blobs::unpack()
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
		while(m_qq->dequeue(&qval)==0);
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
		
	for (i=0; i<CL_NUM_SIGNATURES; i++)
	{
		m_assembler[i].EndFrame();
		m_assembler[i].SortFinished();
	}
		
}


void Blobs::print_blob()
{
	int i;
	CBlob *blob;
	uint16_t left, top, right, bottom;
	
	for(i=0; i<CL_NUM_SIGNATURES; i++)
	{
		blob=m_assembler[i].finishedBlobs;
		while(blob)
		{
			
			blob->getBBox((short &)left, (short &)top, (short &)right, (short &)bottom);
			printf("sig=%d,left=%d,top=%d,right=%d,bottom=%d\r\n",i+1,left,top,right,bottom);
			blob=blob->next;
		}
		
	}
}

void Blobs::getBlob()
{
	int i,j;
	m_mutex = true;
	uint16_t *blobsStart;

	uint16_t left, top, right, bottom;

	CBlob *blob;

	for(i=0, m_numBlobs=0; i<CL_NUM_SIGNATURES; i++)
	{
		for(j=m_numBlobs*5, blobsStart=m_blobs+j,blob=m_assembler[i].finishedBlobs;
					blob; blob=blob->next)
		{
			blob->getBBox((short &)left, (short &)top, (short &)right, (short &)bottom);
			if (bottom-top<=1) // blobs that are 1 line tall
               continue;

			m_blobs[j + 0] = i+1;
            m_blobs[j + 1] = left;
            m_blobs[j + 2] = right;
            m_blobs[j + 3] = top;
            m_blobs[j + 4] = bottom;
            m_numBlobs++;
            j += 5;
			 
		}
	}
	m_mutex = false;
	
}


void Blobs::getBlobs(BlobA **blobs, uint32_t *len)
{
    *blobs = (BlobA *)m_blobs;
    *len = m_numBlobs;
}





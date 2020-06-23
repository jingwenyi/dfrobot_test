//jwy create 2016.4.6  blobs class

#include <stdio.h>
#include "colorlut.h"
#include "pixytypes.h"
#include "qqueue.h"
#include "blob.h"
#include "blobs.h"
//#include <QDebug>
#include <math.h>
#include <string.h>


#define TOL  400
#define CC_SIGNATURE(s) 			true
#define MAX_CODED_DIST        		8
#define G_THRESHOLD  				30
#define MIN_COLOR_CODE_AREA   		20




Blobs::Blobs(Qqueue *qq):
	m_maxNumBlobs(MAX_BLOBS),
	m_mergeDist(MAX_MERGE_DIST)
{
	int i;
	
	m_mutex = false;
	m_qq = qq;
	
	m_maxCodedDist = MAX_CODED_DIST/2;
	m_lut = new uint8_t[CL_LUT_SIZE];
    m_clut = new ColorLUT(m_lut);

	m_blobs = new uint16_t[MAX_BLOBS*5];
    m_numBlobs = 0;

	memset(m_blobs, 0, MAX_BLOBS*5);
	memset(m_max_g, 0, sizeof(m_max_g));
	memset(m_min_g, 0, sizeof(m_min_g));
	memset(m_max_b, 0, sizeof(m_max_b));
	memset(m_min_b, 0, sizeof(m_min_b));
	memset(m_max_r, 0, sizeof(m_max_r));
	memset(m_min_r, 0, sizeof(m_min_r));

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

void Blobs::updata_g()
{
	int i;

	uint8_t avg_g,avg_b,avg_r;
	memset(m_max_g, 0, sizeof(m_max_g));
    memset(m_min_g, 0, sizeof(m_min_g));
	memset(m_max_b, 0, sizeof(m_max_b));
	memset(m_min_b, 0, sizeof(m_min_b));
	memset(m_max_r, 0, sizeof(m_max_r));
	memset(m_min_r, 0, sizeof(m_min_r));

	for(i=0; i<CL_NUM_SIGNATURES; i++)
	{
		avg_g =((m_clut->m_signatures[i].m_rgb >> 8) & 0xff);
		
		if(avg_g != 0)
		{
			m_max_g[i] = ((avg_g + G_THRESHOLD) & 0xff);
           	m_min_g[i] = ((avg_g > G_THRESHOLD) ? (avg_g - G_THRESHOLD):0);

			avg_b = (m_clut->m_signatures[i].m_rgb & 0xff);
			m_max_b[i] = ((avg_b + G_THRESHOLD) & 0xff);
			m_min_b[i] = ((avg_b > G_THRESHOLD) ? (avg_b - G_THRESHOLD):0);

			avg_r =((m_clut->m_signatures[i].m_rgb >> 16) & 0xff);
			m_max_r[i] = ((avg_r + G_THRESHOLD) & 0xff);
			m_min_r[i] = ((avg_r > G_THRESHOLD) ? (avg_r - G_THRESHOLD):0);
			
            //qDebug()<<"max_g:" << m_max_g[i] << "min_g:" << m_min_g[i] << "i" << i;
		}
		
		
	}
	
}


void Blobs::handleLine_yuv(uint8_t *yData_line, uint8_t *uData_line,
								uint8_t *vData_line,uint16_t width)
{
	uint32_t index, sig, sig2, startcol, endcol;
	int32_t x, uu, vv;
    int32_t y,u, v;
	Qval newline = 0;
	m_qq->enqueue(&newline);
	x=0;
	sig=0;
	startcol=0;
	endcol=0;
	
next:
	uu = vv = 0;
	y = yData_line[x];
    u = uData_line[x/2];
    v = vData_line[x/2];
#if 0
	uu = u;
	uu >>= 9-CL_LUT_COMPONENT_SCALE;
	uu &= (1<<CL_LUT_COMPONENT_SCALE)-1;
	vv = v;
	vv >>= 9-CL_LUT_COMPONENT_SCALE;
	vv &= (1<<CL_LUT_COMPONENT_SCALE)-1;

	index = (uu<<CL_LUT_COMPONENT_SCALE)| vv;
#else
	index = (u<<8) + v;

	//printf("-------index:%d------\r\n",index);
	
#endif
	sig2 = m_lut[index];

	if(sig == 0)
	{
		if(sig2 != 0)
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
	x+=1;
	if(x >= width)
		return;
	goto next;


save:
	
	Qval qval = (endcol<<14)|(startcol<<3) | sig;
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

#if 1
void Blobs::rls_yuv(const Frame8 *frame)
{
	int32_t y, y_idx, u_idx, v_idx;
	const long len = frame->m_width*frame->m_height;
	uint8_t *yData = frame->m_pixels;
	uint8_t *uData = &yData[len];
	uint8_t *vData = &uData[len>>2];

	y_idx = u_idx = v_idx = 0;

	Qval eof=0xffffffff;

	for(y=0; y<(uint32_t)frame->m_height; y++)
	{
		y_idx = y*frame->m_width;
		if(y%2 == 0)
		{
			u_idx = (y/2)*(frame->m_width/2);
			v_idx = u_idx;
		}
		
		handleLine_yuv(yData+y_idx, uData+u_idx, vData+v_idx, frame->m_width);
		
	}

	// indicate end of frame
    m_qq->enqueue(&eof);
	
}
#endif

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
			
			if(g>m_min_g[sig2-1] && g<m_max_g[sig2-1]
				&& b>m_min_b[sig2-1] && b<m_max_b[sig2-1] 
				&& r>m_min_r[sig2-1] && r<m_max_r[sig2-1])
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
			goto add_x;
		}
	}
	else
	{
		if(sig == sig2)
		{
			
          	if(g>m_min_g[sig2-1] && g<m_max_g[sig2-1]
				&& b>m_min_b[sig2-1] && b<m_max_b[sig2-1] 
				&& r>m_min_r[sig2-1] && r<m_max_r[sig2-1])
			{
				endcol = x;
			}
			else
			{
				goto save;
			}
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
		if(g>m_min_g[sig2-1] && g<m_max_g[sig2-1]
				&& b>m_min_b[sig2-1] && b<m_max_b[sig2-1] 
				&& r>m_min_r[sig2-1] && r<m_max_r[sig2-1])
		{
			sig = sig2;
			endcol = startcol = x;
		}
		else
		{
			sig = 0;
			endcol = startcol = 0;
		}
		
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
				//qDebug() <<"heap full";
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
	
	uint16_t *blobsStart;

	uint16_t left, top, right, bottom;
	uint16_t numBlobsStart,invalid,invalid2;

	CBlob *blob;

	invalid = 0;
	//m_mutex = true;
	for(i=0, m_numBlobs=0; i<CL_NUM_SIGNATURES; i++)
	{
		for(j=m_numBlobs*5, blobsStart=m_blobs+j,numBlobsStart=m_numBlobs,blob=m_assembler[i].finishedBlobs;
					blob && (m_numBlobs < m_maxNumBlobs); blob=blob->next)
		{
            if(blob->GetArea() < MIN_COLOR_CODE_AREA)
                continue;
			//printf("area:%d\n",blob->GetArea());
            //qDebug()<< "area:"<<blob->GetArea();
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

		while(1)
		{
			invalid2 = combine2(blobsStart, m_numBlobs-numBlobsStart);
			if(invalid2 == 0)
				break;
			invalid += invalid2;
			
		}
	}

	invalid += combine(m_blobs, m_numBlobs);

	if(0)
	{
		 m_ccBlobs = (BlobB *)(m_blobs + m_numBlobs*5);
		 processCC();
	}

	if(invalid)
	{
		//qDebug()<<"invalid-----"<<invalid;
		invalid2 = compress(m_blobs, m_numBlobs);
        m_numBlobs -= invalid2;
	}

	
	//m_mutex = false;

	// free memory
    for (i=0; i<CL_NUM_SIGNATURES; i++)
        m_assembler[i].Reset();
	
}


bool Blobs::closeby(BlobA *blob0, BlobA *blob1)
{
    // check to see if blobs are invalid or equal
    if (blob0->m_model==0 || blob1->m_model==0 || blob0->m_model==blob1->m_model)
        return false;
    // check to see that the blobs are from color code models.  If they aren't both
    // color code blobs, we return false
    //if (!CC_SIGNATURE(blob0->m_model&0x07) || !CC_SIGNATURE(blob1->m_model&0x07))
     //   return false;

    return distance(blob0, blob1)<=m_maxCodedDist;
}


void Blobs::mergeClumps(uint16_t scount0, uint16_t scount1)
{
    int i;
    BlobA *blobs = (BlobA *)m_blobs;
    for (i=0; i<m_numBlobs; i++)
    {
        if ((blobs[i].m_model&~0x07)==scount1)
            blobs[i].m_model = (blobs[i].m_model&0x07) | scount0;
    }
}


// impose weak size constraint
void Blobs::cleanup(BlobA *blobs[], int16_t *numBlobs)
{
	int i, j;
	bool set;
	uint16_t maxEqual, numEqual, numNewBlobs;
	BlobA *newBlobs[MAX_COLOR_CODE_MODELS*2];
	uint32_t area0, area1, lowerArea, upperArea, maxEqualArea;
	
	for (i=0, maxEqual=0, set=false; i<*numBlobs; i++)
	{
		area0 = (blobs[i]->m_right-blobs[i]->m_left) * (blobs[i]->m_bottom-blobs[i]->m_top);
		lowerArea = (area0*100)/(100+TOL);
		upperArea = area0 + (area0*TOL)/100;
	
		for (j=0, numEqual=0; j<*numBlobs; j++)
		{
			if (i==j)
				continue;
			area1 = (blobs[j]->m_right-blobs[j]->m_left) * (blobs[j]->m_bottom-blobs[j]->m_top);
			if (lowerArea<=area1 && area1<=upperArea)
				numEqual++;
		}
		if (numEqual>maxEqual)
		{
			maxEqual = numEqual;
			maxEqualArea = area0;
			set = true;
		}
	}
	
	if (!set)
		*numBlobs = 0;
	
	for (i=0, numNewBlobs=0; i<*numBlobs && numNewBlobs<MAX_COLOR_CODE_MODELS*2; i++)
	{
		area0 = (blobs[i]->m_right-blobs[i]->m_left) * (blobs[i]->m_bottom-blobs[i]->m_top);
		lowerArea = (area0*100)/(100+TOL);
		upperArea = area0 + (area0*TOL)/100;
		if (lowerArea<=maxEqualArea && maxEqualArea<=upperArea)
			newBlobs[numNewBlobs++] = blobs[i];
#ifndef PIXY
		else if (*numBlobs>=5 && (blobs[i]->m_model&0x07)==2)
			printf("eliminated!");
#endif
	}
	
	// copy new blobs over
	for (i=0; i<numNewBlobs; i++)
		blobs[i] = newBlobs[i];
	*numBlobs = numNewBlobs;
}


// eliminate duplicate and adjacent signatures
void Blobs::cleanup2(BlobA *blobs[], int16_t *numBlobs)
{
    BlobA *newBlobs[MAX_COLOR_CODE_MODELS*2];
    int i, j;
    uint16_t numNewBlobs;
    bool set;

    for (i=0, numNewBlobs=0, set=false; i<*numBlobs && numNewBlobs<MAX_COLOR_CODE_MODELS*2; i=j)
    {
        newBlobs[numNewBlobs++] = blobs[i];
        for (j=i+1; j<*numBlobs; j++)
        {
            if ((blobs[j]->m_model&0x07)==(blobs[i]->m_model&0x07))
                set = true;
            else
                break;
        }
    }
    if (set)
    {
        // copy new blobs over
        for (i=0; i<numNewBlobs; i++)
            blobs[i] = newBlobs[i];
        *numBlobs = numNewBlobs;
    }
}


int16_t Blobs::distance(BlobA *blob0, BlobA *blob1)
{
    int16_t left0, right0, top0, bottom0;
    int16_t left1, right1, top1, bottom1;

    left0 = blob0->m_left;
    right0 = blob0->m_right;
    top0 = blob0->m_top;
    bottom0 = blob0->m_bottom;
    left1 = blob1->m_left;
    right1 = blob1->m_right;
    top1 = blob1->m_top;
    bottom1 = blob1->m_bottom;

    if (left0>=left1 && ((top0<=top1 && top1<=bottom0) || (top0<=bottom1 && (bottom1<=bottom0 || top1<=top0))))
        return left0-right1;

    if (left1>=left0 && ((top0<=top1 && top1<=bottom0) || (top0<=bottom1 && (bottom1<=bottom0 || top1<=top0))))
        return left1-right0;

    if (top0>=top1 && ((left0<=left1 && left1<=right0) || (left0<=right1 && (right1<=right0 || left1<=left0))))
        return top0-bottom1;

    if (top1>=top0 && ((left0<=left1 && left1<=right0) || (left0<=right1 && (right1<=right0 || left1<=left0))))
        return top1-bottom0;

    return 0x7fff; // return a large number
}


int16_t Blobs::distance(BlobA *blob0, BlobA *blob1, bool horiz)
{
    int16_t dist;

    if (horiz)
        dist = (blob0->m_right+blob0->m_left)/2 - (blob1->m_right+blob1->m_left)/2;
    else
        dist = (blob0->m_bottom+blob0->m_top)/2 - (blob1->m_bottom+blob1->m_top)/2;

    if (dist<0)
        return -dist;
    else
        return dist;
}


void Blobs::sort(BlobA *blobs[], uint16_t len, BlobA *firstBlob, bool horiz)
{
    uint16_t i, td, distances[MAX_COLOR_CODE_MODELS*2];
    bool done;
    BlobA *tb;

    // create list of distances
    for (i=0; i<len && i<MAX_COLOR_CODE_MODELS*2; i++)
        distances[i] = distance(firstBlob, blobs[i], horiz);

    // sort -- note, we only have 5 maximum to sort, so no worries about efficiency
    while(1)
    {
        for (i=1, done=true; i<len && i<MAX_COLOR_CODE_MODELS*2; i++)
        {
            if (distances[i-1]>distances[i])
            {
                // swap distances
                td = distances[i];
                distances[i] = distances[i-1];
                distances[i-1] = td;
                // swap blobs
                tb = blobs[i];
                blobs[i] = blobs[i-1];
                blobs[i-1] = tb;

                done = false;
            }
        }
        if (done)
            break;
    }
}


int16_t Blobs::angle(BlobA *blob0, BlobA *blob1)
{
    int acx, acy, bcx, bcy;
    float res;

    acx = (blob0->m_right + blob0->m_left)/2;
    acy = (blob0->m_bottom + blob0->m_top)/2;
    bcx = (blob1->m_right + blob1->m_left)/2;
    bcy = (blob1->m_bottom + blob1->m_top)/2;

    res = atan2((float)(acy-bcy), (float)(bcx-acx))*180/3.1415f;

    return (int16_t)res;
}



void Blobs::processCC()
{
	int16_t i, j, k;
	uint16_t scount, scount1, count = 0;
	int16_t left, right, top, bottom;
	uint16_t codedModel0, codedModel;
	int32_t width, height, avgWidth, avgHeight;
	BlobB *codedBlob, *endBlobB;
	BlobA *blob0, *blob1, *endBlob;
	BlobA *blobs[MAX_COLOR_CODE_MODELS*2];
	
#if 0
	BlobA b0(1, 1, 20, 40, 50);
	BlobA b1(1, 1, 20, 52, 60);
	BlobA b2(1, 1, 20, 62, 70);
	BlobA b3(2, 22, 30, 40, 50);
	BlobA b4(2, 22, 30, 52, 60);
	BlobA b5(3, 32, 40, 40, 50);
	BlobA b6(4, 42, 50, 40, 50);
	BlobA b7(4, 42, 50, 52, 60);
	BlobA b8(6, 22, 30, 52, 60);
	BlobA b9(6, 22, 30, 52, 60);
	BlobA b10(7, 22, 30, 52, 60);
	
	BlobA *testBlobs[] =
	{
		&b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7 //, &b8, &b9, &b10
	};
	int16_t ntb = 8;
	cleanup(testBlobs, &ntb);
#endif
	
	endBlob = (BlobA *)m_blobs + m_numBlobs;
	
	// 1st pass: mark all closeby blobs
	for (blob0=(BlobA *)m_blobs; blob0<endBlob; blob0++)
	{
		for (blob1=(BlobA *)blob0+1; blob1<endBlob; blob1++)
		{
			if (closeby(blob0, blob1))
			{
				if (blob0->m_model<=CL_NUM_SIGNATURES && blob1->m_model<=CL_NUM_SIGNATURES)
				{
					count++;
					scount = count<<3;
					blob0->m_model |= scount;
					blob1->m_model |= scount;
				}
				else if (blob0->m_model>CL_NUM_SIGNATURES && blob1->m_model<=CL_NUM_SIGNATURES)
				{
					scount = blob0->m_model & ~0x07;
					blob1->m_model |= scount;
				}
				else if (blob1->m_model>CL_NUM_SIGNATURES && blob0->m_model<=CL_NUM_SIGNATURES)
				{
					scount = blob1->m_model & ~0x07;
					blob0->m_model |= scount;
				}
			}
		}
	}
	
#if 1
		// 2nd pass: merge blob clumps
	for (blob0=(BlobA *)m_blobs; blob0<endBlob; blob0++)
	{
		if (blob0->m_model<=CL_NUM_SIGNATURES) // skip normal blobs
			continue;
		scount = blob0->m_model&~0x07;
		for (blob1=(BlobA *)blob0+1; blob1<endBlob; blob1++)
		{
			if (blob1->m_model<=CL_NUM_SIGNATURES)
				continue;
	
			scount1 = blob1->m_model&~0x07;
			if (scount!=scount1 && closeby(blob0, blob1))
				mergeClumps(scount, scount1);
		}
	}
#endif
	
	// 3rd and final pass, find each blob clean it up and add it to the table
	endBlobB = (BlobB *)((BlobA *)m_blobs + MAX_BLOBS)-1;
	for (i=1, codedBlob = m_ccBlobs, m_numCCBlobs=0; i<=count && codedBlob<endBlobB; i++)
	{
		scount = i<<3;
		// find all blobs with index i
		for (j=0, blob0=(BlobA *)m_blobs; blob0<endBlob && j<MAX_COLOR_CODE_MODELS*2; blob0++)
		{
			if ((blob0->m_model&~0x07)==scount)
				blobs[j++] = blob0;
		}
	
#if 1
		// cleanup blobs, deal with cases where there are more blobs than models
		cleanup(blobs, &j);
#endif
	
		if (j<2)
			continue;
	
		// find left, right, top, bottom of color coded block
		for (k=0, left=right=top=bottom=avgWidth=avgHeight=0; k<j; k++)
		{
			//DBG("* cc %x %d i %d: %d %d %d %d %d", blobs[k], m_numCCBlobs, k, blobs[k]->m_model, blobs[k]->m_left, blobs[k]->m_right, blobs[k]->m_top, blobs[k]->m_bottom);
			if (blobs[left]->m_left > blobs[k]->m_left)
				left = k;
			if (blobs[top]->m_top > blobs[k]->m_top)
				top = k;
			if (blobs[right]->m_right < blobs[k]->m_right)
				right = k;
			if (blobs[bottom]->m_bottom < blobs[k]->m_bottom)
				bottom = k;
			avgWidth += blobs[k]->m_right - blobs[k]->m_left;
			avgHeight += blobs[k]->m_bottom - blobs[k]->m_top;
		}
		avgWidth /= j;
		avgHeight /= j;
		codedBlob->m_left = blobs[left]->m_left;
		codedBlob->m_right = blobs[right]->m_right;
		codedBlob->m_top = blobs[top]->m_top;
		codedBlob->m_bottom = blobs[bottom]->m_bottom;
	
#if 1
			// is it more horizontal than vertical?
		width = (blobs[right]->m_right - blobs[left]->m_left)*100;
		width /= avgWidth; // scale by average width because our swatches might not be square
		height = (blobs[bottom]->m_bottom - blobs[top]->m_top)*100;
		height /= avgHeight; // scale by average height because our swatches might not be square
	
		if (width > height)
			sort(blobs, j, blobs[left], true);
		else
			sort(blobs, j, blobs[top], false);
	
#if 1
		cleanup2(blobs, &j);
		if (j<2)
			continue;
		else if (j>5)
			j = 5;
#endif
		// create new blob, compare the coded models, pick the smaller one
		for (k=0, codedModel0=0; k<j; k++)
		{
			codedModel0 <<= 3;
			codedModel0 |= blobs[k]->m_model&0x07;
		}
		for (k=j-1, codedModel=0; k>=0; k--)
		{
			codedModel <<= 3;
			codedModel |= blobs[k]->m_model&0x07;
			blobs[k]->m_model = 0; // invalidate
		}
	
		if (codedModel0<codedModel)
		{
			codedBlob->m_model = codedModel0;
			codedBlob->m_angle = angle(blobs[0], blobs[j-1]);
		}
		else
		{
			codedBlob->m_model = codedModel;
			codedBlob->m_angle = angle(blobs[j-1], blobs[0]);
		}
#endif
		//DBG("cc %d %d %d %d %d", m_numCCBlobs, codedBlob->m_left, codedBlob->m_right, codedBlob->m_top, codedBlob->m_bottom);
		codedBlob++;
		m_numCCBlobs++;
	}
	
	// 3rd pass, invalidate blobs
	for (blob0=(BlobA *)m_blobs; blob0<endBlob; blob0++)
	{
		if (0)
		{
			if (blob0->m_model>CL_NUM_SIGNATURES)
				blob0->m_model = 0;
		}
		else if (blob0->m_model>CL_NUM_SIGNATURES)
			blob0->m_model = 0; // invalidate-- not part of a color code
	}
}



uint16_t Blobs::compress(uint16_t *blobs, uint16_t numBlobs)
{
    uint16_t i, ii;
    uint16_t *destination, invalid;

    // compress list
    for (i=0, ii=0, destination=NULL, invalid=0; i<numBlobs; i++, ii+=5)
    {
        if (blobs[ii+0]==0)
        {
            if (destination==NULL)
                destination = blobs+ii;
            invalid++;
            continue;
        }
        if (destination)
        {
            destination[0] = blobs[ii+0];
            destination[1] = blobs[ii+1];
            destination[2] = blobs[ii+2];
            destination[3] = blobs[ii+3];
            destination[4] = blobs[ii+4];
            destination += 5;
        }
    }
    return invalid;
}


uint16_t Blobs::combine(uint16_t *blobs, uint16_t numBlobs)
{
	uint16_t i, j, ii, jj, left0, right0, top0, bottom0;
	uint16_t left, right, top, bottom;
	uint16_t invalid;
	
	// delete blobs that are fully enclosed by larger blobs
	for (i=0, ii=0, invalid=0; i<numBlobs; i++, ii+=5)
	{
		if (blobs[ii+0]==0)
			continue;
		left0 = blobs[ii+1];
		right0 = blobs[ii+2];
		top0 = blobs[ii+3];
		bottom0 = blobs[ii+4];
	
		for (j=i+1, jj=ii+5; j<numBlobs; j++, jj+=5)
		{
			if (blobs[jj+0]==0)
				continue;
			left = blobs[jj+1];
			right = blobs[jj+2];
			top = blobs[jj+3];
			bottom = blobs[jj+4];
	
			if (left0<=left && right0>=right && top0<=top && bottom0>=bottom)
			{
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (left<=left0 && right>=right0 && top<=top0 && bottom>=bottom0)
			{
				blobs[ii+0] = 0; // invalidate
				invalid++;
			}
		}
	}
	
	return invalid;
}



uint16_t Blobs::combine2(uint16_t *blobs, uint16_t numBlobs)
{
	uint16_t i, j, ii, jj, left0, right0, top0, bottom0;
	uint16_t left, right, top, bottom;
	uint16_t invalid;
	
	for (i=0, ii=0, invalid=0; i<numBlobs; i++, ii+=5)
	{
		if (blobs[ii+0]==0)
			continue;
		left0 = blobs[ii+1];
		right0 = blobs[ii+2];
		top0 = blobs[ii+3];
		bottom0 = blobs[ii+4];
	
		for (j=i+1, jj=ii+5; j<numBlobs; j++, jj+=5)
		{
			if (blobs[jj+0]==0)
				continue;
			left = blobs[jj+1];
			right = blobs[jj+2];
			top = blobs[jj+3];
			bottom = blobs[jj+4];
	
#if 1 // if corners touch....
			if (left<=left0 && left0-right<=m_mergeDist &&
					((top0<=top && top<=bottom0) || (top0<=bottom && bottom<=bottom0)))
			{
				blobs[ii+1] = left;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (right>=right0 && left-right0<=m_mergeDist &&
						((top0<=top && top<=bottom0) || (top0<=bottom && bottom<=bottom0)))
			{
				blobs[ii+2] = right;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (top<=top0 && top0-bottom<=m_mergeDist &&
						((left0<=left && left<=right0) || (left0<=right && right<=right0)))
			{
				blobs[ii+3] = top;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (bottom>=bottom0 && top-bottom0<=m_mergeDist &&
						((left0<=left && left<=right0) || (left0<=right && right<=right0)))
			{
				blobs[ii+4] = bottom;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
#else // at least half of a side (the smaller adjacent side) has to overlap
			if (left<=left0 && left0-right<=m_mergeDist &&
					((top<=top0 && top0<=top+height) || (top+height<=bottom0 && bottom0<=bottom)))
			{
				blobs[ii+1] = left;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (right>=right0 && left-right0<=m_mergeDist &&
						((top<=top0 && top0<=top+height) || (top+height<=bottom0 && bottom0<=bottom)))
			{
				blobs[ii+2] = right;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (top<=top0 && top0-bottom<=m_mergeDist &&
						((left<=left0 && left0<=left+width) || (left+width<=right0 && right0<=right)))
			{
				blobs[ii+3] = top;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
			else if (bottom>=bottom0 && top-bottom0<=m_mergeDist &&
						((left<=left0 && left0<=left+width) || (left+width<=right0 && right0<=right)))
			{
				blobs[ii+4] = bottom;
				blobs[jj+0] = 0; // invalidate
				invalid++;
			}
#endif
		}
	}
	
	return invalid;
}



void Blobs::getBlobs(BlobA **blobs, uint32_t *len)
{
    *blobs = (BlobA *)m_blobs;
    *len = m_numBlobs;
	//qDebug()<<"m_numBlobs++++++++++"<<m_numBlobs;
}





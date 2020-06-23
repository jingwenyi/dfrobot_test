//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//

#include <new>
#if 0 //jwy add 
#ifdef PIXY
#include "pixy_init.h"
#else
#include "pixymon.h"
#endif
#include "debug.h"
#endif  //add end
#include "blob.h"

#ifdef DEBUG
#ifndef HOST
#include <textdisp.h>
#else 
#include <stdio.h>
#endif

#define DBG_BLOB(x) x
#else
#define DBG_BLOB(x) 
#endif

bool CBlob::recordSegments= false;
// Set to true for testing code only.  Very slow!
bool CBlob::testMoments= false;
// Skip major/minor axis computation when this is false
bool SMoments::computeAxes= false;
int CBlob::leakcheck=0;

#ifdef INCLUDE_STATS
void SMoments::GetStats(SMomentStats &stats) const {
    stats.area= area;
    stats.centroidX = (float)sumX / (float)area;
    stats.centroidY = (float)sumY / (float)area;

    if (computeAxes) {
        // Find the eigenvalues and eigenvectors for the 2x2 covariance matrix:
        //
        // | sum((x-|x|)^2)        sum((x-|x|)*(y-|y|)) |
        // | sum((x-|x|)*(y-|y|))  sum((y-|y|)^2)       |

        // Values= 0.5 * ((sumXX+sumYY) +- sqrt((sumXX+sumYY)^2-4(sumXXsumYY-sumXY^2)))
        // .5 * (xx+yy) +- sqrt(xx^2+2xxyy+yy^2-4xxyy+4xy^2)
        // .5 * (xx+yy) +- sqrt(xx^2-2xxyy+yy^2 + 4xy^2)

        // sum((x-|x|)^2) =
        // sum(x^2) - 2sum(x|x|) + sum(|x|^2) =
        // sum(x^2) - 2|x|sum(x) + n|x|^2 =
        // sumXX - 2*centroidX*sumX + centroidX*sumX =
        // sumXX - centroidX*sumX

        // sum((x-|x|)*(y-|y|))=
        // sum(xy) - sum(x|y|) - sum(y|x|) + sum(|x||y|) =
        // sum(xy) - |y|sum(x) - |x|sum(y) + n|x||y| =
        // sumXY - centroidY*sumX - centroidX*sumY + sumX * centroidY =
        // sumXY - centroidX*sumY

        float xx= sumXX - stats.centroidX*sumX;
        float xyTimes2= 2*(sumXY - stats.centroidX*sumY);
        float yy= sumYY - stats.centroidY*sumY;
        float xxMinusyy = xx-yy;
        float xxPlusyy = xx+yy;
        float sq = sqrt(xxMinusyy * xxMinusyy + xyTimes2*xyTimes2);
        float eigMaxTimes2= xxPlusyy+sq;
        float eigMinTimes2= xxPlusyy-sq;
        stats.angle= 0.5*atan2(xyTimes2, xxMinusyy);
        //float aspect= sqrt(eigMin/eigMax);
        //stats.majorDiameter= sqrt(area/aspect);
        //stats.minorDiameter= sqrt(area*aspect);
        //
        // sqrt(eigenvalue/area) is the standard deviation
        // Draw the ellipse with radius of twice the standard deviation,
        // which is a diameter of 4 times, which is 16x inside the sqrt

        stats.majorDiameter= sqrt(8.0*eigMaxTimes2/area);
        stats.minorDiameter= sqrt(8.0*eigMinTimes2/area);
    }
}

void SSegment::GetMomentsTest(SMoments &moments) const {
    moments.Reset();
    int y= row;
    for (int x= startCol; x <= endCol; x++) {
        moments.area++;
        moments.sumX += x;
        moments.sumY += y;
        if (SMoments::computeAxes) {
            moments.sumXY += x*y;
            moments.sumXX += x*x;
            moments.sumYY += y*y;
        }
    }
}
#endif

///////////////////////////////////////////////////////////////////////////
// CBlob
CBlob::CBlob() 
{
    DBG_BLOB(leakcheck++);
    // Setup pointers
    firstSegment= NULL;
    lastSegmentPtr= &firstSegment;

    // Reset blob data
    Reset();
}

CBlob::~CBlob() 
{
    DBG_BLOB(leakcheck--);
    // Free segments, if any
    Reset();
}

void 
CBlob::Reset() //初始化blob
{
    // Clear blob data
    moments.Reset();

    // Empty bounds
    right = -1;
    left = top = 0x7fff;
    lastBottom.row = lastBottom.invalid_row;
    nextBottom.row = nextBottom.invalid_row;

    // Delete segments if any
    SLinkedSegment *tmp;
    while(firstSegment!=NULL) {
        tmp = firstSegment;
        firstSegment = tmp->next;
        delete tmp;
    }
    lastSegmentPtr= &firstSegment;
}

void 
CBlob::NewRow() //新的一行
{
    if (nextBottom.row != nextBottom.invalid_row) {
        lastBottom= nextBottom;
        nextBottom.row= nextBottom.invalid_row;//开启新的一行
    }
}


//将一个segment 加入到一个blob中
void 
CBlob::Add(const SSegment &segment) 
{
    // Enlarge bounding box if necessary
    //有必要扩大box的边界
    UpdateBoundingBox(segment.startCol, segment.row, segment.endCol);

    // Update next attachment "surface" at bottom of blob
    if (nextBottom.row == nextBottom.invalid_row) {
        // New row.
        nextBottom= segment;
    } else {
        // Same row.  Add to right side of nextBottom.
        //如果是同一行，增加right 的值就行了
        nextBottom.endCol= segment.endCol;
    }

	//求出该segment 像素点的个数
    SMoments segmentMoments;
    segment.GetMoments(segmentMoments);
	//增加整个blob的总大小
    moments.Add(segmentMoments);

    if (testMoments) {
#ifdef INCLUDE_STATS
        SMoments test;
        segment.GetMomentsTest(test);
        assert(test == segmentMoments);
#endif
    }
	//recordSegments=false
    if (recordSegments) {
        // Add segment to the _end_ of the linked list
        //把每个segment 添加到list 后面，估计由于这样做费时费内存，所以没有这样做
        *lastSegmentPtr= new (std::nothrow) SLinkedSegment(segment);
        if (*lastSegmentPtr==NULL)
            return;
        lastSegmentPtr= &((*lastSegmentPtr)->next);
    }
}

// This takes futileResister and assimilates it into this blob
//
// Takes advantage of the fact that we are always assembling top to
// bottom, left to right.
//
// Be sure to call like so:
// leftblob.Assimilate(rightblob);
//
// This lets us assume two things:
//两个假设
// 1) The assimilated blob contains no segments on the current row
//在当前行上，被吸收的blob没有segments
// 2) The assimilated blob lastBottom surface is to the right
//被吸收的blob lastBottom 的接口在吸收blob lastBottom 接口的右边
//    of this blob's lastBottom surface
void 
CBlob::Assimilate(CBlob &futileResister) 
{
	//增加面积
    moments.Add(futileResister.moments);
	//扩大边界
    UpdateBoundingBox(futileResister.left,
                      futileResister.top,
                      futileResister.right);
    // Update lastBottom
    if (futileResister.lastBottom.endCol > lastBottom.endCol) {
        lastBottom.endCol= futileResister.lastBottom.endCol;
    }
    
    if (recordSegments) {
        // Take segments from futileResister, append on end
        *lastSegmentPtr= futileResister.firstSegment;
        lastSegmentPtr= futileResister.lastSegmentPtr;
        futileResister.firstSegment= NULL;
        futileResister.lastSegmentPtr= &futileResister.firstSegment;
        // Futile resister is left with no segments
    }
}

// Only updates left, top, and right.  bottom is updated 
// by UpdateAttachmentSurface below
void 
CBlob::UpdateBoundingBox(int newLeft, int newTop, int newRight) 
{
    if (newLeft  < left ) left = newLeft;
    if (newTop   < top  ) top  = newTop;
    if (newRight > right) right= newRight;
}

///////////////////////////////////////////////////////////////////////////
// CBlobAssembler

CBlobAssembler::CBlobAssembler() 
{
    activeBlobs= currentBlob= finishedBlobs= NULL;
    previousBlobPtr= &activeBlobs;
    currentRow=-1;
    maxRowDelta=1;
    m_blobCount=0;
}

CBlobAssembler::~CBlobAssembler() 
{
    // Flush any active blobs into finished blobs
    EndFrame();
    // Free any finished blobs
    Reset();
}

// Call once for each segment in the color channel
int CBlobAssembler::Add(const SSegment &segment) {
    if (segment.row != currentRow) {
		//如果是新的一行数据，就把activeblobs 中的数据放入到finshblobs中
        // Start new row
        currentRow= segment.row;//记录新的行号
        RewindCurrent();
    }

	//处理同一行数据
    
    // Try to link this to a previous blob
    //试图加入到之前的blob 中
    while (currentBlob) {
        if (segment.startCol > currentBlob->lastBottom.endCol) {
            // Doesn't connect.  Keep searching more blobs to the right.
            //加入的块比之前的位置比之前的大，需要把当前的blob 放入到finsh中，
            //往后面一个一个的找，看是否有合适blob
            AdvanceCurrent();
        } else {
            if (segment.endCol < currentBlob->lastBottom.startCol) {
                // Doesn't connect to any blob.  Stop searching.
                break;
            } else {
                // Found a blob to connect to
                //连接到当前blob
                currentBlob->Add(segment);
                // Check to see if we attach to multiple blobs
                //检查我们是否连接了很多blobs
                while(currentBlob->next &&
                      segment.endCol >= currentBlob->next->lastBottom.startCol) {
                    // Can merge the current blob with the next one,
                    // assimilate the next one and delete it.
                    //是否能把当前的blob 融入到下一个blob中，
                    //吸收下一个blob，并删掉它

                    // Uncomment this for verbose output for testing
                    // cout << "Merging blobs:" << endl
                    //     << " curr: bottom=" << currentBlob->bottom
                    //     << ", " << currentBlob->lastBottom.startCol
                    //     << " to " << currentBlob->lastBottom.endCol
                    //     << ", area " << currentBlob->moments.area << endl
                    //     << " next: bottom=" << currentBlob->next->bottom
                    //     << ", " << currentBlob->next->lastBottom.startCol
                    //     << " to " << currentBlob->next->lastBottom.endCol
                    //     << ", area " << currentBlob->next->moments.area << endl;

                    CBlob *futileResister = currentBlob->next;
                    // Cut it out of the list
                    currentBlob->next = futileResister->next;
                    // Assimilate it's segments and moments
                    currentBlob->Assimilate(*(futileResister));

                    // Uncomment this for verbose output for testing
                    // cout << " NEW curr: bottom=" << currentBlob->bottom
                    //     << ", " << currentBlob->lastBottom.startCol
                    //     << " to " << currentBlob->lastBottom.endCol
                    //     << ", area " << currentBlob->moments.area << endl;

                    // Delete it
                    delete futileResister;

                    BlobNewRow(&currentBlob->next);
                }
                return 0;
            }
        }
    }
    
    // Could not attach to previous blob, insert new one before currentBlob
    //不能连接到之前的blob上，在currentBlob 前插入一个新的
    CBlob *newBlob= new (std::nothrow) CBlob();
    if (newBlob==NULL)
    {
        //DBG("blobs %d\nheap full", m_blobCount);
        return -1;
    }
    m_blobCount++;//记录blob 的数量
    newBlob->next= currentBlob;//插入到currentBlob 之前
    *previousBlobPtr= newBlob; //当前previousBlobPtr 指针指向新的blob
    previousBlobPtr= &newBlob->next;
    newBlob->Add(segment);//将segment 加入newBlob 中
    return 0;
}

// Call at end of frame
// Moves all active blobs to finished list
//把所有active blobs 放到finished list 中
void CBlobAssembler::EndFrame() {
    while (activeBlobs) {
        activeBlobs->NewRow();
        CBlob *tmp= activeBlobs->next;
        activeBlobs->next= finishedBlobs;
        finishedBlobs= activeBlobs;
        activeBlobs= tmp;
    }
}

//获取blob的长度
int CBlobAssembler::ListLength(const CBlob *b) {
    int len= 0;
    while (b) {
        len++;
        b=b->next;
    }
    return len;
}


// Split a list of blobs into two halves
//将一个blobs 链表分成两半
void CBlobAssembler::SplitList(CBlob *all,
                               CBlob *&firstHalf, CBlob *&secondHalf) {
    firstHalf= secondHalf= all;
    CBlob *ptr= all, **nextptr= &secondHalf;
    while (1) {
        if (!ptr->next) break;
        ptr= ptr->next;
        nextptr= &(*nextptr)->next;
        if (!ptr->next) break;
        ptr= ptr->next;
    }
    secondHalf= *nextptr;
    *nextptr= NULL;
}

// Merge maxelts elements from old1 and old2 into newptr
void CBlobAssembler::MergeLists(CBlob *&old1, CBlob *&old2,
                                CBlob **&newptr, int maxelts) {
    int n1= maxelts, n2= maxelts;
    while (1) {
        if (n1 && old1) {
            if (n2 && old2 && old2->moments.area > old1->moments.area) {
                // Choose old2
                *newptr= old2;
                newptr= &(*newptr)->next;
                old2= *newptr;
                --n2;
            } else {
                // Choose old1
                *newptr= old1;
                newptr= &(*newptr)->next;
                old1= *newptr;
                --n1;
            }
        }
        else if (n2 && old2) {
            // Choose old2
            *newptr= old2;
            newptr= &(*newptr)->next;
            old2= *newptr;
            --n2;
        } else {
            // Done
            return;
        }
    }
}

#ifdef DEBUG
void len_error() {
    printf("len error, wedging!\n");
    while(1);
}
#endif

// Sorts finishedBlobs in order of descending area using an in-place
//按照递减的方式对finishedBlobs进行排序
// merge sort (time n log n)
void CBlobAssembler::SortFinished() {
    // Divide finishedBlobs into two lists
    CBlob *old1, *old2;

    if(finishedBlobs == NULL) {
        return;
    }

    DBG_BLOB(int initial_len= ListLength(finishedBlobs));
    DBG_BLOB(printf("BSort: Start 0x%x, len=%d\n", finishedBlobs,
               initial_len));
    SplitList(finishedBlobs, old1, old2);

    // First merge lists of length 1 into sorted lists of length 2
    //首先把list 1混合到list 2中
    // Next, merge sorted lists of length 2 into sorted lists of length 4
    //然后，使list 2 合入list 4
    // And so on.  Terminate when only one merge is performed, which
    // means we're completely sorted.
    //这意味着我们排序完成
    
    for (int blocksize= 1; old2; blocksize <<= 1) {
        CBlob *new1=NULL, *new2=NULL, **newptr1= &new1, **newptr2= &new2;
        while (old1 || old2) {
            DBG_BLOB(printf("BSort: o1 0x%x, o2 0x%x, bs=%d\n",
                       old1, old2, blocksize));
            DBG_BLOB(printf("       n1 0x%x, n2 0x%x\n",
                       new1, new2));
            MergeLists(old1, old2, newptr1, blocksize);
            MergeLists(old1, old2, newptr2, blocksize);
        }
        *newptr1= *newptr2= NULL; // Terminate lists
        old1= new1;
        old2= new2;
    }
    finishedBlobs= old1;
    DBG_BLOB(AssertFinishedSorted());
    DBG_BLOB(int final_len= ListLength(finishedBlobs));
    DBG_BLOB(printf("BSort: DONE  0x%x, len=%d\n", finishedBlobs,
               ListLength(finishedBlobs)));
    DBG_BLOB(if (final_len != initial_len) len_error());
}

// Assert that finishedBlobs is in fact sorted.  For testing only.
//测试finishedBlobs 已经找到像素的个数 大小进行排序了
void CBlobAssembler::AssertFinishedSorted() {
    if (!finishedBlobs) return;
    CBlob *i= finishedBlobs;
    CBlob *j= i->next;
    while (j) {
        assert(i->moments.area >= j->moments.area);
        i= j;
        j= i->next;
    }
}

void CBlobAssembler::Reset() {//初始化所有的变量，释放 finishedBlobs 指针
    assert(!activeBlobs);
    currentBlob= NULL;
    currentRow=-1;
    m_blobCount=0;
    while (finishedBlobs) {
        CBlob *tmp= finishedBlobs->next;
        delete finishedBlobs;
        finishedBlobs= tmp;
    }
    DBG_BLOB(printf("after CBlobAssember::Reset, leakcheck=%d\n", CBlob::leakcheck));
}

// Manage currentBlob
//
// We always want to guarantee that both currentBlob
// and currentBlob->next have had NewRow() called, and have
// been validated to remain on the active list.  We could just
// do this for all activeBlobs at the beginning of each row,
// but it's less work to only do it on demand as segments come in
// since it might allow us to skip blobs for a given row
// if there are no segments which might overlap.

// BlobNewRow:
//
// Tell blob there is a new row of data, and confirm that the
//告诉blob 这里有一新行的数据，确认这个
// blob should still be on the active list by seeing if too many
//blob 应当放到这个active 队列中通过查看是否有太多之前的
// rows have elapsed since the last segment was added.
//行已经加入
// If blob should no longer be on the active list, remove it and
//如果blob 已经 在active list 中，溢出它和把它放到
// place on the finished list, and skip to the next blob.
//finished 中，跳到下一个blob
// Call this either zero or one time per blob per row, never more.
//
// Pass in the pointer to the "next" field pointing to the blob, so
// we can delete the blob from the linked list if it's not valid.

void 
CBlobAssembler::BlobNewRow(CBlob **ptr) 
{
    short left, top, right, bottom;

    while (*ptr) {
        CBlob *blob= *ptr;
        blob->NewRow();
		//maxRowDelta = 1
        if (currentRow - blob->lastBottom.row > maxRowDelta) {
			//已完成很多行
            // Too many rows have elapsed.  Move it to the finished list
            *ptr= blob->next; // cut out of current list
            // check to see if it meets height and area constraints
            blob->getBBox(left, top, right, bottom);
            if (bottom-top>1) //&& blob->GetArea()>=MIN_COLOR_CODE_AREA)
            {
                // add to finished blobs
                //将blob加入到finsishedBlobs 中
                blob->next= finishedBlobs;
                finishedBlobs= blob;
            }
            else
                delete blob;
        } else {
            // Blob is valid
            return;
        }
    }
}

void 
CBlobAssembler::RewindCurrent() //加入currentBlobs
{
	//把activeBlobs 移入到finishedBlobs 中
    BlobNewRow(&activeBlobs);
	//把previousBlobPtr 指针的地址指向activeBlobs的地址
    previousBlobPtr= &activeBlobs;
    currentBlob= *previousBlobPtr;

    if (currentBlob) BlobNewRow(&currentBlob->next);
}

void 
CBlobAssembler::AdvanceCurrent()  //从currentBlob 中取出数据
{
    previousBlobPtr= &(currentBlob->next);
    currentBlob= *previousBlobPtr;
    if (currentBlob) BlobNewRow(&currentBlob->next);
}



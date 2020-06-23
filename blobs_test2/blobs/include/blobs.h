/*
**  filename: blobs.h
**  author: jingwenyi
**  date: 2016.04.05
**
*/

#ifndef BLOBS_H
#define BLOBS_H

#include <stdint.h>
#include "blob.h"
#include "pixytypes.h"
#include "colorlut.h"
#include "qqueue.h"


#define MAX_BLOBS             100
#define MIN_COLOR_CODE_AREA   200
#define MAX_MERGE_DIST        7
#define MAX_COLOR_CODE_MODELS 10






class Blobs
{
public:
	Blobs(Qqueue*qq);
	~Blobs();
	
	ColorLUT* m_clut;
	uint8_t* m_lut;
	Qqueue* m_qq; 


	void rls(const Frame8 *frame);
	void handleLine(uint8_t *line,uint16_t width);
	void unpack();
	void print_blob();
	void getBlob();
	void getBlobs(BlobA **blobs, uint32_t *len);
	void updata_g();

private:
	uint16_t combine2(uint16_t *blobs, uint16_t numBlobs);
	uint16_t combine(uint16_t *blobs, uint16_t numBlobs);
	uint16_t compress(uint16_t *blobs, uint16_t numBlobs);
	void processCC();
	bool closeby(BlobA *blob0, BlobA *blob1);
	void mergeClumps(uint16_t scount0, uint16_t scount1);
	void cleanup(BlobA *blobs[], int16_t *numBlobs);
	void cleanup2(BlobA *blobs[], int16_t *numBlobs);
	void sort(BlobA *blobs[], uint16_t len, BlobA *firstBlob, bool horiz);
	int16_t angle(BlobA *blob0, BlobA *blob1);
	int16_t distance(BlobA *blob0, BlobA *blob1);
	int16_t distance(BlobA *blob0, BlobA *blob1, bool horiz);
	

private:
	
	CBlobAssembler m_assembler[CL_NUM_SIGNATURES];
	bool m_mutex;
	uint16_t m_numBlobs;
	uint16_t m_maxNumBlobs;
	uint16_t *m_blobs;
	uint16_t m_mergeDist;
	BlobB *m_ccBlobs;
	uint16_t m_numCCBlobs;
	uint16_t m_maxCodedDist;
	unsigned char  m_max_g[7];
	unsigned char  m_min_g[7];
	unsigned char  m_max_b[7];
	unsigned char  m_min_b[7];
	unsigned char  m_max_r[7];
	unsigned char  m_min_r[7];
	
	
};



#endif //BLOBS_H






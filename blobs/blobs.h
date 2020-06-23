//jwy create 2016.4.5 

#ifndef BLOBS_H
#define BLOBS_H

#include <stdint.h>
#include "blob.h"
#include "pixytypes.h"
#include "colorlut.h"
#include "qqueue.h"


#define MAX_BLOBS             100



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
	
	

private:
	
	CBlobAssembler m_assembler[CL_NUM_SIGNATURES];
	bool m_mutex;
	uint16_t m_numBlobs;
	uint16_t *m_blobs;
	
};



#endif //BLOBS_H






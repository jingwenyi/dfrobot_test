// author jingwenyi create 2016.4.11  yuv420pTorgb24.h

#ifndef  _YUV420PTORGB24_H_
#define _YUV420PTORGB24_H_

#include <stdint.h>

int yuv420p2rgb24(uint8_t *pYUV, uint8_t *pBGR24, int width, int height);
int yuv420p2rgb24_2(uint8_t *pYUV, uint8_t *pBGR24, int width, int height);
int yuv420p2rgb24_3(uint8_t *pYUV, uint8_t *pBGR24, int width, int height);


#endif




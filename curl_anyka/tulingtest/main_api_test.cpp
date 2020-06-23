#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "json_api.h"
/*
struct ColorSignature
{
	ColorSignature()
	{
		m_uMin = m_uMax = m_uMean = m_vMin = m_vMax = m_vMean = m_type = 0;
	}	

    int32_t m_uMin;
    int32_t m_uMax;
    int32_t m_uMean;
    int32_t m_vMin;
    int32_t m_vMax;
    int32_t m_vMean;
	uint32_t m_rgb;
	uint32_t m_type;
};
*/


int main()
{
	int i, count=0;
	struct ColorSignature sig_array[7];
	memset((void *)sig_array, 0, sizeof(ColorSignature)*7);
	
	for(i=0; i<7; i++)
	{	
		count++;
		sig_array[i].m_uMin = count;
		sig_array[i].m_uMax = count;
		sig_array[i].m_uMean = count;
		sig_array[i].m_vMin = count;
		sig_array[i].m_vMax = count;
		sig_array[i].m_vMean = count;
		sig_array[i].m_rgb = (uint32_t)count;
		sig_array[i].m_type = (uint32_t)count;
	}

	
	json_write_file(sig_array,"save2.txt");

	system("mv save2.txt save1.txt");

	struct ColorSignature *sig_array1;
	sig_array1 = json_load_file("save1.txt");
	
	for(i=0; i<7; i++)
	{
		printf("signum:%d,uMin=%d,uMax=%d,uMean=%d,vMax=%d,vMin=%d,vMean=%d,rgb=%d,type=%d\r\n",
			i+1,sig_array1[i].m_uMin,sig_array1[i].m_uMax,sig_array1[i].m_uMean,sig_array1[i].m_vMax,
			sig_array1[i].m_vMin,sig_array1[i].m_vMean,sig_array1[i].m_rgb,sig_array1[i].m_type);
	}
	
	


	return 0;
}

//jingwenyi create 2016.05.26



#include <stdio.h>
#include "jpeglib.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>
#include <sys/time.h>


#define		WIDTH		320
#define		HEIGHT		240
#define		WIDTH_720P	1280
#define		HEIGHT_720P	720

#define		IMAG_SIZE_720P   	(1280*720*3 + 54)
#define		IMAG_SIZE_320X240	(320*240*3 + 54)
#define		JPEG_BUF_SIZE		(1024*1024)

#define		JPEG_FILE_NAME		"test.jpeg"


void rgb2jpeg(unsigned char ** jpeg_buf, unsigned long int *jpeg_len, unsigned char *bmp_data, int width, int height)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];

	int row_stride;

	//开始进行jpg的数据写入
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	//确定要用于输出压缩jpeg 的数据空间
	jpeg_mem_dest(&cinfo, jpeg_buf, jpeg_len);
	
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	
	jpeg_set_defaults(&cinfo);
	
	jpeg_set_quality(&cinfo, 90, TRUE);
	
	jpeg_start_compress(&cinfo, TRUE);
	
	row_stride = cinfo.image_width*3;

	while(cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &bmp_data[cinfo.next_scanline*row_stride];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

}


int main()
{
	uint8_t* bmp_data;
	uint8_t* jpeg_data;
	unsigned long int jpeg_len;

	bmp_data = new uint8_t[IMAG_SIZE_320X240];
	jpeg_data = new uint8_t[JPEG_BUF_SIZE];

	long fd = open("DC001319.bmp", O_RDONLY);
	if(fd <= 0)
	{
		printf("open file failed\r\n");
		return -1;
	}

	read(fd,bmp_data,IMAG_SIZE_320X240);
	close(fd);

	struct timeval tvs1;
	struct timeval tvs2;
	static int count = 0;

	while(1)
	{
		count++;
		rgb2jpeg(&jpeg_data, &jpeg_len, &bmp_data[53], WIDTH, HEIGHT);

		//printf("jpeg size:%ld\r\n",jpeg_len);
		gettimeofday(&tvs1, NULL);
		if(tvs1.tv_sec != tvs2.tv_sec)
		{
			tvs2.tv_sec = tvs1.tv_sec;
			printf("count=%d\r\n",count);
			count=0;
		}
	}
	
	fd = open(JPEG_FILE_NAME, O_RDWR | O_CREAT, 0777);
	if(fd <= 0)
	{
		printf("open file failed\r\n");
		return -1;
	}
	write(fd, jpeg_data, jpeg_len);
	close(fd);
	
	printf("dfrobot hello\r\n");

	delete []bmp_data;
	delete []jpeg_data;
	return 0;
}



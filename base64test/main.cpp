//jwy 2016.07.27 test base64

#include <stdio.h>
#include <stdlib.h>
#include "Base64.h"

#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#if 0

unsigned char test[10] = {'0','1','2','3','4','5','6','7','8','9'};
//len必需填,为src的长度，dest 的长度为len*4/3 + (len%3 == 0)?0:4

int main()
{
	
	int i;
	for(i=0; i<10; i++)
	{
		printf("%c\t",test[i]);
	}
	printf("\r\n");
	int len = 10*4/3 + ((10%3==0) ? 0:4);
	printf("len:%d\r\n",len);
	unsigned char res[len+1] = {0};
	unsigned char res2[20] = {0};

	Base64_Encode(test,res, 10);
	printf("%s\r\n",res);
	
	Base64_Decode(res, res2, len);
	for(i=0; i<10; i++)
	{
		printf("%c\t",res2[i]);
	}
	
	
	return 0;
}

#endif

#if 0
int main()
{
	unsigned char buf[40*1024] = {0};
	long fd = open("test.mp3.txt", O_RDONLY);
	if(fd < 0)
	{
		printf("open test.mp3 failed\r\n");
		return 0;
	}

	int size = read(fd, buf,40*1024);
	close(fd);

	printf("size=%d\r\n", size);

	int len = size*4/3 + ((size%3==0) ? 0:4);
	printf("len=%d\r\n", len);

	unsigned char res[len+1] = {0};

	//Base64_Encode(buf,res, size);
	
	//printf("len = %d,%s\r\n",strlen((char*)res), res);

	memset(buf, 0 ,sizeof(buf));

	//Base64_Decode(res, buf, len);

	long fd1 = open("decode.mp3.txt",  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);

	if(fd1 < 0)
	{
		printf("open decode.mp3 error\r\n");
		return -1;
	}

	write(fd1, buf, size);
	close(fd1);
		
	return 0;
	

}

#endif

int main()
{
	unsigned char buf[40*1024] = {0};
	long fd = open("test.mp3.txt", O_RDONLY);
	if(fd < 0)
	{
		printf("open test.mp3 failed\r\n");
		return 0;
	}

	int size = read(fd, buf,40*1024);
	close(fd);

	printf("size=%d\r\n", size);

	char* res = base64_encode(buf, size);

	printf("strlen res=%d,%s\r\n",strlen(res), res);


	int res_len = 0;

	unsigned char *res2 = base64_decode(res, strlen(res), &res_len);

	printf("res_len=%d\r\n",res_len);

	
	long fd1 = open("decode.mp3.txt",  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);

	if(fd1 < 0)
	{
		printf("open decode.mp3 error\r\n");
		return -1;
	}

	write(fd1, res2, size);
	close(fd1);


	return 0;
	
	

}






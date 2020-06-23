#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main()
{
	unsigned int a = 0x12345678;
	unsigned char b[4];
	memcpy(b, &a,4);
	//78 56 34 12
	printf("a[0]=%x,a[1]=%x,a[2]=%x,a[3]=%x\r\n",b[0],b[1],b[2],b[3]);
	return 0;
}

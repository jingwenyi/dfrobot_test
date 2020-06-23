#include <stdio.h>
#include <unistd.h>


int main()
{
#if 0
	FILE *fd = fopen("/dev/ttydfrobot0", "w");
	while(1)
	{
		fwrite("1234567890\n", 1, 11,fd);
		sleep(2);
	}

	fclose(fd);
#endif
	while(1)
	{
		printf("------jingwenyi-------test\r\n");
		sleep(3);
	}
}

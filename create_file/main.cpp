#include <stdio.h>
#include <fcntl.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>



int main()
{
	int fd = open("/mnt/DC1980011/DC023916.jpeg", O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);

	if(fd <= 0)
	{
		printf("open file error\r\n");
		return -1;
	}
	write(fd, "hello", 10);

	close(fd);
	return 0;
}



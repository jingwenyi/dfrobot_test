

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <errno.h>

long fd;


#define TTY_COMMUNICATE_ARDUINO   "/dev/ttySAK0"


pthread_t uart_write;


void *uart_write_func(void *arg)
{
	while(1)
	{
		sleep(5);
		write(fd, "I am write", 11);
	}

}


int main()
{
	
	


	
	struct termios  oldtio;
	
	int maxfd = -1;
	fd_set  allset;
	fd_set  fdset;

	printf("-------------------\r\n");

	fd = open(TTY_COMMUNICATE_ARDUINO, O_RDWR | O_NOCTTY );

	if(fd < 0 )
	{
		printf("fd  open error\r\n");
		return -1;
	}

	if(tcgetattr(fd, &oldtio) != 0)
	{
		printf("tcgettattr failed \r\n");
		return -1;
	}

	cfsetispeed(&oldtio, B115200);

	if(tcsetattr(fd, TCSANOW, &oldtio) != 0)
	{
		printf("tcsetattr failed \r\n");
		return -1;
	}

	printf("set uart ok\r\n");
	
	
	


	//FD_ZERO(&allset);
	//FD_SET(fd, &allset);
	//maxfd = fd;


	//signal(SIGTTIN, SIG_IGN);
	//signal(SIGTTOU, SIG_IGN);

	//pthread_create(&uart_write, NULL, uart_write_func, NULL);
	

	while(1)
	{
		
		FD_ZERO(&allset);
		FD_SET(fd, &allset);
		maxfd = fd;
		fdset = allset;
		if (select(maxfd+1, &fdset, NULL, NULL, NULL) > 0)
		{
			int count;
			char buf[100]={0};
			count = read(fd, buf, 100);
			printf("read:%s,count=%d\r\n", buf, count);
			
			write(fd, "i am ok",7);
		}
	}
	
	close(fd);

	return 0;
}




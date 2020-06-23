/*
**  author jingwenyi
**  date  2016.04.12
**  for test socket
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>


#define MAXBUF 1024
int main(int argc, char **argv)
{
	int sockfd,new_fd;
	socklen_t len;
	struct sockaddr_in my_addr, their_addr;
	unsigned int myport,lisnum;
	char buf[MAXBUF+1];
	fd_set rfds;

	struct timeval tv;
	int retval,maxfd = -1;

	if(argv[1])
	{
		myport = atoi(argv[1]);
	}
	else
	{
		myport = 7838;
	}

	if(argv[2])
	{
		lisnum = atoi(argv[2]);
	}
	else
	{
		lisnum = 2;
	}

	if((sockfd = socket(PF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket\r\n");
		exit(1);
	}

	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = PF_INET;
	my_addr.sin_port = htons(myport);
	if(argv[3])
		my_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1)
	{
		perror("bind\r\n");
		exit(1);
	}

	if(listen(sockfd,lisnum) == -1)
	{
		perror("listen\r\n");
		exit(1);
	}

	while(1)
	{
		printf("--waiting for new connecting --\r\n");
		len = sizeof(struct sockaddr);
		if((new_fd = accept(sockfd,(struct sockaddr *)&their_addr,&len))==-1)
		{
			perror("accept\r\n");
			exit(errno);
		}
		else
		{
			printf("server:get connection from %s, prot%d,socked%d\n",
				inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),new_fd);
		}

		//��ʼ����ÿ���������ϵ������շ�
		while(1)
		{
			//�Ѽ������
			FD_ZERO(&rfds);
			//�ѱ�׼������0 ���뵽������
			FD_SET(0,&rfds);
			maxfd=0;
			//�ѵ�ǰ���Ӿ��new_fd ���뵽������
			FD_SET(new_fd,&rfds);
			if(new_fd > maxfd)
				maxfd = new_fd;
			//�������ȴ�ʱ��
			tv.tv_sec = 1;
			tv.tv_usec = 0;

			retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
			if(retval == -1)
			{
				printf("select error%s\n",strerror(errno));
				break;
			}
			else if(retval == 0)
			{
				continue;
			}
			else
			{
				if(FD_ISSET(new_fd,&rfds))
				{
					//���ӵ�socket ������Ϣ����ղ���ʾ
					bzero(buf,MAXBUF+1);
					len = recv(new_fd,buf,MAXBUF,0);
					if(len>0)
					{
						printf("recv msg success:%s,%dbytes\n",buf,len);
					}
					

					send(new_fd, "i server\n",100 ,0);
				}//FD_ISSET = sockfd ���
				if(FD_ISSET(0,&rfds))
				{
			#if 0
					//�û������룬���ȡ�����ݲ�����
					bzero(buf,MAXBUF+1);
					fgets(buf,MAXBUF,stdin);
					if(!strncasecmp(buf,"quit",4))
					{
						printf("self request to quit the chating\n");
						break;
					}
					//����Ϣ��������
					len = send(new_fd, buf, strlen(buf)-1,0);
					if(len < 0)
					{
						printf("msg send fail,%d,%s\r\n",errno,strerror(errno));
						break;
					}
					else
					{
						printf("send success\n");
					}
			#endif
				} //FD_ISSET = 0
				
			} //select �������
			
		}
		close(new_fd);
			fflush(stdout);
			bzero(buf,MAXBUF+1);
			fgets(buf,MAXBUF,stdin);
			if(!strncasecmp(buf,"no",2))
			{
				printf("quit the chating\n");
				break;
			}
		
	}
	close(sockfd);

	return 0;
}

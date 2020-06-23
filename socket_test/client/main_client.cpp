// author jingwenyi create 2016.04.12 

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <resolv.h>

#define MAXBUF 1024


int ClientInit()
{
	int mSocketfd = 0;
	struct sockaddr_in dest;
	printf("clientinit\n");
	if(mSocketfd = socket(AF_INET,SOCK_STREAM, 0) < 0)
	{
		printf("socket error\n");
		return -1;
	}

	printf("mSocketfd = %d \r\n",mSocketfd);
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(3232);
	//printf("mip:%s\n",mIp);
	//inet_aton(mIp, (struct in_addr*)&dest.sin_addr.s_addr); //Ҳ����
	dest.sin_addr.s_addr = inet_addr("192.168.18.128");

	printf("------ %s, prot%d,socked%d\n",
				inet_ntoa(dest.sin_addr),ntohs(dest.sin_port),mSocketfd);

	if(connect(mSocketfd, (struct sockaddr*)&dest, sizeof(dest)) != 0)
	{
		printf("connect failed \r\n");
		close(mSocketfd);
		mSocketfd = -1;
		return -1;
	}
	printf("connect ok\n");
	return mSocketfd;
}

//int sockfd;


int main(int argc, char **argv)
{
	int sockfd, len;
	//int len;
	struct sockaddr_in dest;
	char buffer[MAXBUF+1];

	fd_set rfds;

	struct timeval tv;
	int retval,maxfd=-1;
#if 1

	if(argc != 3)
	{
		printf("the param style wrong\n");
		//exit(0);
	}
	//����һ��socket	����tcp ͨ��
	if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		printf("socket\r\n");
		exit(errno);
	}
	//��ʼ���������(�ط��ĵ�ַ�Ͷ˿���Ϣ)
	bzero(&dest,sizeof(dest));
	dest.sin_family = AF_INET;
	//dest.sin_port = htons(atoi(argv[2]));
	dest.sin_port = htons(3232);

	/*
	if(inet_aton(argv[1],(struct in_addr*)&dest.sin_addr.s_addr)==0)
	{
		perror(argv[1]);
		exit(errno);
	}
	*/
	dest.sin_addr.s_addr = inet_addr("192.168.18.128");

	printf("sockfd = %d\n",sockfd);

	printf("------ %s, prot%d,socked%d\n",
				inet_ntoa(dest.sin_addr),ntohs(dest.sin_port),sockfd);

	//���ӷ�����
	if(connect(sockfd,(struct sockaddr*)&dest, sizeof(dest)) != 0)
	{
		perror("connect\n");
		exit(errno);
	}
#endif
	//sockfd = ClientInit();

	printf("\n-------read to chatting\n");

	while(1)
	{
		//�Ѽ������
		FD_ZERO(&rfds);
		//�ѱ�׼���������뵽������
		FD_SET(0,&rfds);
		maxfd=0;
		//�ѵ�ǰ���Ӿ��socket ���뵽������
		FD_SET(sockfd,&rfds);
		if(sockfd > maxfd)
			maxfd = sockfd;
		//�������ȴ�ʱ��
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		//��ʼ�ȴ�
		retval = select(maxfd+1,&rfds,NULL,NULL,&tv);

		if(retval == -1)
		{
			printf("select error,quit\n");
			break;
		}
		else if(retval == 0)
		{
			continue;
		}
		else
		{
			if(FD_ISSET(sockfd,&rfds))
			{
				//���ӵ�socket ������Ϣ���ղ���ʾ
				bzero(buffer,MAXBUF+1);
				//���նԷ�����������Ϣ�����MAXBUF �ֽ�
				len = recv(sockfd,buffer,MAXBUF,0);

				if(len > 0)
				{
					printf("recv msg success:%s\n",buffer);
				}
				else
				{
					if(len < 0)
					{
						printf("error:%d,%s\n",errno,strerror(errno));
					}
					else
						printf("quit\n");
					break;
				}
			}//FD_ISSET = sockfd ���
			if(FD_ISSET(0,&rfds))
			{
				//�÷�������Ϣ����
				bzero(buffer,MAXBUF+1);
				fgets(buffer,MAXBUF,stdin);
				if(!strncasecmp(buffer,"quit",4))
				{
					printf("==quit\n");
					break;
				}
				len = send(sockfd, buffer, strlen(buffer)-1,0);
				if(len < 0)
				{
					printf("msg send fail\n");
					break;
				}
				else
				{
					printf("msg:%s --send ok\r\n",buffer);
				}
			}  //FD_ISSET = 0
		} //select �������
		
	}

	close(sockfd);


	return 0;
}

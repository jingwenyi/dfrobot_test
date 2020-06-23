/*********************************
**  Author : wenyi.jing
**  Date:	2016.4.14
**  Company: dfrobot
**  File name: dfrobot_client.cpp
**
**********************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>





#include "dfrobot_client.h"


#define  SOCK_TIMEOUT_SECOND       5
#define  RECV_BUF_SIZE             (8*1024*1024)

#define DFROBOT_MAX(_a, _b)			((_a) > (_b) ? (_a) : (_b))



//jwy add 2016.4.18 for cmd 

#define		CMD_SERVER_UDPPORT			0X01
#define		CMD_CLIENT_UDPADDR			0X02
#define		CMD_SERVER_UDPDATA			0X10
#define		CMD_SERVER_UDPDATA_JPEG		0X20

//add end







Dfrobot_client* Dfrobot_client::Create(char *ip, uint16_t serverPort)
{
	Dfrobot_client* result = new Dfrobot_client(ip, serverPort);
	if(result && (0 != result->Construct()))
	{
		delete result;
		result = NULL;
	}

	return result;
}

int Dfrobot_client::Construct()
{
	if(pipe(mPipe) < 0)
	{
		printf("pipe error\n");
		return -1;
	}

	mRecvData= new uint8_t[RECV_BUF_SIZE];
	if(mRecvData== NULL)
	{
		printf(" new buffer error\n");
		return -1;
	}
	if((mRecvMutex = CMutex::Create(false)) == NULL)
	{
		printf("mRecvMutex error\n");
		return -1;
	}
	
	if((mCondRecvData = CCondition::Create()) == NULL)
	{
		printf("mCondRecvData error\n");
		return -1;
	}
	return 0;
}


int Dfrobot_client::startClientThread()
{
	DFROBOT_DELETE(mClientThread);
	if(NULL == (mClientThread = CThread::Create("dfrobotClient", StaticThread, this)))
	{
		printf("failed to create client thread\n");
		return -1;
	}
	return 0;
}


int Dfrobot_client::GetRecvData(char *buff, int len)
{
	printf("GetRecvData  jia suo\n");
	mCondRecvData->Wait(mRecvMutex);
	if(mRcvLen > 0)
	{
		if(len < mRcvLen)
		{
			printf("len < mRcvLen\n");
			return -1;
		}
		memset(buff, 0, mRcvLen);
		mDataCount--;
		memcpy(buff, mRecvData, mRcvLen);
		printf("mDat");
		return mRcvLen;
	}
	else
	{
		return -1;
	}
}


int Dfrobot_client::SendData(char *buff,int dataLen)
{
	if(mSocketfd < 0)
	{
		printf("socket not exist\n");
		return -1;
	}
	else
	{
		if(send(mSocketfd, buff, dataLen, 0) < 0)
		{
			printf("send buff error\n");
			return -1;
		}

		return 0;
	}
}


int Dfrobot_client::ClientThread()
{
	
	int maxfd = -1;
	fd_set allset;
	fd_set fdset;

	struct timeval tv;

	tv.tv_sec = SOCK_TIMEOUT_SECOND;
	tv.tv_usec = 0;

	FD_ZERO(&allset);
	FD_SET(mSocketfd, &allset);
	FD_SET(CLIENT_CTRL_READ, &allset);
	FD_SET(mSocketUdp, &allset);

	maxfd = DFROBOT_MAX(mSocketfd, CLIENT_CTRL_READ);
	maxfd = DFROBOT_MAX(mSocketUdp,maxfd);
	//static int count=0;
	while(mRun)
	{
		int retval = -1;
		struct timeval tv;

		tv.tv_sec = SOCK_TIMEOUT_SECOND;
		tv.tv_usec = 0;

		fdset = allset;

		//printf("client select ....\n");
		if((retval = select(maxfd+1, &fdset, NULL, NULL, &tv)) > 0)
		{
			if(FD_ISSET(CLIENT_CTRL_READ, &fdset))
			{
				//处理自己给自己发的命令
				
			}
			else if(FD_ISSET(mSocketUdp, &fdset))
			{
				//printf("mSocketUdp----------\r\n");
				//char buf[100];
				AUTO_LOCK(mRecvMutex);
				//int len = recvfrom(mSocketUdp, buf, 100, 0,NULL, NULL);
				int rcvCnt = 0;
				int readCnt = 0;

				mRcvLen = 0;
				memset(mRecvData, 0, RECV_BUF_SIZE);//清空buffer

				mRcvLen = recvfrom(mSocketUdp, mRecvData, RECV_BUF_SIZE, 0, NULL, NULL);

				printf("mRcvLen=%x,%s\r\n",mRcvLen,mRecvData);
				//if(mRcvLen > 0)
				{
					//count++;
					//printf("c--%d--\n",count);
				}
#if 0
				do
				{
					rcvCnt = recvfrom(mSocketUdp, mRecvData, RECV_BUF_SIZE, 0, NULL, NULL);
					mRcvLen += rcvCnt;
					printf("rcvcnt=%d\n",rcvCnt);
					
				}while((++readCnt < 25) && ((rcvCnt > 0) && (errno != EINTR)));

				
				printf("mRcvLen=%d\r\n",mRcvLen);
#endif

				
#if 0
				sockaddr_in addr;
				socklen_t addrLen = sizeof(addr);
				memset(&addr, 0, sizeof(addr));
		
				getsockname(mSocketUdp,(struct sockaddr*)&addr,&addrLen);

				printf("my udp+++++ prot:%hu\n",addr.sin_port);
#endif
				
				//if(len > 0)
				//{
					//printf("buf:%s\r\n",buf);
				//}
			}
			else if(FD_ISSET(mSocketfd, &fdset))
			{
				
				//处理服务器发过来的数据
				AUTO_LOCK(mRecvMutex); //加锁

				unsigned char rcvBuf[50];
				int len;
				memset(rcvBuf, 0, sizeof(rcvBuf));

				if((len = recv(mSocketfd, rcvBuf, sizeof(rcvBuf), 0)) < 6)
				{
					printf("rcvLen < 6\n");
					continue;
				}
				else
				{
					int i;
					for(i=0; i<len; i++)
					{
						printf("%x\t", rcvBuf[i]);
					}
					printf("\n");
					if(CheckCrc(rcvBuf) == false)
					{
						printf("checkcrc false\n");
						continue;
					}
					else
					{
						
						if(rcvBuf[2] == CMD_SERVER_UDPPORT)
						{
							unsigned char clientAddrBuf[6] = {0x55, 0xaa, CMD_CLIENT_UDPADDR, 0};
							CalCrc(clientAddrBuf);
							mServerUdpPort = (rcvBuf[5] << 8) | rcvBuf[4];
							mServerUdpAddr.sin_port = htons(mServerUdpPort);
							printf("mSerVerUdpPort=%d\r\n", mServerUdpPort);
							
							//use udp fd sendto msg to server
							if(sendto(mSocketUdp, clientAddrBuf, sizeof(clientAddrBuf), 0,
										(struct sockaddr*)&mServerUdpAddr, sizeof(mServerUdpAddr)) < 0)
							{
								printf("client udp prot send error\n");
							}
						}
					}
					
				}
				
				
				


#if 0
				
				mRcvLen = 0;
				memset(mRecvData, 0, RECV_BUF_SIZE);//清空buffer
				
				mRcvLen = recv(mSocketfd, mRecvData, RECV_BUF_SIZE, 0);

				
			

				
				if(mRcvLen > 0)
				{
					//给接收数据的进程一个信号
					mDataCount++;
					//printf("send signal mRcvLen=%d\n",mRcvLen);
					mCondRecvData->Signal();
				}

				if(sendto(mSocketUdp, "i am client udp!", 100, 0,
								(struct sockaddr*)&mServerUdpAddr, sizeof(mServerUdpAddr)) < 0)
				{
					printf("udp send error\n");
				}
#endif

				
			}
		}
		else if(retval == 0)
		{
			//printf("timeout\n");
			//mRun = false;
			
		}
		else
		{
			printf("select error\n");
			mRun = false;
			
		}
	}

	if(mSocketfd >= 0)
	{
		close(mSocketfd);
		mSocketfd = -1;
	}
	
}


int Dfrobot_client::CalCrc(unsigned char* buf)
{
	if(buf == NULL)
		return -1;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return -1;
	int i;
	unsigned char crc0,crc1;
	int len = buf[3]+4;

	crc0 = buf[2];
	crc1 = crc0;

	for(i=3; i<len; i++)
	{
		crc0 += buf[i];
		crc1 += crc0;
	}
	
	buf[len++] = crc0;
	buf[len++] = crc1;

	return len; //返回buf 的有效长度
}

bool Dfrobot_client::CheckCrc(unsigned char *buf)
{
	
	if(buf == NULL)
		return false;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return false;
	int i;
	unsigned char crc0,crc1;
	int len = buf[3]+4;

	crc0 = buf[2];
	crc1 = crc0;

	for(i=3; i<len; i++)
	{
		crc0 += buf[i];
		crc1 += crc0;
	}

	if(buf[len++] != crc0)
		return false;
	if(buf[len] != crc1)
		return false;
	return true;
	
}



int Dfrobot_client::ClientInit()
{
	if(Buildtcp() != 0)
	{
		printf("build tcp error\n");
		return -1;
	}

	BuildUdp();
	
	return 0;
}

int Dfrobot_client::BuildUdp()
{
	//mServerUdpPort = 50000;

	mServerUdpAddr.sin_family = AF_INET;
	//mServerUdpAddr.sin_port = htons(mServerUdpPort);
	mServerUdpAddr.sin_addr.s_addr = inet_addr(mIp);

	printf("---udp--- %s, prot:%d\n",
				inet_ntoa(mServerUdpAddr.sin_addr),ntohs(mServerUdpAddr.sin_port));
	
	if((mSocketUdp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		printf("create client udp error\n");
		return -1;
	}
	else
	{
#if 0
		unsigned short port;
		sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		memset(&addr, 0, sizeof(addr));
		
		getsockname(mSocketUdp,(struct sockaddr*)&addr,&addrLen);
		port = ntohs(addr.sin_port);
		printf("---------udp ----port:%hu\n",port);
#endif
		int revbuf = RECV_BUF_SIZE;
		if(setsockopt(mSocketUdp, SOL_SOCKET, SO_RCVBUF, &revbuf, sizeof(revbuf)) != 0)
		{
			printf("set mSocketfd buf error\n");
			close(mSocketUdp);
			mSocketUdp = -1;
			return -1;
		}
	}

	printf("--------client udp ok---------\n");
	
	return 0;
}

int Dfrobot_client::Buildtcp()
{

	struct sockaddr_in dest;
	
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(mServerPort);
	dest.sin_addr.s_addr = inet_addr(mIp);

	//printf("------ %s, prot%d,socked%d\n",
				//inet_ntoa(dest.sin_addr),ntohs(dest.sin_port),mSocketfd);

	if((mSocketfd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("socket error\n");
		return -1;
	}
	else
	{
		int revbuf = RECV_BUF_SIZE;
		if(setsockopt(mSocketfd, SOL_SOCKET, SO_RCVBUF, &revbuf, sizeof(revbuf)) != 0)
		{
			printf("set mSocketfd buf error\n");
			close(mSocketfd);
			mSocketfd = -1;
			return -1;
		}
	}

	if(connect(mSocketfd, (struct sockaddr*)&dest, sizeof(dest)) != 0)
	{
		printf("connect failed \r\n");
		close(mSocketfd);
		mSocketfd = -1;
		return -1;
	}
	printf("connect ok\n");
 


	return 0;
}


int Dfrobot_client::Start()
{
	
	if(mRun == false)
	{
		
		if(ClientInit() != 0)
		{
			return -1;
		}
		if(startClientThread() != 0)
		{
			return -1;
		}
	}
	mRun = true;
	return 0;
}


Dfrobot_client::Dfrobot_client(char *ip, uint16_t serverPort):
	mServerPort(serverPort),
	mServerUdpPort(-1),
	mSocketfd(-1),
	mSocketUdp(-1),
	mRcvLen(0),
	mRecvData(NULL),
	mCondRecvData(NULL),
	mRecvMutex(NULL),
	mDataCount(0),
	mRun(false)
{
	memset(mIp, 0, sizeof(mIp));
	sprintf(mIp,"%s",ip);
	//printf("mIP:%s\n",mIp);
	memset(mPipe, -1, sizeof(mPipe));
	memset(&mServerUdpAddr, 0, sizeof(mServerUdpAddr));
}

	
Dfrobot_client::~Dfrobot_client()
{

	DFROBOT_DELETE(mCondRecvData);
	DFROBOT_DELETE(mRecvMutex);
	if(mPipe[0] >= 0)
	{
		close(mPipe[0]);
	}
	if(mPipe[1] >= 0)
	{
		close(mPipe[1]);
	}
	if(mSocketfd >= 0)
	{
		close(mSocketfd);
		mSocketfd = -1;
	}
	if(mSocketUdp >= 0)
	{
		close(mSocketUdp);
		mSocketUdp = -1;
	}

	if(mRecvData != NULL)
	{
		delete [] mRecvData;
		mRecvData = NULL;
	}
}


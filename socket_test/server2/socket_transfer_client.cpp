/**********************************
**
** 	Author: 		wenyi.jing  
** 	Date:	    	13/04/2016
** 	Company:	dfrobot
**	File name: 	socket_transfer_client.cpp
**
************************************/


#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>



#include "socket_transfer_client.h"
#include "socket_transfer_server.h"


#define  SEND_BUF_SIZE             (8*1024*1024)
#define  SOCK_TIMEOUT_SECOND       5


#define  CMD_CLIENT_ABORT          'a'
#define  CMD_CLIENT_STOP           'e'


//jwy add 2016.4.18 for cmd 

#define		CMD_SERVER_UDPPORT			0X01
#define		CMD_CLIENT_UDPADDR			0X02
#define		CMD_SERVER_UDPDATA_YUV		0X10
#define		CMD_SERVER_UDPDATA_JPEG		0X20

//add end






TransferClient::TransferClient(TransferServer* server, TransferClient**location, uint16_t Port):
	mTServer(server),
	mSelfLocation(location),
	mClientSessionId(0),
	mClientName(NULL),
	mTcpSock(-1),
	mTClientUdp(-1),
	mClientUdpOk(false),
	mClientThread(NULL),
	mSessionState(CLIENT_SESSION_INIT),
	mDynamicTimeOutSec(CLIENT_TIMEOUT_THRESHOLD),
	mUdpPort(Port)
{
	memset(&mClientAddr, 0, sizeof(mClientAddr));
	memset(&mClientUdpAddr, 0, sizeof(mClientUdpAddr));
}
TransferClient::~TransferClient()
{
	*mSelfLocation = NULL;

	DFROBOT_DELETE(mClientThread);
	CloseAllSocket();
	delete[] mClientName;
	
}


void TransferClient::CloseAllSocket()
{
	if(mTcpSock >= 0)
	{
		close(mTcpSock);
		mTcpSock = -1;
	}
	if(mTClientUdp >= 0)
	{
		close(mTClientUdp);
		mTClientUdp = -1;
	}
}



int TransferClient::ClientThread()
{
	bool run = true;
	int maxfd = -1;

	int readRet = 0;

	fd_set allset;
	fd_set fdset;

	timeval timeout = {CLIENT_TIMEOUT_THRESHOLD,0};
	
	FD_ZERO(&allset);
	FD_SET(mTcpSock, &allset);
	FD_SET(mTClientUdp, &allset);
	FD_SET(CLIENT_CTRL_READ, &allset);
	maxfd = DFROBOT_MAX(mTcpSock, mTClientUdp);
	maxfd = DFROBOT_MAX(maxfd, CLIENT_CTRL_READ);

	printf("new client:%s, prot %hu\n", inet_ntoa(mClientAddr.sin_addr),ntohs(mClientAddr.sin_port));

	signal(SIGPIPE, SIG_IGN);
#if 0  //发送server udp port

	char tmpbuf[10] = {0};
	sprintf(tmpbuf,"%04x",mUdpPort);
	printf("tmpbuf:%s\n",tmpbuf);
	struct TcpDataMsg udpport;
	udpport.cmd = CMD_DATA_UDP_PORT;
	udpport.data = tmpbuf;
	char testsendbuf[100];
	sprintf(testsendbuf, "%02x%s", udpport.cmd, udpport.data);

	printf("udpport cmd:%d,data:%s\n",udpport.cmd,udpport.data);
	printf("testsendbuf:%s\n",testsendbuf);

	if (send(mTcpSock, testsendbuf,strlen(testsendbuf),0) != (ssize_t)strlen(testsendbuf))
	{
		printf("server send udp addr error\n");
	}
#else //jwy add 2016.4.18
	unsigned char sendPort[8] = {0x55, 0xaa, CMD_SERVER_UDPPORT};
	sendPort[3] = 2;
	sendPort[4] = mUdpPort & 0xff;
	sendPort[5] = (mUdpPort >> 8) & 0xff;

/*
	for(int i=0; i<8; i++)
	{
		printf("buf[%d]:%x\t",i,sendPort[i]);
	}
	printf("\n");
*/
	int len = CalCrc(sendPort);
	printf("sendprot len=%d\r\n",len);

	if(send(mTcpSock, sendPort, len, 0) != len)
	{
		printf("server send udp port error\n");
	}
	
#endif


	while(run)
	{
		int retval = -1;
		timeval* tm = &timeout;
		timeout.tv_sec = mDynamicTimeOutSec;

		fdset = allset;
		if(retval = select(maxfd+1, &fdset, NULL, NULL, tm) > 0)
		{
			if(FD_ISSET(CLIENT_CTRL_READ, &fdset))
			{
				char cmd[1] = {0};
				int readCnt = 0;
				ssize_t readRet = 0;
				do
				{
					readRet = read(CLIENT_CTRL_READ, &cmd, sizeof(cmd));
				}while((++readCnt < 5) && ((readRet == 0)|| ((readRet < 0) && 
						((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))));
				if(readRet <= 0)
				{
					printf("read error22\n");
				}
				else if((cmd[0] == CMD_CLIENT_STOP) || (cmd[0] == CMD_CLIENT_ABORT))
				{
					printf("client:%s exit", mClientName);
					run = false;
					CloseAllSocket();
					break;
				}
				
			}
			else if(FD_ISSET(mTcpSock, &fdset))
			{
				//接收从客服端发过来的消息并处理回复消息
				int len;
				char buf[100] = {0};
				int readCnt = 0;

				do
				{
					readRet = recv(mTcpSock, buf, 99, 0);
				}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));

#if 1

				if(readRet > 0)
				{
					printf("recivce msg:%s\n",buf);
					//if (send(mTcpSock,"i am server tcp-----",100,0) < 0)
					//{
					//	printf("server tcp send error\n");
					//}
					
				}
				else if(readRet <= 0)
				{
					if((errno == ECONNRESET) || (readRet == 0))
					{
						if(mTcpSock >= 0)
						{
							close(mTcpSock);
							mTcpSock = -1;
						}
						printf("close mTcpsock\n");
					}
					else
					{
						printf("recv error\n");
					}
					run = false;
					continue;
				}
#endif
				
			}
			else if(FD_ISSET(mTClientUdp, &fdset))
			{
				unsigned char buf[50];
				int len;
				memset(buf, 0, sizeof(buf));
				struct sockaddr_in clientAddr;
				socklen_t addrLen = sizeof(clientAddr);
				memcpy(&clientAddr, &mClientAddr, sizeof(clientAddr));
				clientAddr.sin_port = htons(mUdpPort);
				if((len = recvfrom(mTClientUdp, buf, sizeof(buf), 0,(struct sockaddr*)&clientAddr, &addrLen)) < 0)
				{
					printf("recvfrom error\n");
				}
				else
				{
					if(len < 6)
					{
						printf("len < 6 :%d,buf:%s\r\n",len,buf);
						continue;
					}
					else if(CheckCrc(buf) == false)
					{
						printf("server udp data check error,buf:%s\n",buf);
						continue;
					}
					else if(buf[2] == CMD_CLIENT_UDPADDR)
					{
						if(mClientUdpOk == false)
						{
							memcpy(&mClientUdpAddr, &clientAddr, sizeof(clientAddr));
							mClientUdpOk= true;
						}
					#if 1
						printf("rcv msg client udpaddr\n");
						if(sendto(mTClientUdp, "i am server udp-----", 100, 0,
									(struct sockaddr*)&mClientUdpAddr, sizeof(mClientUdpAddr)) < 0)
						{
							printf(" server udp sendto error\n");
						}
					#endif
					}
				
				}

				

#if 0 //test
				//test udp
				char testbuf[100] = {0};
				int ret = -1;
				struct sockaddr_in clientAddr;
				socklen_t addrLen = sizeof(clientAddr);
				memcpy(&clientAddr, &mClientAddr, sizeof(clientAddr));
				clientAddr.sin_port = htons(mUdpPort);

				if((ret = recvfrom(mTClientUdp, testbuf, sizeof(testbuf), 0,
										(struct sockaddr*)&clientAddr, &addrLen)) < 0)
				{
					printf("udp recvfrom error\n");
				}
				else
				{
					//udp 数据不处理，值打印
					printf("udp rec msg:%x\n",testbuf[2]);
					//printf("clientaddr:%s, clientaddr:%s\n", inet_ntoa(clientAddr.sin_addr),
													//		   inet_ntoa(clientAddr.sin_addr));
					printf("client udp --------:%s, prot %hu\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
					if(sendto(mTClientUdp, "i am server udp", 100, 0,
									(struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0)
					{
						printf(" server udp sendto error\n");
					}
				}
#endif
				
			}
			
			
		}
		else if(retval == 0)
		{
			//printf("client: %s is not responding within %d seconds, shutdown!\n",mClientName,mDynamicTimeOutSec);
			//run = false;
			//CloseAllSocket();
			
		}
		else
		{
			perror("select client\n");
			run = false;
		}
	}

	mSessionState = CLIENT_SESSION_STOPPED;
	

	if(!run)
	{
		int count = 0;
		bool val = false;
		while((++count < 5)&& !(val = mTServer->DeleteClientSession(mSelfLocation)))
		{
			usleep(5000);
		}

		if((count >= 5) && !val)
		{
			printf("TServer is dead before client session exit\n");
		}
		
	}
	
	
	return 0;
}

void TransferClient::ClientAbort()
{
	while(mSessionState == CLIENT_SESSION_INIT)
	{
		usleep(100000);
	}

	if((mSessionState == CLIENT_SESSION_OK) || (mSessionState == CLIENT_SESSION_THREAD_RUN))
	{
		char cmd[1] = {CMD_CLIENT_ABORT};
		int count = 0;

		while((++count < 5) && (1 != write(CLIENT_CTRL_WRITE, cmd, 1)))
		{
			if((errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR))
			{
				perror("write\n");
				break;
			}
		}
		printf("abort command to client:%s\n",mClientName);
	}
	else
	{
		printf("this client is not initialized successfully!\n");
	}
	
	
}


int TransferClient::CalCrc( unsigned char* buf)
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


bool TransferClient::CheckCrc(unsigned char *buf)
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


int TransferClient::SetupServerSocketUdp(uint16_t streamPort)
{
	int reuse = 1;
	int sock = -1;
	 struct sockaddr_in streamAddr;

	 printf("server udp port:%d\n",streamPort);

	 memset(&streamAddr, 0, sizeof(streamAddr));
	 streamAddr.sin_family = AF_INET;
	 streamAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	 streamAddr.sin_port = htons(streamPort);

	//printf("----------SetupServerSocketUdp------------\n");

	 if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	 {
		perror("socket client\n");
	 }
	 else
	 {
		int sendBuf = SEND_BUF_SIZE;
		if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0) &&
			(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendBuf, sizeof(sendBuf))== 0))
		{
			if(bind(sock, (struct sockaddr*)&streamAddr, sizeof(streamAddr)) != 0)
			{
				close(sock);
				sock = -1;
				perror("bind client\n");
			}
			else
			{
				//printf("=====================================\n");
				if(mTServer->mSendNeedWait)// go here
				{
					timeval timeout = {SOCK_TIMEOUT_SECOND*3, 0};
					if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) != 0)
					{
						close(sock);
						sock = -1;
						printf("setsockopt client\n");
					}
					else if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) != 0)
					{
						close(sock);
						sock = -1;
						printf("setsockopt client\n");
					}
					printf("---------create udp ok------------\r\n");
				}
				else
				{
					int flag = fcntl(sock, F_GETFL, 0);
					flag |= O_NONBLOCK;
					if(fcntl(sock, F_SETFL, flag) != 0)
					{
						close(sock);
						sock = -1;
						perror("fcntl client\n");
					}
				}
			}
		}
		else
		{
			close(sock);
			sock = -1;
			perror("setsockopt client\n");
		}
		
	 }

	 return sock;
}


bool TransferClient::InitClient(int serverTcpSock)
{
	bool ret = false;
	
	mSessionState = CLIENT_SESSION_INIT;
	
	if(pipe(mClientCtrlSock) < 0)
	{
		perror("client pipe\n");
		mSessionState = CLIENT_SESSION_FAILED;
	}
	else if(mTServer && mSelfLocation && (serverTcpSock >=0))
	{
		unsigned int acceptCnt = 0;
		socklen_t sockLen = sizeof(mClientAddr);
		//产生一个sessionId
		mClientSessionId = mTServer->GetRandomNumber();

		do{
			mTcpSock = accept(serverTcpSock, (sockaddr*)&(mClientAddr), &sockLen);
		}while((++acceptCnt < 5) && (mTcpSock < 0) && 
				((errno == EAGAIN)|| (errno == EWOULDBLOCK) || (errno == EINTR)));

		printf("mTcpsock = %d\n",mTcpSock);
		if(mTcpSock >= 0)
		{
			//这里只建立一个UDP 服务和建立连接的client 通信
			mTClientUdp = SetupServerSocketUdp(mUdpPort);
		}
		else
		{
			perror("accept\n");
		}

		printf("mTcpsock=%d,mTclientUdp=%d\n",mTcpSock,mTClientUdp);
		if((mTcpSock >= 0) && (mTClientUdp >= 0))
		{
			int	sendBuf = SEND_BUF_SIZE;
			int noDelay = 1;
			timeval timeout = {SOCK_TIMEOUT_SECOND, 0};

			if(setsockopt(mTcpSock, IPPROTO_TCP, TCP_NODELAY,
								&noDelay, sizeof(noDelay)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else if(setsockopt(mTcpSock, SOL_SOCKET, SO_SNDBUF,
									&sendBuf, sizeof(sendBuf)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else if(setsockopt(mTcpSock, SOL_SOCKET, SO_RCVTIMEO,
									(char*)&timeout, sizeof(timeout)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else if(setsockopt(mTcpSock, SOL_SOCKET, SO_SNDTIMEO,
									(char*)&timeout, sizeof(timeout)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else
			{
				char clientName[128] = {0};
				sprintf(clientName, "%s-%hu",inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
				SetClientName(clientName);
				mClientThread = CThread::Create(mClientName, StaticClientThread, this);

				if(!mClientThread)
				{
					printf("failed to create client thread for %s\n",clientName);
					mSessionState = CLIENT_SESSION_FAILED;
				}
				else
				{
					ret = true;
					printf("----------client session ok----------\r\n");
					mSessionState = CLIENT_SESSION_THREAD_RUN;
					mSessionState = CLIENT_SESSION_OK;
				}
				
				printf("+++++++++++++++++++++++\r\n");
				
			}
			
		}
		
		
	}
	else //失败打印出原因
	{
		if(!mTServer)
		{
			printf("invalid tcp server\n");
		}
		if(!mSelfLocation)
		{
			printf("invalid location\n");
		}
		if(serverTcpSock < 0)
		{
			printf("invalid tcp sever socket\n");
		}
		mSessionState = CLIENT_SESSION_FAILED;
	}
	
	return ret;
}





/*********************************
**  Author : wenyi.jing
**  Date:	2016.4.12
**  Company: dfrobot
**  File name: socket_transfer_server.cpp
**
**********************************/

#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>


#include "socket_transfer_server.h"
#include "socket_transfer_client.h"
#include "osal_linux.h"


#define  SERVER_MAX_LISTEN_NUM     20
#define  SERVER_CTRL_READ          mPipe[0]
#define  SERVER_CTRL_WRITE         mPipe[1]


#define  CMD_DEL_CLIENT             'd'
#define  CMD_ABRT_ALL               'a'
#define  CMD_STOP_ALL               'e'


struct ServerCtrlMsg
{
    uint32_t code;
    void*    data;
};




TransferServer* TransferServer::Create()
{
	TransferServer* result = new TransferServer();

	if(result && (result->Construct() < 0))
	{
		perror("pipe\n");
		delete result;
		result = NULL;
	}


	return result;
	
}



TransferServer::TransferServer():
	mSrvSockTcp(-1),
	mPortTcp(DFROBOT_SERVER_PORT),
	mSrvThread(-1),
	mThread(NULL),
	mClientMutex(NULL),
	mMutex(NULL),
	mSendNeedWait(true),//设置成发送需要等待
	mRun(false)
{
	memset(mPipe, -1, sizeof(mPipe));
	memset(mClientData, 0, sizeof(TransferClient*) * MAX_CLIENT_NUMBER);
}

TransferServer::~TransferServer()
{
	if(mSrvSockTcp >= 0)
	{
		close(mSrvSockTcp);
	}
	
	DFROBOT_DELETE(mThread);
	DFROBOT_DELETE(mClientMutex);
	DFROBOT_DELETE(mMutex);

	if(mPipe[0] >= 0)
	{
		close(mPipe[0]);
	}

	if(mPipe[1] >= 0)
	{
		close(mPipe[1]);
	}

	for (int i = 0; i < MAX_CLIENT_NUMBER; ++ i)
	{
    	delete mClientData[i];
  	}
	
}


int TransferServer::Construct()
{
	if(NULL == (mMutex = CMutex::Create()))
	{
		printf("mMutex error\n");
		return -1;
	}
	if(NULL == (mClientMutex = CMutex::Create()))
	{
		printf("mClientMutex error\n");
		return -1;
	}
	
	if(pipe(mPipe) < 0)
	{
		perror("pipe\n");
		return -1;
	}

	return 0;
}



bool TransferServer::DeleteClientSession(TransferClient** data)
{
	ServerCtrlMsg msg;
	msg.code = CMD_DEL_CLIENT;
	msg.data = ((void*)data);
	return (ssize_t)sizeof(msg) == write(SERVER_CTRL_WRITE, &msg, sizeof(msg));
}



uint32_t TransferServer::GetRandomNumber()
{
  struct timeval current = {0};
  gettimeofday(&current, NULL);
  srand(current.tv_usec);
  return (rand() % ((uint32_t)-1));
}


void TransferServer::SendYuvDataToClient(uint8_t *buf,int len)
{
	static int count = 0;
	if(mClientMutex == NULL)
	{
		return ;
	}
	AUTO_LOCK(mClientMutex);
	int i;
	int sendRet = -1;
	printf("-----------test1----------\r\n");
	for(i=0; i<MAX_CLIENT_NUMBER; ++i)
	{
		if(mClientData[i])
		{
			printf("---------test2---------\r\n");
			if(mClientData[i]->mClientUdpOk == false)
			{
				continue;
			}
			
			sendRet = sendto(mClientData[i]->mTClientUdp, buf, len, 0, 
								(struct sockaddr*)&(mClientData[i]->mClientUdpAddr), sizeof(mClientData[i]->mClientUdpAddr));
			if(sendRet < 0)
			{
				printf("sever send yuv error:%s, prot %hu\n",
					inet_ntoa(mClientData[i]->mClientUdpAddr.sin_addr),ntohs(mClientData[i]->mClientUdpAddr.sin_port));
			}
			else
			{
				count++;
				printf("--%d--\n",count);
			}
			
				
			
		}
	}
}



bool TransferServer::SetupServerSocketTcp(uint16_t port)
{
	bool ret = false;
	int reuse = 1;
	printf("SetupServerSocketTcp start \n");
	struct sockaddr_in serverAddr;
	
	mPortTcp = port;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family      = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port        = htons(mPortTcp);

	if((mSrvSockTcp = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) < 0)
	{
		perror("socket\n");
		//exit(errno);
	}
	else
	{
		if(setsockopt(mSrvSockTcp, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0)
		{
			int flag = fcntl(mSrvSockTcp, F_GETFL, 0);
			flag |= O_NONBLOCK;
			if(fcntl(mSrvSockTcp, F_SETFL, flag) != 0)
			{
				perror("fcntl\n");
			}
			else if(bind(mSrvSockTcp, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
			{
				perror("bind\n");
			}
			else if(listen(mSrvSockTcp, SERVER_MAX_LISTEN_NUM) < 0)
			{
				perror("listen\n");
			}
			else
			{
				ret = true;
			}
		}
		else
		{
			perror("setsockopt\n");
		}
	}

	printf("SetupServerSocketTcp ok \n");
	return ret;
	
}


void TransferServer::AbortClient(TransferClient& client)
{
	client.ClientAbort();
	while(client.mClientThread->IsThreadRunning())
	{
		printf("wait for client %s to exit", client.mClientName);
		usleep(10000);
	}
}



int TransferServer::ServerThread()
{
	int maxfd = -1;
	fd_set allset;
	fd_set fdset;

	
	FD_ZERO(&allset);
	FD_SET(mSrvSockTcp, &allset);
	FD_SET(SERVER_CTRL_READ,&allset);
	maxfd = DFROBOT_MAX(mSrvSockTcp, SERVER_CTRL_READ);
	

	signal(SIGPIPE, SIG_IGN);
	printf("------------serverThread-------------\n");
	while(mRun)
	{
		fdset = allset;
		printf("wait for client connect......\n");
		if(select(maxfd+1, &fdset, NULL, NULL, NULL) > 0)
		{
			
			if(FD_ISSET(SERVER_CTRL_READ, &fdset))
			{
				ServerCtrlMsg msg = {0};
				int readCnt = 0;
				ssize_t readRet = 0;

				do
				{
					readRet= read(SERVER_CTRL_READ, &msg, sizeof(msg));
				}while((++readCnt < 5) && ((readRet == 0) ||
								((readRet < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))));

				if(readRet < 0)
				{
					printf("read error11\n");
				}
				else
				{
					if(msg.code == CMD_DEL_CLIENT)
					{
						AUTO_LOCK(mClientMutex);
						TransferClient** client = (TransferClient**)msg.data;
						if(*client)
						{
							printf("server delete client %s",(*client)->mClientName);
							delete *client;
						}
					}
					else if((msg.code == CMD_ABRT_ALL) || (msg.code == CMD_STOP_ALL))
					{
						AUTO_LOCK(mClientMutex);
						mRun = false;
						close(mSrvSockTcp);
						mSrvSockTcp = -1;
						int i;
						for(i=0; i<MAX_CLIENT_NUMBER; ++i)
						{
							if(mClientData[i])
							{
								AbortClient(*mClientData[i]);
								delete mClientData[i];
							}
						}
					}
				}
			}
			else if(FD_ISSET(mSrvSockTcp, &fdset))
			{
#if 0  //test
				printf("client connect\n");
				int new_fd;
				socklen_t len;
				struct sockaddr_in their_addr;
				accept(mSrvSockTcp,(struct sockaddr *)&their_addr,&len);
				printf("server:get connection from %s, prot%d,socked%d\n",
				inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),new_fd);
#endif //test
				
				
#if 1
				AUTO_LOCK(mClientMutex);
				TransferClient** self = NULL;
				int i;
				for(i=0; i<MAX_CLIENT_NUMBER; ++i)
				{
					if(!mClientData[i])
					{
						self = &mClientData[i];
						*self = new TransferClient(this, self, (STREAM_PORT_BASE+i));
						break;
					}
				}
				if(!(*self))
				{
					printf("maximum client number is has reached\n");
				}
				else if(!(*self)->InitClient(mSrvSockTcp))
				{
					//失败的情况稍后处理
					printf(" initClient error\n");
					
					if(*self)
					{
						delete *self;
					}
				}
#endif
				
			}
		}
		else
		{
			//perror("select\n");
			printf("slelect error\n");
			break;
		}
		
	}
	
}

bool TransferServer::StartServerThread()
{
	bool ret = true;


	DFROBOT_DELETE(mThread);
	
	if(NULL == (mThread = CThread::Create("TServer", staticServerThread,this)))
	{
		perror("create TServer---------\n");
		ret = false;
	}
	
	return ret;
}


bool TransferServer::start(uint16_t tcpPort)
{
	if(mRun == false)
	{
		mRun = StartServerThread() && SetupServerSocketTcp(tcpPort);
	}

	return mRun;
}





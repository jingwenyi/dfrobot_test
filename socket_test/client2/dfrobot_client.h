/*********************************
**  Author : wenyi.jing
**  Date:	2016.4.14
**  Company: dfrobot
**  File name: dfrobot_client.h
**
**********************************/
#ifndef  _DFROBOT_CLIENT_H_
#define _DFROBOT_CLIENT_H_

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "osal_linux.h"

#define DFROBOT_SERVER_PORT   3232


#define CLIENT_CTRL_READ  mPipe[0]
#define CLIENT_CTRL_WRITE mPipe[1]


class Dfrobot_client
{

public:
	Dfrobot_client(char *ip, uint16_t serverPort);
	~Dfrobot_client();

public:
	static Dfrobot_client* Create(char* ip, uint16_t serverPort);
	int Start();
	int GetRecvData(char *buff, int len);
	int SendData(char *buff,int dataLen);

	void Delete()
	{
		 delete this;
	}

private:
	int Construct();
	int ClientInit();
	int startClientThread();
	static int StaticThread(void *data)
	{
		return ((Dfrobot_client*)data)->ClientThread();
	}
	int ClientThread();
	int Buildtcp();
	int BuildUdp();
	int CalCrc(unsigned char* buf);
	bool CheckCrc(unsigned char *buf);
private:
	uint16_t				mServerPort;
	uint16_t				mServerUdpPort;
	char					mIp[50];
	int                 	mPipe[2];
	int 					mSocketfd;
	int 					mSocketUdp;
	CThread					*mClientThread;
	bool                	mRun;
	int						mRcvLen;
	uint8_t					*mRecvData;
	CMutex					*mRecvMutex;
	CCondition 				*mCondRecvData;
	int						mDataCount;
	sockaddr_in          	mServerUdpAddr;
	
};

#endif //_DFROBOT_CLIENT_H_



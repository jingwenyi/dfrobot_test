/*********************************
**  Author : wenyi.jing
**  Date:	2016.4.12
**  Company: dfrobot
**  File name: socket_transfer_server.h
**
**********************************/


#ifndef _SOCKET_TRANSFER_SERVER_H_
#define _SOCKET_TRANSFER_SERVER_H_

#include <stdint.h>
#include <pthread.h>

#include "osal_linux.h"
#include "socket_transfer_client.h"




#define DFROBOT_SERVER_PORT   3232
#define STREAM_PORT_BASE 50000


#define DFROBOT_MAX(_a, _b)			((_a) > (_b) ? (_a) : (_b))



class TransferClient;


class TransferServer
{

	friend class TransferClient;

	enum {
      MAX_CLIENT_NUMBER = 32
    };
public:
	TransferServer();
	~TransferServer();

	
public:
	static TransferServer* Create();
	bool start(uint16_t tcpPort = DFROBOT_SERVER_PORT);

	uint32_t GetRandomNumber();
	void SendYuvDataToClient(uint8_t *buf, int len);

private:
	static int staticServerThread(void *data)
	{
		return ((TransferServer*)data)->ServerThread();
	}
	

private:
	bool StartServerThread();
	bool SetupServerSocketTcp(uint16_t port);
	int  ServerThread();
	int  Construct();
	bool DeleteClientSession(TransferClient** data);
	void AbortClient(TransferClient& client);

private:
	int                 mPipe[2];
	uint16_t 			mPortTcp;
	int                 mSrvSockTcp;
	pthread_t 			mSrvThread;
	bool                mRun;
	CThread            	*mThread;
	CMutex             	*mClientMutex;
	CMutex             	*mMutex;
	TransferClient		*mClientData[MAX_CLIENT_NUMBER];
	bool                mSendNeedWait;
	
};


#endif  //_SOCKET_TRANSFER_SERVER_H_




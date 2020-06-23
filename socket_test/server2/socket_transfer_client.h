/**********************************
**
** 	Author: 		wenyi.jing  
** 	Date:	    	13/04/2016
** 	Company:	dfrobot
**	File name: 	socket_transfer_client.h
**
************************************/
#ifndef _SOCKET_TRANSFER_CLIENT_H_
#define _SOCKET_TRANSFER_CLIENT_H_

#include <stdint.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>


#include "socket_transfer_server.h"

#define  CLIENT_TIMEOUT_THRESHOLD  30


class TransferServer;



class TransferClient
{
	friend class TransferServer;

	enum ClientSessionState {
      CLIENT_SESSION_INIT,
      CLIENT_SESSION_THREAD_RUN,
      CLIENT_SESSION_OK,
      CLIENT_SESSION_FAILED,
      CLIENT_SESSION_STOPPED
    };
public:
	TransferClient(TransferServer* server, TransferClient**location, uint16_t Port);
	~TransferClient();
	bool InitClient(int serverTcpSock);

	void ClientAbort();

	void Delete()
    {
      delete this;
    }

	char* amstrdup(const char* str)
	{
  		char* newstr = NULL;
  		if (str) 
		{
    		newstr = new char[strlen(str) + 1];
    		if (newstr) 
			{
      			memset(newstr, 0, strlen(str) + 1);
      			strcpy(newstr, str);
    		}
  		}
		return newstr;
  	}

	void SetClientName(const char* nm)
    {
      delete[] mClientName;
      mClientName = NULL;
      if (nm) {
        mClientName = amstrdup(nm);
      }
    }


private:
	int CalCrc(unsigned char* buf);
	bool CheckCrc(unsigned char *buf);
	int SetupServerSocketUdp(uint16_t streamPort);
	static int StaticClientThread(void *data)
	{
		return ((TransferClient*)data)->ClientThread();
	}

	int ClientThread();

	void CloseAllSocket();

private:
	TransferServer*			mTServer;
	TransferClient**		mSelfLocation;
	uint16_t				mUdpPort;
	ClientSessionState   	mSessionState;
	int	               		mClientCtrlSock[2];
	sockaddr_in          	mClientAddr;
	sockaddr_in				mClientUdpAddr;
	bool					mClientUdpOk;
	uint32_t               	mClientSessionId;
	int 					mTcpSock;
	int						mTClientUdp;
	char*                	mClientName;
	CThread*             	mClientThread;
	int		               	mDynamicTimeOutSec;
	#define CLIENT_CTRL_READ  mClientCtrlSock[0]
	#define CLIENT_CTRL_WRITE mClientCtrlSock[1]
};


#endif //_SOCKET_TRANSFER_CLIENT_H_


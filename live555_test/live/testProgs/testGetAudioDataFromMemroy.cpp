/*
** filename: testGetAudioDataFromMemroy.cpp
** author: jingwenyi
** date: 2016.06.15
*/


#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "ADTSAudioLiveSource.hh"
#include "ADTSAudioLiveServerMediaSubsession.hh"
#include <string.h>

UsageEnvironment* env;


Boolean reuseFirstSource = False;

Boolean iFramesOnly = False;


unsigned char *audioBuffer = NULL;
unsigned char *pData = NULL;
long int audioSize = 0;
long int usedSize = 0;

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName); // fwd

static int rtspGetAudioData(unsigned char* buffer, int len);
void setLiveSourceCallBack(ADTSAudioLiveSource* subsms);


int main(int argc, char** argv) 
{
	audioBuffer = new unsigned char[1024*1024]; // 1M
	if(audioBuffer == 	NULL)
	{
		printf("new audio buffer error\r\n");
		return -1;
	}

	FILE* fid;
	fid = fopen("test.aac", "rb");
	if(fid == NULL)
	{
		printf("open test.aac error\r\n");
		delete []audioBuffer;
		return -1;
	}

	audioSize = fread(audioBuffer, 1, 1024*1024, fid);
	printf("----------retSize=%ld----------\r\n",audioSize);
	fclose(fid);

	pData = audioBuffer;

	
	

	// Begin by setting up our usage environment:
  	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  	env = BasicUsageEnvironment::createNew(*scheduler);

  	UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  	// To implement client access control to the RTSP server, do the following:
  	authDB = new UserAuthenticationDatabase;
  	authDB->addUserRecord("username1", "password1"); // replace these with real strings
  	// Repeat the above with each <username>, <password> that you wish to allow
  	// access to the server.
#endif

	// Create the RTSP server:
 	RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554, authDB);
 	if (rtspServer == NULL) {
   		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		delete []audioBuffer;
		fclose(fid);
   		exit(1);
 	}


 	char const* descriptionString
    	= "Session streamed by \"testOnDemandRTSPServer\"";

 
	// An AAC audio stream (ADTS-format file):
  	{
    	char const* streamName = "aacAudioTest";
    	char const* inputFileName = "test.aac";
    	ServerMediaSession* sms
     	 = ServerMediaSession::createNew(*env, streamName, streamName,
				      descriptionString);
		ADTSAudioLiveServerMediaSubsession *subsms = ADTSAudioLiveServerMediaSubsession::createNew(*env, reuseFirstSource, 1, 8000, 1);
		
    	sms->addSubsession(subsms);
		subsms->CallBackSubSms = setLiveSourceCallBack;
    	rtspServer->addServerMediaSession(sms);

    	announceStream(rtspServer, sms, streamName, inputFileName);
  	}

	env->taskScheduler().doEventLoop(); // does not return

	delete []audioBuffer;
 	return 0; // only to prevent compiler warning

}


static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}

static int rtspGetAudioData(unsigned char* buffer, int len)
{
	if(usedSize == audioSize)
	{
		return 0;
	}
	if(audioSize > (usedSize + len))
	{
		memcpy(buffer, &pData[usedSize], len);
		usedSize += len;
		return len;
	}
	else
	{
		int availSize = audioSize - usedSize;
		
		memcpy(buffer, &pData[usedSize], availSize);
		//usedSize = audioSize;
		usedSize = 0;
		return availSize;
	}
}


void setLiveSourceCallBack(ADTSAudioLiveSource* subsms)
{
	subsms->getAudioData = rtspGetAudioData;
}














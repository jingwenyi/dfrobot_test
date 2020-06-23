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
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <list>
#include <iostream>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>



using namespace std;



typedef struct AUDIO_LIST
{
	uint8_t* data;
	long int len;
	struct AUDIO_LIST* next;
}dfrobot_audio_list, *Pdfrobot_audio_list;


typedef struct AUDIO_LIST_HEADER
{
	dfrobot_audio_list* head;
	dfrobot_audio_list* tail;
	pthread_mutex_t mutex;
	int list_size;
}dfrobot_auido_list_header, *Pdfrobot_audio_list_header;

dfrobot_auido_list_header* list_header_init();

dfrobot_audio_list* list_init(int len);

void list_add_tail(dfrobot_auido_list_header *header, dfrobot_audio_list *list);

void list_delete_head(dfrobot_auido_list_header *header);

void list_destroy(dfrobot_audio_list* list);

void list_header_destroy(dfrobot_auido_list_header *header);

dfrobot_audio_list* list_pop_head(dfrobot_auido_list_header *header);



dfrobot_auido_list_header* g_list_header;


typedef list<uint8_t*>  LIST_UINT8_T;

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
void LiveSourceCloseNotify(ADTSAudioLiveSource* subsms);

#if 1

dfrobot_auido_list_header* header;// = list_header_init();


long fd;

int main(int argc, char** argv) 
{
	header = list_header_init();
	audioBuffer = new unsigned char[1024*1024]; // 1M
	if(audioBuffer == 	NULL)
	{
		printf("new audio buffer error\r\n");
		return -1;
	}

	fd = open("/mnt/testGetsave2.acc",  O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);

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

	unsigned char buff[1000];
	int ret = 0;
	while(1)
	{
		
		ret = rtspGetAudioData(buff,1000);
		if(ret == 0)
		{
			printf("header size =%d\r\n",header->list_size);
			break;
		}
	}
	

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
		ADTSAudioLiveServerMediaSubsession *subsms = ADTSAudioLiveServerMediaSubsession::createNew(*env, reuseFirstSource, 1, 44100, 2);
		
    	sms->addSubsession(subsms);
		subsms->CallBackSubSms = setLiveSourceCallBack;
		subsms->CallBackSubSmsClose = LiveSourceCloseNotify;
    	rtspServer->addServerMediaSession(sms);

    	announceStream(rtspServer, sms, streamName, inputFileName);
  	}

	env->taskScheduler().doEventLoop(); // does not return

	delete []audioBuffer;
	close(fd);
 	return 0; // only to prevent compiler warning

}
#endif



static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}

static int rtspGetAudioData3(unsigned char* buffer, int len)
	{
		if(usedSize == audioSize)
		{
			return 0;
		}
		int temp_len;
#if 1
		unsigned char headers[7];
		//rtspGetAudioData(headers, sizeof headers);
		
		memcpy(headers, &pData[usedSize], 7);
		usedSize += 7;
		
		u_int16_t frame_length
			= ((headers[3]&0x03)<<11) | (headers[4]<<3) | ((headers[5]&0xE0)>>5);
		unsigned numBytesToRead
			= frame_length > sizeof headers ? frame_length - sizeof headers : 0;
		printf("size=%d\r\n",numBytesToRead);
		temp_len = numBytesToRead;
#endif
	
#if 0
		if(audioSize > (usedSize + temp_len))
		{
			memcpy(buffer, &pData[usedSize], temp_len);
			usedSize += temp_len;
			return temp_len;
		}
		else
		{
			int availSize = audioSize - usedSize;
			
			memcpy(buffer, &pData[usedSize], availSize);
			//usedSize = audioSize;
			usedSize = 0;
			return availSize;
		}
#else
		//dfrobot_auido_list_header* header = list_header_init();
		if(header == NULL)
		{
			printf("list header == NULL\r\n");
		}
		dfrobot_audio_list* list =	list_init(temp_len);
		//dfrobot_audio_list* list2 =  list_init(temp_len);
		if(list == NULL)
		{
			printf("list==NULL--\r\n");
		}
		if(audioSize > (usedSize + temp_len))
		{
			memcpy(list->data, &pData[usedSize], temp_len);
			//memcpy(list2->data, &pData[usedSize], temp_len);
			usedSize += temp_len;
			//return temp_len;
		}
		else
		{
			//int availSize = audioSize - usedSize;
			temp_len = audioSize - usedSize;
			
			memcpy(list->data, &pData[usedSize], temp_len);
			//memcpy(list2->data, &pData[usedSize], temp_len);
			usedSize = audioSize;
			//usedSize = 0;
			//return availSize;
		}
		list_add_tail(header,list);
		//list_add_tail(header,list2);
		//list_add_tail(g_list_header,list);
		list = NULL;
		//list2 = NULL;
#if 0
		//list = list_pop_head(header);
		//if(list != NULL)
		//{
		//	list_destroy(list);
		//	list = NULL;
		//}
		list = list_pop_head(header);
		if(list == NULL)
		{
			printf("list === NULL--\r\n");
		}
	
		
	
		memcpy(buffer, list->data, list->len);
	
		list_destroy(list);
		list = NULL;
	
		//list_header_destroy(header);
#endif
	
		return temp_len;
		
#endif
	
	}


static int rtspGetAudioData(unsigned char* buffer, int len)
{
	if(usedSize == audioSize)
	{
		return 0;
	}
	int temp_len;
#if 1
	unsigned char headers[7];
	//rtspGetAudioData(headers, sizeof headers);
	
	memcpy(headers, &pData[usedSize], 7);
	usedSize += 7;
	
	u_int16_t frame_length
    	= ((headers[3]&0x03)<<11) | (headers[4]<<3) | ((headers[5]&0xE0)>>5);
	unsigned numBytesToRead
    	= frame_length > sizeof headers ? frame_length - sizeof headers : 0;
	printf("size=%d\r\n",numBytesToRead);
	temp_len = numBytesToRead;
#endif

#if 0
	if(audioSize > (usedSize + temp_len))
	{
		memcpy(buffer, &pData[usedSize], temp_len);
		usedSize += temp_len;
		return temp_len;
	}
	else
	{
		int availSize = audioSize - usedSize;
		
		memcpy(buffer, &pData[usedSize], availSize);
		//usedSize = audioSize;
		usedSize = 0;
		return availSize;
	}
#else
	//dfrobot_auido_list_header* header = list_header_init();
	if(header == NULL)
	{
		printf("list header == NULL\r\n");
	}
	dfrobot_audio_list* list =  list_init(temp_len);
	//dfrobot_audio_list* list2 =  list_init(temp_len);
	if(list == NULL)
	{
		printf("list==NULL--\r\n");
	}
	if(audioSize > (usedSize + temp_len))
	{
		memcpy(list->data, &pData[usedSize], temp_len);
		//memcpy(list2->data, &pData[usedSize], temp_len);
		usedSize += temp_len;
		//return temp_len;
	}
	else
	{
		//int availSize = audioSize - usedSize;
		temp_len = audioSize - usedSize;
		
		memcpy(list->data, &pData[usedSize], temp_len);
		//memcpy(list2->data, &pData[usedSize], temp_len);
		usedSize = audioSize;
		//usedSize = 0;
		//return availSize;
	}
	list_add_tail(header,list);
	//list_add_tail(header,list2);
	//list_add_tail(g_list_header,list);
	list = NULL;
	//list2 = NULL;
#if 0
	//list = list_pop_head(header);
	//if(list != NULL)
	//{
	//	list_destroy(list);
	//	list = NULL;
	//}
	list = list_pop_head(header);
	if(list == NULL)
	{
		printf("list === NULL--\r\n");
	}

	

	memcpy(buffer, list->data, list->len);

	list_destroy(list);
	list = NULL;

	//list_header_destroy(header);
#endif

	return temp_len;
	
#endif

}

static int rtspGetAudioData2(unsigned char* buffer, int len)
{
#if 0
	dfrobot_audio_list* list;
	list = list_pop_head(g_list_header);
	if((list != NULL) && (list->data != NULL))
	{
		//int size = len > list->len ? list->len : len;
		//printf("size=%d\r\n",size);
		printf("g list header size=%d, size=%d\r\n", g_list_header->list_size, list->len);
		memcpy(buffer, list->data, list->len);
		list_destroy(list);
		list = NULL;
	}
#endif
	dfrobot_audio_list* list =  list_pop_head(header);
	if(list == NULL)
	{
		printf("no audio date\r\n");
		return 0;
	}
	memcpy(buffer, list->data, list->len);
	//write(fd,list->data, list->len);
	//write(fd, buffer, list->len);
	int tem_len = list->len;
	list_destroy(list);
	list = NULL;
	return tem_len;
	
}

void setLiveSourceCallBack(ADTSAudioLiveSource* subsms)
{
	subsms->getAudioData = rtspGetAudioData2;
	printf("++++++++++++create live source+++++++++++++++++++\r\n");
}

void LiveSourceCloseNotify(ADTSAudioLiveSource* subsms)
{
	printf("-------LiveSourceCloseNotify-----------\r\n");
}




#if 0

int main()
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
	g_list_header = list_header_init();

	//读出每个音频数据放到list 中
	int ret;
	while(1)
	{
		#if 0
		unsigned char headers[7];
		ret = rtspGetAudioData(headers, sizeof headers);
		if(ret == 0)break;
		u_int16_t frame_length
    		= ((headers[3]&0x03)<<11) | (headers[4]<<3) | ((headers[5]&0xE0)>>5);
		unsigned numBytesToRead
    		= frame_length > sizeof headers ? frame_length - sizeof headers : 0;
		printf("size=%d\r\n",numBytesToRead);
		if(numBytesToRead > 0)
		{
			dfrobot_audio_list* list = list_init(numBytesToRead);
			
			printf("-----test1---\r\n");
			ret = rtspGetAudioData(list->data, numBytesToRead);
			printf("---test2---\r\n");
			list_add_tail(g_list_header, list);
			printf("---test3--\r\n");
			list = NULL;
		}
		#endif
		unsigned char buffer[1000];
		int ret = rtspGetAudioData(buffer, 1000);
		if(ret == 0)
		{
			printf("list size=%d\r\n", g_list_header->list_size);
			break;
		}
		
		
	}
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
		ADTSAudioLiveServerMediaSubsession *subsms = ADTSAudioLiveServerMediaSubsession::createNew(*env, reuseFirstSource, 1, 44100, 2);
		
    	sms->addSubsession(subsms);
		subsms->CallBackSubSms = setLiveSourceCallBack;
		subsms->CallBackSubSmsClose = LiveSourceCloseNotify;
    	rtspServer->addServerMediaSession(sms);

    	announceStream(rtspServer, sms, streamName, inputFileName);
  	}

	env->taskScheduler().doEventLoop(); // does not return

	delete []audioBuffer;
 	return 0; // only to prevent compiler warning

	

	return 0;
}

#endif




dfrobot_auido_list_header* list_header_init()
{
	dfrobot_auido_list_header* header = (dfrobot_auido_list_header*)malloc(sizeof(dfrobot_auido_list_header));
	if(header == NULL)
	{
		return NULL;
	}
	header->head = NULL;
	header->tail = NULL;
	header->list_size = 0;
	pthread_mutex_init(&(header->mutex), NULL);
	return header;
}

dfrobot_audio_list* list_init(int len)
{
	if(len <= 0)
	{
		return NULL;
	}
	dfrobot_audio_list* list = (dfrobot_audio_list*)malloc(sizeof(dfrobot_audio_list));
	if(list == 	NULL)
	{
		printf("list=null\r\n");
		return NULL;
	}
	list->data = (uint8_t*)malloc(len);
	if(list->data == NULL)
	{
		free(list);
		return NULL;
	}
	list->len = len;
	list->next = NULL;
	return list;
}

void list_add_tail(dfrobot_auido_list_header *header, dfrobot_audio_list *list)
{
	if((header == NULL) || (list == NULL))
	{
		printf("list add tail NULL\r\n");
		return;
	}
	pthread_mutex_lock(&(header->mutex));
	
	if(header->list_size == 0)
	{
		header->head = list;
		header->tail = list;
	}
	else
	{
		header->tail->next = list;
		header->tail = list;
	}
	header->list_size++;
	
	pthread_mutex_unlock(&(header->mutex));
}

void list_delete_head(dfrobot_auido_list_header *header)
{
#if 0
	if(header == NULL) return;
	if(header->head == NULL) return;
	
	pthread_mutex_lock(&(header->mutex));
	
	dfrobot_audio_list* list = header->head;
	header->head = list->next;
	header->list_size--;
	if(header->list_size == 0)
	{
		header->head = NULL;
		header->tail = NULL;
	}
	list_destroy(list);
	list = NULL;
	pthread_mutex_unlock(&(header->mutex));
#endif
	dfrobot_audio_list* list = list_pop_head(header);
	list_destroy(list);
	list = NULL;

}

dfrobot_audio_list* list_pop_head(dfrobot_auido_list_header *header)
{
	if(header == NULL)
	{
		printf("list pop header NULL \r\n");
		return NULL;
	}
	if(header->head == NULL)
	{
		printf("list pop header->head NULL\r\n");
		return NULL;
	}
	
	pthread_mutex_lock(&(header->mutex));
	
	dfrobot_audio_list* list = header->head;
	header->head = list->next;
	header->list_size--;
	if(header->list_size == 0)
	{
		header->head = NULL;
		header->tail = NULL;
	}
	
	pthread_mutex_unlock(&(header->mutex));

	return list;
}

void list_destroy(dfrobot_audio_list* list)
{
	if(list == NULL) return;
	
	if(list->data != NULL)
	{
		free(list->data);
	}
	list->next = NULL;
	free(list);
	list = NULL;
}

void list_header_destroy(dfrobot_auido_list_header *header)
{
	if(header == NULL) return;
	while(header->head != NULL)
	{
		dfrobot_audio_list* list = header->head;
		header->head = list->next;
		list_destroy(list);
		list = NULL;
	}
	header->head = NULL;
	header->tail = NULL;
	pthread_mutex_destroy(&(header->mutex));
	free(header);
}








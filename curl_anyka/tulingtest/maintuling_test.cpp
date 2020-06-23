/*
**
** jingwenyi test curl
**
**
*/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "curl/curl.h"
#include "dfrobot_tuling.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>




#if 0


char *curl_buffer = NULL;

size_t my_write_func(void *ptr, size_t size, size_t nmemb, void *stream)
{
	memset(curl_buffer, 0, 100*1024);
	printf("\r\n-----------size=%d,nmemb=%d-------------\r\n",size,nmemb);
	memcpy(curl_buffer, ptr,size*nmemb);
	
	//printf("%s\r\n",curl_buffer);
	return (size*nmemb);
}


int main()
{


	CURL *curl;
	CURLcode res;

	curl_buffer = (char*) malloc(100*1024);//100k

	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "http://www.baidu.com");
		//Ö¸Ã÷stream µØÖ·
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_write_func);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	//getchar();

	while(1)
	{
		sleep(5);
	}
	free(curl_buffer);
	
	return 0;
}

#endif

//char* tmp_curl_buffer = NULL;

size_t my_write_func(void *ptr, size_t size, size_t nmemb, void *stream)
{
	char* tmp_curl_buffer = (char *)malloc((size*nmemb+1));
	tmp_curl_buffer = (char *)malloc((size*nmemb+1));
	memset(tmp_curl_buffer, 0, (size*nmemb+1));
	printf("\r\n-----------size=%d,nmemb=%d-------------\r\n",size,nmemb);
	memcpy(tmp_curl_buffer, ptr,size*nmemb);
	
	
	printf("%s\r\n",tmp_curl_buffer);
	free(tmp_curl_buffer);
	return (size*nmemb);
}


size_t my_write_func_mp3(void *ptr, size_t size, size_t nmemb, void *stream)
{
	printf("---------size*nmemb=%d\r\n",size*nmemb);
	long fd = open("tuling.mp3", O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
	if(fd < 0)
	{
		printf("open tuling.mp3 error\r\n");
		return 0;
	}
	write(fd, ptr, size*nmemb);
	close(fd);
	
	return (size*nmemb);
}


size_t my_write_func_mp4(void *ptr, size_t size, size_t nmemb, char **result)
{
	printf("---------size*nmemb=%d\r\n",size*nmemb);
	long fd = open("tuling.mp3", O_CREAT | O_APPEND | O_TRUNC | O_WRONLY);
	if(fd < 0)
	{
		printf("open tuling.mp3 error\r\n");
		return 0;
	}
	write(fd, ptr, size*nmemb);
	close(fd);
	
	return (size*nmemb);
}




int main()
{
	int i;
	#if 0
	if(tmp_curl_buffer != NULL)
	{
		free(tmp_curl_buffer);
		tmp_curl_buffer = NULL;
	}
	#endif
	//talk_with_tuling("hello", &my_write_func);
	//talk_with_tuling("hello,what your name?", &my_write_func_mp3);
	//get_baidu_tts_api_access_token();
	//text_to_mp3("hello", &my_write_func);

	//char* tmp = talk_to_tuling("hello");
	//printf("tmp=%s\r\n",tmp);

	char *tmp = get_baidu_api_access_tocken();
	printf("tmp:%s\r\n",tmp);

	
#if 0

	uint8_t *pcm_data = (uint8_t *)malloc(2*1024*1024);
	if(pcm_data == NULL)
	{
		printf("pcm_data malloc error\r\n");
		return -1;
	}

	memset(pcm_data, 0, 2*1024*1024);

	long fd = open("test.pcm",O_RDONLY);
	if(fd < 0)
	{
		printf("fd < 0 \r\n");
		free(pcm_data);
		return -1;
	}
	size_t pcm_data_len;
	pcm_data_len = read(fd, pcm_data, 2*1024*1024);

	printf("pcm_data_len:%d\r\n", pcm_data_len);

	close(fd);

	
	talk_to_talk_with_tuling(pcm_data, pcm_data_len, my_write_func_mp4);

	//pcm_to_text(pcm_data, pcm_data_len,NULL);
	
#endif
	

	sleep(5);
	return 0;

}

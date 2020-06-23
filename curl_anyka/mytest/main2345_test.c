/*
**
** jingwenyi test curl
**
**
*/


#include <stdio.h>
#include <time.h>
#include "curl/curl.h"

#define DFROBOT_BAIDU_APIID			"5882776"
#define	DFROBOT_BAIDU_APIKEY		"kxEPGh7TXL3javiTkCsoRlfG"
#define	DFROBOT_BAIDU_APISECRTKEY	"8fdc8211c0fadf4cfeab7f4e9647823c"
#define	DFROBOT_BAIDU_LANGUAGE		"zh"



int main()
{

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(curl)
	{
		//curl_easy_setopt(curl, CURLOPT_URL, "https://bbs.byr.cn");
		curl_easy_setopt(curl, CURLOPT_URL,
			"https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id="DFROBOT_BAIDU_APIKEY
			"&client_secret="DFROBOT_BAIDU_APISECRTKEY);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		printf("---------res=%d-----------\r\n",res);
	}

	getchar();
	
	return 0;
}

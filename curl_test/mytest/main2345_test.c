/*
**
** jingwenyi test curl
**
**
*/


#include <stdio.h>
#include "curl/curl.h"




int main()
{

	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, "https://www.baidu.com");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}

	getchar();
	
	return 0;
}

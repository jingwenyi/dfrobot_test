#include <stdio.h>
#include <time.h>



int main()
{
	time_t now;
	now = time(NULL);
	struct tm *timeinfo;
	timeinfo = localtime(&now);
	char buf[100];
	strftime(buf, sizeof(buf), "%Y%m%d%H%S", timeinfo);
	printf("%s\r\n",buf);

}

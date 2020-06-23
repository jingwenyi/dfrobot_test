#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

int main()
{
	struct ifaddrs *ifAddrStrcut = NULL;
	void *tmpAddrPtr = NULL;
	getifaddrs(&ifAddrStrcut);
	
	while(ifAddrStrcut != NULL)
	{
		if(ifAddrStrcut->ifa_addr->sa_family == AF_INET) //ipv4
		{
			tmpAddrPtr = &((struct sockaddr_in*)ifAddrStrcut->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			printf("%s:%s\r\n", ifAddrStrcut->ifa_name, addressBuffer);			
		}
		else if(ifAddrStrcut->ifa_addr->sa_family == AF_INET6)
		{
			tmpAddrPtr = &((struct sockaddr_in*)ifAddrStrcut->ifa_addr)->sin_addr;
			char addressBuffer[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
			printf("%s:%s\r\n", ifAddrStrcut->ifa_name, addressBuffer);
		}

		ifAddrStrcut = ifAddrStrcut->ifa_next;
		
	}
	return 0;
}

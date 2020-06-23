#include <stdio.h>
#include <string.h>


int main()
{
	char buff[100] = "auto,12345678khd=kjdopngjsj,";
	unsigned char buff2[100] = {0};
	memcpy(buff2, buff, strlen(buff));
	char buff3[100] = {0};
	memcpy(buff3, buff2, strlen(buff));
	printf("%s\r\n",buff3);
	
	return 0;
}

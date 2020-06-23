/*
**
** test config file
**
*/

#include <stdio.h>
#include "read_config.h"

#define DFROBOT_CONFIG_FILE	"dfrobot_config"	

typedef struct dfrobot_config{
	//broad name
	char *name;         
	//wifi
	char *ssid;
	char *password;
} df_config;

df_config globle_config;

int main()
{
	char *des = NULL;
	des = read_element("ssid", DFROBOT_CONFIG_FILE);
	if(des != NULL)
	{
		printf("ssid:%s\r\n",des);
	}
	des = read_element("name",DFROBOT_CONFIG_FILE);
	if(des != NULL)
	{
		printf("name:%s\r\n",des);
	}

	
	des = read_element("password",DFROBOT_CONFIG_FILE);
	if(des != NULL)
	{
		printf("password:%s\r\n",des);
	}

	des = read_element("passwor",DFROBOT_CONFIG_FILE);
	if(des != NULL)
	{
		printf("password1:%s\r\n",des);
	}

	write_element("ssid", "jingwenyhi",DFROBOT_CONFIG_FILE);
	
	return 0;
}

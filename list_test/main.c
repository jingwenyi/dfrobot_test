
#include <stdio.h>
#include "audio_list.h"



dfrobot_auido_list_header* header;


int main()
{
	header = list_header_init();
	int i=0;

	char buf[100];
	
	for(i=1; i<100; i++)
	{
		dfrobot_audio_list* list = list_init( (1000+i));
		printf("before = list->len = %d \r\n", list->len);
		sprintf(buf,"i am %d",i);
		memcpy(list->data, buf, 20);
		list_add_tail(header, list);
		list = NULL;
	}

	while(header->head != NULL)
	{
		dfrobot_audio_list* list = list_pop_head(header);
		printf("pop list->len=%d,%s\r\n",list->len, list->data);

		list_destroy(list);
		list = 	NULL;
	}
	
	
	list_header_destroy(header);

	return 0;
}

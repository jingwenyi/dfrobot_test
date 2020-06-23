#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define NOISE_THRESHOLD	100000
#define VOICE_MAX_SIZE  (1*1024*1024)
#define	VOICE_START_OK	5
#define VOICE_END_OK	5

int file_count = 1;


//该函数以100000 为阈值截取数据，测试结果


typedef enum{
	ANALYZE_NO_DATA,
	ANALYZE_START_RUNING,
	ANALYZE_RUNING,
	ANALYZE_END_RUNING
}analyze_stat;



typedef struct guess_data{
	uint8_t* data;
	int len;
	analyze_stat status;
	int start_flag;
	int end_flag;
}GUESS_DATA, *PGUESS_DATA; 

GUESS_DATA *voice_data = NULL;

enum{
	ERR_OK,
	ERR_MALLOC_FAILED,
	ERR_INPUT_NULL
};




int audio_analyze_init()
{
	voice_data = (GUESS_DATA*)malloc(sizeof(GUESS_DATA));
	if(voice_data == NULL)
	{
		printf("voice_data malloc error\r\n");
		return ERR_MALLOC_FAILED;
	}

	voice_data->len = 0;
	voice_data->data = (uint8_t*)malloc(VOICE_MAX_SIZE);
	voice_data->status = ANALYZE_NO_DATA;
	voice_data->end_flag = 0;
	voice_data->start_flag = 0;
	if(voice_data->data == NULL)
	{
		printf("voice_data data malloc error\r\n");
		free(voice_data);
		return ERR_MALLOC_FAILED;
	}
	
	return ERR_OK;
}

void audio_analyze_exit()
{
	if(voice_data != NULL)
	{
		if(voice_data->data != NULL)
		{
			free(voice_data->data);
			voice_data->data = NULL;
		}
		free(voice_data);
		voice_data = NULL;
	}
}

int audio_frame_analyze(uint8_t* frame, int frame_len)
{
	 long long int sum = 0;
	 if(frame == NULL)
	 {
		printf("frame is null\r\n");
		return ERR_INPUT_NULL;
	 }
	 //连续5个包大于10000,为语音数据开始，连续5个包小于10000，为语音数据结束
	
	 int i;
	 uint8_t *tmp = frame;
	 unsigned char *sample = (unsigned char *)malloc(2);
	 for(i=0; i<frame_len; )
	 {
	 	sample[0] = tmp[i++];
		sample[1] = tmp[i++];
	 	short int *samplenum = NULL;
	 	samplenum = (short *)sample;
		//printf("samplenum = %d\r\n",*samplenum);
		sum += abs(*samplenum);
	 }
	 free(sample);

	 if(sum >= NOISE_THRESHOLD)
	 {
	 	if(voice_data->status == ANALYZE_RUNING)
	 	{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
		}
	 	else if(voice_data->status == ANALYZE_START_RUNING)
		{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			if((++voice_data->start_flag)> VOICE_START_OK)
			{
				voice_data->status = ANALYZE_RUNING;
			}
		}
		else if(voice_data->status== ANALYZE_NO_DATA)
		{
			voice_data->status = ANALYZE_START_RUNING;
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			voice_data->start_flag++;
		}
		else if(voice_data->status == ANALYZE_END_RUNING)
		{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			if((--voice_data->end_flag) <= 0)
			{
				voice_data->end_flag = 0;
				voice_data->status = ANALYZE_RUNING;
			}
		}
		
	 }
	 else
	 {
		if(voice_data->status == ANALYZE_START_RUNING)
		{
			voice_data->status = ANALYZE_NO_DATA;
			voice_data->start_flag = 0;
			voice_data->len = 0;
		}
		else if(voice_data->status == ANALYZE_END_RUNING)
		{
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			if((++voice_data->end_flag) > VOICE_END_OK)
			{
				char buf[50] = {0};
				sprintf(buf, "file_cut_out_voice%d.pcm", file_count);
				FILE *fd = fopen(buf, "wb+");
				fwrite(voice_data->data, 1, voice_data->len, fd);
				fclose(fd);
				file_count++;

				
				voice_data->status = ANALYZE_NO_DATA;
				voice_data->end_flag = 0;
				voice_data->len = 0;
				voice_data->start_flag = 0;
			}
		}
		else if(voice_data->status == ANALYZE_RUNING)
		{
			voice_data->status  = ANALYZE_END_RUNING;
			memcpy(&voice_data->data[voice_data->len], frame, frame_len);
			voice_data->len += frame_len;
			voice_data->end_flag++;
		}
		
	 }
	 
	 
	
}


int main()
{
	unsigned char buf[320];
	FILE *pcm_file = fopen("test16k6.pcm", "rb+");

	audio_analyze_init();
	while(!feof(pcm_file))
	{
		fread(buf, 1, 320, pcm_file);
		audio_frame_analyze(buf, 320);
	}

	audio_analyze_exit();

	return 0;
}


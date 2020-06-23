/*
** filename: ADTSAudioLiveSource.cpp
** author: jingwenyi
** date:2016.06.14
**
*/
#include "ADTSAudioLiveServerMediaSubsession.hh"
#include "ADTSAudioLiveSource.hh"
#include "InputFile.hh"
#include <GroupsockHelper.hh>


//typedef int callFunc(unsigned char *buffer, int len);


static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};


ADTSAudioLiveSource* 
ADTSAudioLiveSource::createNew(UsageEnvironment& env, u_int8_t profile, 
									unsigned int samplingFrequency, u_int8_t channel)
{
	do
	{
		if(profile == 3)
		{
			env.setResultMsg("Bad (reserved) 'profile': 3 in first frame of ADTS file");
			break;
		}

		u_int8_t sampling_frequency_index = 15;
		int i=0;
		for(i=0; i<13; i++)
		{
			if(samplingFrequency == samplingFrequencyTable[i])
			{
				sampling_frequency_index = i;
				break;
			}
		}

		if (samplingFrequencyTable[sampling_frequency_index] == 0) 
		{
      		env.setResultMsg("Bad 'sampling_frequency_index' in first frame of ADTS file");
     		 break;
    	}

		return new ADTSAudioLiveSource(env, profile,
				  				 sampling_frequency_index, channel);

	}while(0);
	
	return NULL;
}

ADTSAudioLiveSource

::ADTSAudioLiveSource(UsageEnvironment& env, u_int8_t profile,
		      u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration)
	:FramedSource(env)
{
	fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
	fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
	fuSecsPerFrame
		= (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;
	unsigned char audioSpecificConfig[2];
	u_int8_t const audioObjectType = profile + 1;
	audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
  	audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
	sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

}

ADTSAudioLiveSource::~ADTSAudioLiveSource() {
}



void ADTSAudioLiveSource::doGetNextFrame() 
{

	// Begin by reading the 7-byte fixed_variable headers:
  	unsigned char headers[7];
	
	if(getAudioData(headers, sizeof headers) < sizeof headers)
	{
		printf("--doGetNextFrame--:no audio data\r\n");
	}
	// Extract important fields from the headers:
  	Boolean protection_absent = headers[1]&0x01;
	u_int16_t frame_length
    	= ((headers[3]&0x03)<<11) | (headers[4]<<3) | ((headers[5]&0xE0)>>5);
	unsigned numBytesToRead
    	= frame_length > sizeof headers ? frame_length - sizeof headers : 0;
	// If there's a 'crc_check' field, skip it:
  	if (!protection_absent) {
    	//SeekFile64(fFid, 2, SEEK_CUR);
		unsigned char seek_buff[2];
		getAudioData(seek_buff, 2);
    	numBytesToRead = numBytesToRead > 2 ? numBytesToRead - 2 : 0;
  	}

	 // Next, read the raw frame data into the buffer provided:
  	if (numBytesToRead > fMaxSize) {
    	fNumTruncatedBytes = numBytesToRead - fMaxSize;
    	numBytesToRead = fMaxSize;
  	}

	int numBytesRead = getAudioData(fTo, numBytesToRead);
	if (numBytesRead < 0) numBytesRead = 0;
	fFrameSize = numBytesRead;
	fNumTruncatedBytes += numBytesToRead - numBytesRead;


	// Set the 'presentation time':
 	if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
   	 	// This is the first frame, so use the current time:
    	gettimeofday(&fPresentationTime, NULL);
  	} else {
   	 // Increment by the play time of the previous frame:
    	unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
    	fPresentationTime.tv_sec += uSeconds/1000000;
    	fPresentationTime.tv_usec = uSeconds%1000000;
  	}

	fDurationInMicroseconds = fuSecsPerFrame;

	// Switch to another task, and inform the reader that he has data:
  	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
						(TaskFunc*)FramedSource::afterGetting, this);
}




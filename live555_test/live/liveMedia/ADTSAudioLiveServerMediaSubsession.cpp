/*
** filename:ADTSAudioLiveServerMediaSubsession.cpp
** author: jingwenyi
** date: 2016.06.14
**
*/

#include "ADTSAudioLiveServerMediaSubsession.hh"
#include "MPEG4GenericRTPSink.hh"





ADTSAudioLiveServerMediaSubsession*
ADTSAudioLiveServerMediaSubsession::createNew(UsageEnvironment& env, Boolean reuseFirstSource, u_int8_t profile, 
									unsigned int samplingFrequency, u_int8_t channel)
{
	
	return  new ADTSAudioLiveServerMediaSubsession(env, reuseFirstSource, profile, samplingFrequency, channel);
	
}


ADTSAudioLiveServerMediaSubsession
::ADTSAudioLiveServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource, u_int8_t profile, 
									unsigned int samplingFrequency, u_int8_t channel)
	: OnDemandServerMediaSubsession(env, reuseFirstSource), 
	profile(profile),
	samplingFrequency(samplingFrequency),
	channel(channel)
{
}

ADTSAudioLiveServerMediaSubsession
::~ADTSAudioLiveServerMediaSubsession()
{
}

FramedSource* ADTSAudioLiveServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned & estBitrate)
{
	printf("++++++++++++createNewStreamSource++++++++++++++++++\r\n");
	estBitrate = 96; // kbps, estimate
	ADTSAudioLiveSource* test = 
	ADTSAudioLiveSource:: createNew(envir(), profile,
													samplingFrequency, channel);
	CallBackSubSms(test);
	return test;
	
}


RTPSink* ADTSAudioLiveServerMediaSubsession
::createNewRTPSink(Groupsock * rtpGroupsock,
					unsigned char rtpPayloadTypeIfDynamic,
					FramedSource * inputSource)
{
	ADTSAudioLiveSource* adtsSource = (ADTSAudioLiveSource*)inputSource;
	return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
						rtpPayloadTypeIfDynamic,
						adtsSource->samplingFrequency(),
						"audio", "AAC-hbr", adtsSource->configStr(),
						adtsSource->numChannels());
}










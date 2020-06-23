/*
** filename: h264FramedLiveSource.hh
** Auther: jingwenyi
** data: 2016.06.13
**
*/

#ifndef _H264FRAMEDLIVESOURCE_HH_
#define _H264FRAMEDLIVESOURCE_HH_

#include <FramedSource.hh>


class H264FramedLiveSource : public FramedSource
{
public:
	static H264FramedLiveSource *createNew(UsageEnvironment& env,
			char const* fileName,
			unsigned preferredFrameSize = 0,
			unsigned playTimePerFrame = 0);
protected:
	H264FramedLiveSource(UsageEnvironment& env,
			char const* fileName,
			unsigned preferredFrameSize,
			unsigned playTimePerFrame);
	~H264FramedLiveSource();

private:
	virtual void doGetNextFrame();
	int TransportData(unsigned char *to, unsigned maxSize);

protected:
	FILE *fp;
};



#endif //_H264FRAMEDLIVESOURCE_HH_
/*
** Filename: H264LiveVideoServerMediaSubssion.hh
** Author: jingwenyi
** data: 2016.06.13
**
*/


#ifndef _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_
#define _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_

#include "H264VideoFileServerMediaSubsession.hh"

class H264LiveVideoServerMediaSubssion: public H264VideoFileServerMediaSubsession {

public:
  static H264LiveVideoServerMediaSubssion*
  createNew( UsageEnvironment& env,
               char const* fileName, 
                Boolean reuseFirstSource );

protected: // we're a virtual base class
  H264LiveVideoServerMediaSubssion( UsageEnvironment& env, char const* fileName, Boolean reuseFirstSource );
   ~H264LiveVideoServerMediaSubssion();

protected: // redefined virtual functions
   FramedSource* createNewStreamSource(unsigned clientSessionId,
                          unsigned& estBitrate);
public: 
   char fFileName[100];

};

#endif  //_H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH_


    

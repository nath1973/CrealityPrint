#ifndef PLAYER_FFMPEG_WEBRTC_DECODER_H_
#define PLAYER_FFMPEG_WEBRTC_DECODER_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
//extern "C"
//{
#define __STDC_CONSTANT_MACROS
//#include "video/yangrecordthread.h"
#include "yangplayer/YangPlayerHandle.h"
#include "yangstream/YangStreamType.h"
//#include "yangplayer/YangPlayWidget.h"
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangSysMessageI.h>
#include <yangutil/sys/YangSocket.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangMath.h>

//}
#include <future>
#include <functional>
class WebRTCDecoder :  public YangSysMessageI
{
    enum Status {
    STOPPED = 1,
    CONNECTTING = 2,
    CONNECTED = 3
    };
public:
    static WebRTCDecoder* GetInstance();
    void stopPlay();
    void startPlay(const std::string& strUrl,const std::function<void(std::vector<unsigned char>&)> recevie_frame);
    WebRTCDecoder();
    ~WebRTCDecoder();
    void success();
    void failure(int32_t errcode);
    bool isStop() { return m_isStop; }
    int width();
    int height();
    bool receiveFrame(); 
    std::vector<unsigned char> getFrameData();

private:
    bool m_isStop = false;
    Status m_status = STOPPED;
    YangPlayerHandle* m_player;
    YangFrame m_frame;
    static WebRTCDecoder *g_pSingleton;
    const std::function<void(std::vector<unsigned char>&)> m_recevie_frame_callback;
protected:
    YangContext* m_context;
    std::string m_url;
    std::future<void> m_playFutrue;
    int m_width=1920;
    int m_height=1080;
};

#endif // ! PLAYER_FFMPEG_WEBRTC_DECODER_H_
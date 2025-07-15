/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_DECODER_H_
#define _C_VIDEO_DECODER_H_


#include <atomic>
#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>
//#include "Extension/frame.h"
//#include "Extension/Track.h"
//#ifdef __cplusplus
//extern "C" {
//    #endif 
////#include <libavutil/avutil.h>
//#include <libavutil/frame.h>
//// #include <libavutil/avutil.h>
//#include <libavcodec/avcodec.h>
//#include <libavfilter/avfilter.h>
//#include <libavfilter/buffersink.h>
//#include <libavfilter/buffersrc.h>
//#include <libavformat/avformat.h>
////#include <libavutil/cpu.h>
////#include <libavutil/display.h>
////#include <libavutil/imgutils.h>
////#include <libavutil/opt.h>
//#include <libswscale/swscale.h>
//#ifdef __cplusplus
//}

//#endif // #ifdef __cplusplus
//#include <Extension/Frame.h>
#include <functional>
//#include "cVideoEncoder.h"
#include "NvDecoder.h"
#include "Record/Recorder.h"
#include "Common/ctranscode_info_mgr.h"
#include "ccuvid_video_render.h"
namespace mediakit {



     class FrameDispatcher;


    class cVideoEncoder;
     class Frame;
    typedef std::function<void(std::shared_ptr<Frame> frame)> DISPATHFRAME;
     
	class cVideoDecoder : public cv::cudacodec::EncoderCallback 
	{
     public:
        
    private:
       
        typedef std::condition_variable ccond;
	public:
         cVideoDecoder()
            : m_stoped (true)
            , m_packet_queue ()
             , m_encoder (nullptr)
            , m_nv_decoder(nullptr)
            , m_frame_callback(nullptr)
             , m_count(0)
             , m_cuvid_video_render(nullptr)
             , m_media()
             , m_transcode_info()
        {}
        virtual ~cVideoDecoder() {}

	public:
        bool init(DISPATHFRAME  callback/*std::shared_ptr< FrameDispatcher> ptr*/, const  MediaTuple  & media);
        //void push();
        void push_frame(/*const char *data, int32_t size*/ const std::shared_ptr<Frame> &frame);



        const MediaTuple&  get_media() const { return m_media; }


       CodecId     get_video_codec()   ;
    public:

          void onEncoded(const std::vector<std::vector<uint8_t>>& vPacket, const std::vector<uint64_t>& pts) ;

        /** @brief Set the GOP pattern used by the encoder.

         @param frameIntervalP Specify the GOP pattern as follows : \p frameIntervalP = 0: I, 1 : IPP, 2 : IBP, 3 : IBBP.
        */
          bool setFrameIntervalP(const int frameIntervalP)  ;

        /** @brief Callback function to that the encoding has finished.
        * */
          void onEncodingFinished()  ;
	private:
        void _work_thread();
        void _encoder_thread();
    private:
        void _handler_packet_item(std::shared_ptr<Frame>& av_packet);
	private:
        
	/*		AVCodecContext *m_c;
        AVCodecParserContext *m_parser;*/
            std::atomic < bool> m_stoped;
        std::thread m_thread;

        std::mutex m_lock;
        std::list<std::shared_ptr<Frame>> m_packet_queue;
        ccond m_condition;

         std::shared_ptr < cVideoEncoder> m_encoder;


         //std::shared_ptr<NvDecoder> m_nv_decoder;
         NvDecoder *m_nv_decoder;
        //std::shared_ptr<FrameDispatcher> _FrameWriterInterface;

         DISPATHFRAME m_frame_callback;
         int32_t m_count;


         std::shared_ptr< dsp::ccuvid_video_render>   m_cuvid_video_render;


         std::thread   m_encoder_thread;

         MediaTuple   m_media;
        // mediakit::CodecId     m_video_codec;

         dsp::ctranscode_info    m_transcode_info;
         int64_t        m_frame_samples { 90000 / 25 };
          
	};
}

#endif // _C_VIDEO_DECODER_H_
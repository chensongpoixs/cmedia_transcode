/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_ENCODER_H_
#define _C_VIDEO_ENCODER_H_
 
#include "Extension/Frame.h"
//#include "Extension/Track.h"
#include "cVideoDecoder.h"
#include "NvEncoderCuda.h"

#include "cvideo_writer.h"
#include "crockchip_encoder.h"
namespace mediakit {

    class cVideoEncoder 
    {
    public:
    //using Ptr = std::shared_ptr<cVideoEncoder>;
    public:
            cVideoEncoder() 
        : m_encoder (NULL)
#ifdef __GNUC__
        , m_param()
#endif 
            {}
            virtual ~cVideoEncoder();

    public:
        bool init(  int32_t width, int32_t   height , int32_t fps , const mediakit::CodecId& codec = mediakit::CodecH264 );
        bool init(const dsp::ctranscode_info& info);
        void destroy();
    public:
        void encode(
            const uint8_t *data, int32_t size, int64_t pts, std::vector<std::vector<uint8_t>> &vPacket,
            std::vector<uint64_t> &ptss /*AVFrame *frame*/ /*, std::shared_ptr<mediakit::FrameDispatcher> Interface*/);
    
#ifdef _MSC_VER
        void encode(const cv::Mat& frame, int64_t pts,  cv::cudacodec::EncoderCallback * callback);
        void encode(const cv::cuda::GpuMat& frame, int64_t pts, cv::cudacodec::EncoderCallback* callback);
#elif defined(__GNUC__)
        bool encode(MppFrame& frame, int64_t pts, dsp::EncoderCallback* callback);
#else

#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
    private:
      //  AVCodecContext *m_c ;
#ifdef _MSC_VER
        NvEncoderCuda *m_encoder;


          cv::cudacodec::Codec m_codec = cv::cudacodec::Codec::HEVC;
          
          cv::cudacodec::ColorFormat writerColorFormat = cv::cudacodec::ColorFormat::RGBA;//cv::cudacodec::ColorFormat::RGBA;
        std::shared_ptr <dsp::VideoWriterImpl> writer = nullptr;
        cv::cudacodec::EncoderParams encoder_params  ;
#elif defined(__GNUC__)
        dsp::crockchip_encoder* m_encoder;
        dsp::video_info_param   m_param;
        int32_t            m_codec = { MPP_VIDEO_CodingAVC };
#else
         
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif

        double m_fps = 25;
    };
} // namespace chen

#endif // _C_VIDEO_ENCODER_H_
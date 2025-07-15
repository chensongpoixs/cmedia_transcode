/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_ENCODER_H_
#define _C_VIDEO_ENCODER_H_
 
#include "Extension/frame.h"
//#include "Extension/Track.h"
#include "cVideoDecoder.h"
#include "NvEncoderCuda.h"
#include <opencv2/cudacodec.hpp>
namespace mediakit {

    class cVideoEncoder 
    {
    public:
    //using Ptr = std::shared_ptr<cVideoEncoder>;
    public:
            cVideoEncoder()
        : m_encoder (NULL){}
        virtual ~cVideoEncoder() {}

    public:
        bool init(  int32_t width, int32_t   height , int32_t fps , const mediakit::CodecId& codec = mediakit::CodecH264 );


        void encode(
            const uint8_t *data, int32_t size, int64_t pts, std::vector<std::vector<uint8_t>> &vPacket,
            std::vector<uint64_t> &ptss /*AVFrame *frame*/ /*, std::shared_ptr<mediakit::FrameDispatcher> Interface*/);
    
    
        void encode(const cv::Mat& frame,  cv::cudacodec::EncoderCallback * callback);
        void encode(const cv::cuda::GpuMat& frame, cv::cudacodec::EncoderCallback* callback);
    private:
      //  AVCodecContext *m_c ;

        NvEncoderCuda *m_encoder;


          cv::cudacodec::Codec m_codec = cv::cudacodec::Codec::HEVC;
          double m_fps = 25;
          cv::cudacodec::ColorFormat writerColorFormat = cv::cudacodec::ColorFormat::RGBA;//cv::cudacodec::ColorFormat::RGBA;
        std::shared_ptr <cv::cudacodec::VideoWriter> writer = nullptr;
        cv::cudacodec::EncoderParams encoder_params  ;
    };
} // namespace chen

#endif // _C_VIDEO_ENCODER_H_
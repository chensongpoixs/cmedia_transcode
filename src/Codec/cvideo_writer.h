/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_WRITER_H_
#define _C_VIDEO_WRITER_H_
#include <queue>
#include <vector>
#include <mutex>
#ifdef _MSC_VER
#include <nvcuvid.h>
#include <opencv2/cudacodec.hpp>
#include "NvEncoderCuda.h"
#include "NvEncoderCuda.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <fstream>
//#include "precomp.hpp"


//bool operator==(const cv::cudacodec::EncoderParams& lhs, const cv::cudacodec::EncoderParams& rhs)
//{
//    return std::tie(lhs.nvPreset, lhs.tuningInfo, lhs.encodingProfile, lhs.rateControlMode, lhs.multiPassEncoding, lhs.constQp.qpInterB, lhs.constQp.qpInterP, lhs.constQp.qpIntra,
//        lhs.averageBitRate, lhs.maxBitRate, lhs.targetQuality, lhs.gopLength) == std::tie(rhs.nvPreset, rhs.tuningInfo, rhs.encodingProfile, rhs.rateControlMode, rhs.multiPassEncoding, rhs.constQp.qpInterB, rhs.constQp.qpInterP, rhs.constQp.qpIntra,
//            rhs.averageBitRate, rhs.maxBitRate, rhs.targetQuality, rhs.gopLength);
//};
namespace  dsp
{
    NV_ENC_BUFFER_FORMAT EncBufferFormat(const cv::cudacodec::ColorFormat colorFormat);
    int NChannels(const cv::cudacodec:: ColorFormat colorFormat);
    GUID CodecGuid(const cv::cudacodec::Codec codec);
    void FrameRate(const double fps, uint32_t& frameRateNum, uint32_t& frameRateDen);
    GUID EncodingProfileGuid(const cv::cudacodec::EncodeProfile encodingProfile);
    GUID EncodingPresetGuid(const cv::cudacodec::EncodePreset nvPreset);

   


   /* class RawVideoWriter : public cv::cudacodec::EncoderCallback
    {
    public:
        RawVideoWriter(const std::string& fileName);
        ~RawVideoWriter();
        void onEncoded(const std::vector<std::vector<uint8_t>>& vPacket, const std::vector<uint64_t>& pts);
        void onEncodingFinished();
        bool setFrameIntervalP(const int) { return false; }
    private:
        std::ofstream fpOut;
    };*/


    class VideoWriterImpl : public cv::cudacodec::VideoWriter
    {
    public:
      /*  VideoWriterImpl(const std::shared_ptr<cv::cudacodec::EncoderCallback>& videoWriter, const cv::Size frameSize, const cv::cudacodec::Codec codec, const double fps,
            const cv::cudacodec::ColorFormat colorFormat, const cv::cuda::Stream& stream = cv::cuda::Stream::Null());
      */  VideoWriterImpl(  cv::cudacodec::EncoderCallback* videoWriter, const cv::Size frameSize, const cv::cudacodec::Codec codec, const double fps,
            const cv::cudacodec::ColorFormat colorFormat, const cv::cudacodec::EncoderParams& encoderParams, const cv::cuda::Stream& stream = cv::cuda::Stream::Null());
        ~VideoWriterImpl();
        void write(cv::InputArray frame);
        void write(cv::InputArray frame, int64_t pts);
        void write(const cv::cuda::GpuMat & frame, int64_t pts);
        cv::cudacodec::EncoderParams getEncoderParams() const;
        void release();
    private:
        void Init(const cv::cudacodec::Codec codec, const double fps, const cv::Size frameSz);
        void InitializeEncoder(const GUID codec, const double fps);
        void CopyToNvSurface(const  cv::InputArray src);
        void CopyToNvSurface(const cv::cuda::GpuMat& src);
           cv::cudacodec::EncoderCallback * encoderCallback = nullptr;
        cv::cudacodec::ColorFormat colorFormat = cv::cudacodec::ColorFormat::UNDEFINED;
        NV_ENC_BUFFER_FORMAT surfaceFormat = NV_ENC_BUFFER_FORMAT::NV_ENC_BUFFER_FORMAT_UNDEFINED;
        cv::cudacodec::EncoderParams encoderParams;
        cv::cuda::Stream stream = cv::cuda::Stream::Null();

        std::shared_ptr<NvEncoderCuda> pEnc;
        
        std::vector<std::vector<uint8_t>> vPacket;
        int nSrcChannels = 0;
        CUcontext cuContext;
    };

}
#endif // #ifdef _MSC_VER
#endif // _C_VIDEO_WRITER_H_
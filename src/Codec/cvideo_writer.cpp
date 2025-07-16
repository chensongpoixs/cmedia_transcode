/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/
#define NOMINMAX
#include "cvideo_writer.h"
#include <ostream>
#include <fstream>
#include <cassert>
#include <opencv2/cudaimgproc.hpp>
#include "Util/logger.h"
namespace dsp
{
    NV_ENC_BUFFER_FORMAT EncBufferFormat(const cv::cudacodec::ColorFormat colorFormat) {
        switch (colorFormat) {
        case cv::cudacodec::ColorFormat::BGR: return NV_ENC_BUFFER_FORMAT_ARGB;
        case cv::cudacodec::ColorFormat::RGB: return NV_ENC_BUFFER_FORMAT_ABGR;
        case cv::cudacodec::ColorFormat::BGRA: return NV_ENC_BUFFER_FORMAT_ARGB;
        case cv::cudacodec::ColorFormat::RGBA: return NV_ENC_BUFFER_FORMAT_ABGR;
        case cv::cudacodec::ColorFormat::GRAY:
        case cv::cudacodec::ColorFormat::NV_NV12: return NV_ENC_BUFFER_FORMAT_NV12;
        case cv::cudacodec::ColorFormat::NV_YV12: return NV_ENC_BUFFER_FORMAT_YV12;
        case cv::cudacodec::ColorFormat::NV_IYUV: return NV_ENC_BUFFER_FORMAT_IYUV;
        case cv::cudacodec::ColorFormat::NV_YUV444: return NV_ENC_BUFFER_FORMAT_YUV444;
        case cv::cudacodec::ColorFormat::NV_AYUV: return NV_ENC_BUFFER_FORMAT_AYUV;
        case cv::cudacodec::ColorFormat::NV_YUV420_10BIT: return NV_ENC_BUFFER_FORMAT_YUV420_10BIT;
        case cv::cudacodec::ColorFormat::NV_YUV444_10BIT: return NV_ENC_BUFFER_FORMAT_YUV444_10BIT;
        default: return NV_ENC_BUFFER_FORMAT_UNDEFINED;
        }
    }

    int NChannels(const cv::cudacodec::ColorFormat colorFormat) {
        switch (colorFormat) {
        case cv::cudacodec::ColorFormat::BGR:
        case cv::cudacodec::ColorFormat::RGB: return 3;
        case cv::cudacodec::ColorFormat::RGBA:
        case cv::cudacodec::ColorFormat::BGRA:
        case cv::cudacodec::ColorFormat::NV_AYUV: return 4;
        case cv::cudacodec::ColorFormat::GRAY:
        case cv::cudacodec::ColorFormat::NV_NV12:
        case cv::cudacodec::ColorFormat::NV_IYUV:
        case cv::cudacodec::ColorFormat::NV_YV12:
        case cv::cudacodec::ColorFormat::NV_YUV420_10BIT:
        case cv::cudacodec::ColorFormat::NV_YUV444:
        case cv::cudacodec::ColorFormat::NV_YUV444_10BIT: return 1;
        default: return 0;
        }
    }


    void FrameRate(const double fps, uint32_t& frameRateNum, uint32_t& frameRateDen) {
        CV_Assert(fps >= 0);
        int frame_rate = (int)(fps + 0.5);
        int frame_rate_base = 1;
        while (fabs(((double)frame_rate / frame_rate_base) - fps) > 0.001) {
            frame_rate_base *= 10;
            frame_rate = (int)(fps * frame_rate_base + 0.5);
        }
        frameRateNum = frame_rate;
        frameRateDen = frame_rate_base;
    }

    GUID EncodingProfileGuid(const cv::cudacodec::EncodeProfile encodingProfile) {
        switch (encodingProfile) {
        case(cv::cudacodec::ENC_CODEC_PROFILE_AUTOSELECT): return NV_ENC_CODEC_PROFILE_AUTOSELECT_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_BASELINE): return NV_ENC_H264_PROFILE_BASELINE_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_MAIN): return NV_ENC_H264_PROFILE_MAIN_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_HIGH): return NV_ENC_H264_PROFILE_HIGH_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_HIGH_444): return NV_ENC_H264_PROFILE_HIGH_444_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_STEREO): return NV_ENC_H264_PROFILE_STEREO_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_PROGRESSIVE_HIGH): return NV_ENC_H264_PROFILE_PROGRESSIVE_HIGH_GUID;
        case(cv::cudacodec::ENC_H264_PROFILE_CONSTRAINED_HIGH): return NV_ENC_H264_PROFILE_CONSTRAINED_HIGH_GUID;
        case(cv::cudacodec::ENC_HEVC_PROFILE_MAIN): return NV_ENC_HEVC_PROFILE_MAIN_GUID;
        case(cv::cudacodec::ENC_HEVC_PROFILE_MAIN10): return NV_ENC_HEVC_PROFILE_MAIN10_GUID;
        case(cv::cudacodec::ENC_HEVC_PROFILE_FREXT): return NV_ENC_HEVC_PROFILE_FREXT_GUID;
        default: break;
        }
        std::string msg = "Unknown Encoding Profile.";
        printf("[%s][%d]%s\n", __FUNCTION__, __LINE__, msg.c_str());
       // CV_LOG_WARNING(NULL, msg);
       // CV_Error(Error::StsUnsupportedFormat, msg);
    }

    GUID EncodingPresetGuid(const cv::cudacodec::EncodePreset nvPreset) {
        switch (nvPreset) {
        case cv::cudacodec::ENC_PRESET_P1: return NV_ENC_PRESET_P1_GUID;
        case cv::cudacodec::ENC_PRESET_P2: return NV_ENC_PRESET_P2_GUID;
        case cv::cudacodec::ENC_PRESET_P3: return NV_ENC_PRESET_P3_GUID;
        case cv::cudacodec::ENC_PRESET_P4: return NV_ENC_PRESET_P4_GUID;
        case cv::cudacodec::ENC_PRESET_P5: return NV_ENC_PRESET_P5_GUID;
        case cv::cudacodec::ENC_PRESET_P6: return NV_ENC_PRESET_P6_GUID;
        case cv::cudacodec::ENC_PRESET_P7: return NV_ENC_PRESET_P7_GUID;
        default: break;
        }
        std::string msg = "Unknown Nvidia Encoding Preset.";
        printf("[%s][%d]%s\n", __FUNCTION__, __LINE__, msg.c_str());
       // CV_LOG_WARNING(NULL, msg);
        //CV_Error(Error::StsUnsupportedFormat, msg);
    }

    std::string GetVideoCodecString(const GUID codec) {
        if (codec == NV_ENC_CODEC_H264_GUID)
        {
            return "AVC/H.264";
        }
        else if (codec == NV_ENC_CODEC_HEVC_GUID)
        {
            return "H.265/HEVC";
        }
        else if (codec == NV_ENC_CODEC_AV1_GUID)
        {
            return "AV1";
        }
        return "Unknown";
    }
    GUID CodecGuid(const cv::cudacodec::Codec codec) {
        switch (codec) {
        case cv::cudacodec::Codec::H264: return NV_ENC_CODEC_H264_GUID;
        case cv::cudacodec::Codec::HEVC: return NV_ENC_CODEC_HEVC_GUID;
        default: break;
        }
        std::string msg = "Unknown codec: cudacodec::VideoWriter only supports CODEC_VW::H264 and CODEC_VW::HEVC";
        printf("[%s][%d]%s\n", __FUNCTION__, __LINE__, msg.c_str());
       // CV_LOG_WARNING(NULL, msg);
        //CV_Error(Error::StsUnsupportedFormat, msg);
    }




    //RawVideoWriter::RawVideoWriter(const std::string& fileName) {
    //    fpOut = std::ofstream(fileName, std::ios::out | std::ios::binary);
    //    if (!fpOut)
    //    {
    //        printf("[%s][%d]Failed to open video file = %s  for writing!!!\n", __FUNCTION__, __LINE__, fileName.c_str());
    //    }
    //        //CV_Error(Error::StsError, "Failed to open video file " + fileName + " for writing!");
    //}

    //void RawVideoWriter::onEncodingFinished() {
    //    fpOut.close();
    //}

    //RawVideoWriter::~RawVideoWriter() {
    //    onEncodingFinished();
    //}

    //void RawVideoWriter::onEncoded(const std::vector<std::vector<uint8_t>>& vPacket, const std::vector<uint64_t>&) {
    //    for (auto& packet : vPacket)
    //        fpOut.write(reinterpret_cast<const char*>(packet.data()), packet.size());
    //}



    VideoWriterImpl::VideoWriterImpl(  cv::cudacodec::EncoderCallback* encoderCallBack_, const cv::Size frameSz, const cv::cudacodec::Codec codec, const double fps,
        const cv::cudacodec::ColorFormat colorFormat_, const cv::cudacodec::EncoderParams& encoderParams_, const cv::cuda::Stream& stream_) :
        encoderCallback(encoderCallBack_), colorFormat(colorFormat_), encoderParams(encoderParams_), stream(stream_)
    {
        CV_Assert(colorFormat != cv::cudacodec::ColorFormat::UNDEFINED);
        surfaceFormat = EncBufferFormat(colorFormat);
        if (surfaceFormat == NV_ENC_BUFFER_FORMAT_UNDEFINED) {
            std::string msg = cv::format("Unsupported input surface format: %i", colorFormat);
            printf("[%s][%d]%s\n", __FUNCTION__, __LINE__, msg.c_str());
           /* CV_LOG_WARNING(NULL, msg);
            CV_Error(Error::StsUnsupportedFormat, msg);*/
        }
        nSrcChannels = NChannels(colorFormat);
        Init(codec, fps, frameSz);
    }

    void VideoWriterImpl::release() {
        std::vector<uint64_t> pts;
        pEnc->EndEncode(vPacket, pts);
        encoderCallback->onEncoded(vPacket, pts);
        encoderCallback->onEncodingFinished();
    }

    VideoWriterImpl::~VideoWriterImpl() 
    {
        InfoL << "---->";
        release();
    }

    void VideoWriterImpl::Init(const cv::cudacodec::Codec codec, const double fps, const cv::Size frameSz) {
        // init context
        cv::cuda::GpuMat temp(1, 1, CV_8UC1);
        temp.release();
        cudaSafeCall(cuCtxGetCurrent(&cuContext));
        CV_Assert(nSrcChannels != 0);
        const GUID codecGuid = CodecGuid(codec);
        try {
            pEnc.reset(new NvEncoderCuda(cuContext, frameSz.width, frameSz.height, surfaceFormat));
            InitializeEncoder(codecGuid, fps);
            const cudaStream_t cudaStream = 0;// cuda::StreamAccessor::getStream(stream);
            pEnc->SetIOCudaStreams((NV_ENC_CUSTREAM_PTR)&cudaStream, (NV_ENC_CUSTREAM_PTR)&cudaStream);
        }
        catch (cv::Exception& e)
        {
            std::string msg = std::string("Error initializing Nvidia Encoder. Refer to Nvidia's GPU Support Matrix to confirm your GPU supports hardware encoding, ") +
                std::string("codec and surface format and check the encoder documentation to verify your choice of encoding paramaters are supported.") +
                e.msg;
            printf("[%s][%d]%s\n", __FUNCTION__, __LINE__, msg.c_str());
            //CV_Error(Error::GpuApiCallError, msg);
        }
        const cv::Size encoderFrameSz(pEnc->GetEncodeWidth(), pEnc->GetEncodeHeight());
        CV_Assert(frameSz == encoderFrameSz);
    }
    void VideoWriterImpl::InitializeEncoder(const GUID codec, const double fps)
    {
        NV_ENC_INITIALIZE_PARAMS initializeParams = {};
        initializeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
        NV_ENC_CONFIG encodeConfig = {};
        encodeConfig.version = NV_ENC_CONFIG_VER;
        initializeParams.encodeConfig = &encodeConfig;
        pEnc->CreateDefaultEncoderParams(&initializeParams, codec, EncodingPresetGuid(encoderParams.nvPreset), (NV_ENC_TUNING_INFO)encoderParams.tuningInfo);
        FrameRate(fps, initializeParams.frameRateNum, initializeParams.frameRateDen);
        initializeParams.encodeConfig->profileGUID = EncodingProfileGuid(encoderParams.encodingProfile);
        initializeParams.encodeConfig->rcParams.rateControlMode = (NV_ENC_PARAMS_RC_MODE)(encoderParams.rateControlMode + encoderParams.multiPassEncoding);
        initializeParams.encodeConfig->rcParams.constQP = { encoderParams.constQp.qpInterB, encoderParams.constQp.qpInterB,encoderParams.constQp.qpInterB };
        initializeParams.encodeConfig->rcParams.averageBitRate = encoderParams.averageBitRate;
        initializeParams.encodeConfig->rcParams.maxBitRate = encoderParams.maxBitRate;
        initializeParams.encodeConfig->rcParams.targetQuality = encoderParams.targetQuality;
        initializeParams.encodeConfig->gopLength = encoderParams.gopLength;
        if (initializeParams.encodeConfig->frameIntervalP > 1) {
            CV_Assert(encoderCallback->setFrameIntervalP(initializeParams.encodeConfig->frameIntervalP));
        }
        if (codec == NV_ENC_CODEC_H264_GUID) {
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.idrPeriod = encoderParams.idrPeriod;
            if (encoderParams.videoFullRangeFlag) {
                initializeParams.encodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoFullRangeFlag = 1;
                initializeParams.encodeConfig->encodeCodecConfig.h264Config.h264VUIParameters.videoSignalTypePresentFlag = 1;
            }
        }
        else if (codec == NV_ENC_CODEC_HEVC_GUID) {
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.idrPeriod = encoderParams.idrPeriod;
            if (encoderParams.videoFullRangeFlag) {
                initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.videoFullRangeFlag = 1;
                initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.hevcVUIParameters.videoSignalTypePresentFlag = 1;
            }
        }
        else {
            std::string msg = "videoFullRangeFlag is not supported by codec: " + GetVideoCodecString(codec);
            printf("[%s][%d]%s\n", __FUNCTION__, __LINE__, msg.c_str());
            //CV_LOG_WARNING(NULL, msg);
        }
        pEnc->CreateEncoder(&initializeParams);
    }

    inline bool CvFormat(const cv::cudacodec::ColorFormat cf) {
        if (cf == cv::cudacodec::ColorFormat::BGR || cf == cv::cudacodec::ColorFormat::RGB
            || cf == cv::cudacodec::ColorFormat::BGRA || cf == cv::cudacodec::ColorFormat::RGBA || cf == cv::cudacodec::ColorFormat::GRAY)
        {
            return true;
        }
        return false;
    }
    void VideoWriterImpl::CopyToNvSurface(const cv::InputArray src)
    {
        const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
        CV_Assert(src.isGpuMat() || src.isMat());
        if (CvFormat(colorFormat))
        {
            assert(src.size() == cv::Size(pEnc->GetEncodeWidth(), pEnc->GetEncodeHeight()));
        }
        unsigned char* /*Npp8u*/  dst = (unsigned char*/*Npp8u**/)encoderInputFrame->inputPtr;
        if (colorFormat == cv::cudacodec::ColorFormat::BGR || colorFormat == cv::cudacodec::ColorFormat::RGB) {
            cv::cuda::GpuMat srcDevice;
            if (src.isGpuMat())
                srcDevice = src.getGpuMat();
            else {
                if (stream)
                    srcDevice.upload(src, stream);
                else
                    srcDevice.upload(src);
            }
            if (colorFormat == cv::cudacodec::ColorFormat::BGR) {
                cv::cuda::GpuMat dstGpuMat(pEnc->GetEncodeHeight(), pEnc->GetEncodeWidth(), CV_8UC4, dst, encoderInputFrame->pitch);
                cv::cuda::cvtColor(srcDevice, dstGpuMat, cv::COLOR_BGR2BGRA, 0, stream);
            }
            else {
                cv::cuda::GpuMat dstGpuMat(pEnc->GetEncodeHeight(), pEnc->GetEncodeWidth(), CV_8UC4, dst, encoderInputFrame->pitch);
                cv::cuda::cvtColor(srcDevice, dstGpuMat, cv::COLOR_RGB2RGBA, 0, stream);
            }
        }
        else if (colorFormat == cv::cudacodec::ColorFormat::GRAY) {
            const cudaMemcpyKind memcpyKind = src.isGpuMat() ? cudaMemcpyDeviceToDevice : cudaMemcpyHostToDevice;
            const void* srcPtr = src.isGpuMat() ? src.getGpuMat().data : src.getMat().data;
            const size_t srcPitch = src.isGpuMat() ? src.getGpuMat().step : src.getMat().step;
            const uint32_t chromaHeight = NvEncoder::GetChromaHeight(NV_ENC_BUFFER_FORMAT_NV12, pEnc->GetEncodeHeight());
            if (stream) {
                cudaMemcpy2DAsync(dst, encoderInputFrame->pitch, srcPtr, srcPitch, pEnc->GetEncodeWidth(), pEnc->GetEncodeHeight(), memcpyKind,
                    0/*cuda::StreamAccessor::getStream(stream)*/);
                cudaMemset2DAsync(&dst[encoderInputFrame->pitch * pEnc->GetEncodeHeight()], encoderInputFrame->pitch, 128, pEnc->GetEncodeWidth(), chromaHeight,
                    0/*cuda::StreamAccessor::getStream(stream)*/);
            }
            else 
            {
                cudaMemcpy2D(dst, encoderInputFrame->pitch, srcPtr, srcPitch, pEnc->GetEncodeWidth(), pEnc->GetEncodeHeight(), memcpyKind);
                cudaMemset2D(&dst[encoderInputFrame->pitch * pEnc->GetEncodeHeight()], encoderInputFrame->pitch, 128, pEnc->GetEncodeWidth(), chromaHeight);
            }
        }
        else {
            void* srcPtr = src.isGpuMat() ? src.getGpuMat().data : src.getMat().data;
            const CUmemorytype cuMemoryType = src.isGpuMat() ? CU_MEMORYTYPE_DEVICE : CU_MEMORYTYPE_HOST;
            NvEncoderCuda::CopyToDeviceFrame(cuContext, srcPtr, static_cast<unsigned>(src.step()), (CUdeviceptr)encoderInputFrame->inputPtr, (int)encoderInputFrame->pitch, pEnc->GetEncodeWidth(),
                pEnc->GetEncodeHeight(), cuMemoryType, encoderInputFrame->bufferFormat, encoderInputFrame->chromaOffsets, encoderInputFrame->numChromaPlanes,
                false, 0/*cuda::StreamAccessor::getStream(stream)*/);
        }
    }
    void VideoWriterImpl::CopyToNvSurface(const cv::cuda::GpuMat& src)
    {
        const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
       /* CV_Assert(src.isGpuMat() || src.isMat());
        if (CvFormat(colorFormat))
        {
            assert(src.size() == cv::Size(pEnc->GetEncodeWidth(), pEnc->GetEncodeHeight()));
        }*/
        unsigned char* /*Npp8u*/  dst = (unsigned char*/*Npp8u**/)encoderInputFrame->inputPtr;
        void* srcPtr = src.data;
        const CUmemorytype cuMemoryType =   CU_MEMORYTYPE_DEVICE  ;
        NvEncoderCuda::CopyToDeviceFrame(cuContext, srcPtr, static_cast<unsigned>(src.step), (CUdeviceptr)encoderInputFrame->inputPtr, (int)encoderInputFrame->pitch, pEnc->GetEncodeWidth(),
            pEnc->GetEncodeHeight(), cuMemoryType, encoderInputFrame->bufferFormat, encoderInputFrame->chromaOffsets, encoderInputFrame->numChromaPlanes,
            false, 0/*cuda::StreamAccessor::getStream(stream)*/);
    }
    void VideoWriterImpl::write(const cv::InputArray frame ) 
    {
        CV_Assert(frame.channels() == nSrcChannels);
        CopyToNvSurface(frame);
        std::vector<uint64_t> pts;
      
        pEnc->EncodeFrame(vPacket, pts);
        encoderCallback->onEncoded(vPacket, pts);
    };
    void VideoWriterImpl::write(const cv::InputArray frame, int64_t frame_pts) {
        CV_Assert(frame.channels() == nSrcChannels);
        CopyToNvSurface(frame);
       // InfoL << "";
        std::vector<uint64_t> pts;
        NV_ENC_PIC_PARAMS input_params;
        input_params.inputTimeStamp = frame_pts;
        input_params.frameIdx = frame_pts;
       // InfoL << "";
        pEnc->EncodeFrame(vPacket, pts, &input_params);
      //  InfoL << "";
        encoderCallback->onEncoded(vPacket, pts);
       // InfoL << "";
    }
    void VideoWriterImpl::write(const cv::cuda::GpuMat& frame, int64_t frame_pts)
    {
       // CV_Assert(frame.channels() == nSrcChannels);
        CopyToNvSurface(frame);
        // InfoL << "";
        std::vector<uint64_t> pts;
        NV_ENC_PIC_PARAMS input_params;
        input_params.inputTimeStamp = frame_pts;
        input_params.frameIdx = frame_pts;
        // InfoL << "";
        pEnc->EncodeFrame(vPacket, pts, &input_params);
        //  InfoL << "";
        encoderCallback->onEncoded(vPacket, pts);
    }
    

    cv::cudacodec::EncoderParams VideoWriterImpl::getEncoderParams() const {
        return encoderParams;
    };


}
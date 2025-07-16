/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_PARSER_H_
#define _C_VIDEO_PARSER_H_
#include "cvideo_decoder.h"
#include <cuda_runtime_api.h>
#include "ccuda_define.h"
#include <cuda_runtime.h>
#include "Util/logger.h"
namespace  dsp
{
   
   

	  cvideo_decoder::cvideo_decoder(const cv::cudacodec::Codec& codec, const int32_t  minNumDecodeSufaces,
		cv::Size targetsSz, cv::Rect  srcRoi, cv::Rect targetRoi,
		const bool enableHistogram, CUcontext ctx, CUvideoctxlock lock)
	{
		m_cuda_ctx = ctx;
		m_cuda_lock = lock;
		m_video_format.codec = codec;
		m_video_format.ulNumDecodeSurfaces = minNumDecodeSufaces;
		m_video_format.enableHistogram = enableHistogram;
		// alignment enforced by nvcuvid, likely due to chroma subsampling
		m_video_format.targetSz.width = targetsSz.width - targetsSz.width % 2; m_video_format.targetSz.height = targetsSz.height - targetsSz.height % 2;
		m_video_format.srcRoi.x = srcRoi.x - srcRoi.x % 4; m_video_format.srcRoi.width = srcRoi.width - srcRoi.width % 4;
		m_video_format.srcRoi.y = srcRoi.y - srcRoi.y % 2; m_video_format.srcRoi.height = srcRoi.height - srcRoi.height % 2;
		m_video_format.targetRoi.x = targetRoi.x - targetRoi.x % 4; m_video_format.targetRoi.width = targetRoi.width - targetRoi.width % 4;
		m_video_format.targetRoi.y = targetRoi.y - targetRoi.y % 2; m_video_format.targetRoi.height = targetRoi.height - targetRoi.height % 2;
		//return true;
	}
      cvideo_decoder::~cvideo_decoder()
      {
          InfoL << "---> ";
          destroy();
      }
      void cvideo_decoder::destroy()
      {
          InfoL << "---> ";
          release();
      }
	cv::cuda::GpuMat cvideo_decoder::mapFrame(int picIdx, CUVIDPROCPARAMS& videoProcParams)
	{
		CUdeviceptr ptr;
		unsigned int pitch;

		CUresult result = /*cudaSafeCall*/(cuvidMapVideoFrame(m_cuda_decoder, picIdx, &ptr, &pitch, &videoProcParams));
		if (result != CUDA_SUCCESS)
		{
			printf("[%s][%s][%d] cuvidMapVideoFrame failed !!!  [result = %u]\n", __FILE__, __FUNCTION__, __LINE__, result);
			return cv::cuda::GpuMat();
		}
		const int height = (m_video_format.surfaceFormat == cudaVideoSurfaceFormat_NV12 || m_video_format.surfaceFormat == cudaVideoSurfaceFormat_P016) ? targetHeight() * 3 / 2 : targetHeight() * 3;
		const int type = (m_video_format.surfaceFormat == cudaVideoSurfaceFormat_NV12 || m_video_format.surfaceFormat == cudaVideoSurfaceFormat_YUV444) ? CV_8U : CV_16U;
      //  return cv::cuda::GpuMat(height, targetWidth(), type, (void*)ptr);
        return cv::cuda::GpuMat(height, targetWidth(), type, (void*)ptr, pitch);
	}

	void cvideo_decoder::unmapFrame(cv::cuda::GpuMat& frame)
	{
		CUresult result = cuvidUnmapVideoFrame(m_cuda_decoder, (CUdeviceptr)frame.data);
		if (result != CUDA_SUCCESS)
		{
			printf("[%s][%s][%d] cuvidUnmapVideoFrame failed !!!  [result = %u]\n", __FILE__, __FUNCTION__, __LINE__, result);
		}
	}

 
	// parser data call 
	bool cvideo_decoder::create(const cv::cudacodec::FormatInfo& video_format)
	{
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_video_format = video_format;
        }
        const cudaVideoCodec _codec = static_cast<cudaVideoCodec>(m_video_format.codec);
        const cudaVideoChromaFormat _chromaFormat = static_cast<cudaVideoChromaFormat>(m_video_format.chromaFormat);

        cudaVideoSurfaceFormat surfaceFormat = cudaVideoSurfaceFormat_NV12;
#if (CUDART_VERSION < 9000)
        if (videoFormat.nBitDepthMinus8 > 0) {
            std::ostringstream warning;
            warning << "NV12 (8 bit luma, 4 bit chroma) is currently the only supported decoder output format. Video input is " << videoFormat.nBitDepthMinus8 + 8 << " bit " \
                << std::string(GetVideoChromaFormatString(_chromaFormat)) << ".  Truncating luma to 8 bits";
            if (videoFormat.chromaFormat != YUV420)
                warning << " and chroma to 4 bits";
            CV_LOG_WARNING("%s\n", warning.str());
        }
#else
        if (_chromaFormat == cudaVideoChromaFormat_420 || cudaVideoChromaFormat_Monochrome)
        {
            surfaceFormat = m_video_format.nBitDepthMinus8 ? cudaVideoSurfaceFormat_P016 : cudaVideoSurfaceFormat_NV12;

        }
        else if (_chromaFormat == cudaVideoChromaFormat_444)
        {
            surfaceFormat = m_video_format.nBitDepthMinus8 ? cudaVideoSurfaceFormat_YUV444_16Bit : cudaVideoSurfaceFormat_YUV444;

        }
        else if (_chromaFormat == cudaVideoChromaFormat_422) {
            surfaceFormat = m_video_format.nBitDepthMinus8 ? cudaVideoSurfaceFormat_P016 : cudaVideoSurfaceFormat_NV12;
            printf("%s\n", "YUV 4:2:2 is not currently supported, falling back to YUV 4:2:0.");
        }
#endif

        const cudaVideoCreateFlags videoCreateFlags = (_codec == cudaVideoCodec_JPEG || _codec == cudaVideoCodec_MPEG2) ?
            cudaVideoCreate_PreferCUDA :
            cudaVideoCreate_PreferCUVID;

        // Validate video format.  These are the currently supported formats via NVCUVID
        bool codecSupported = cudaVideoCodec_MPEG1 == _codec ||
            cudaVideoCodec_MPEG2 == _codec ||
            cudaVideoCodec_MPEG4 == _codec ||
            cudaVideoCodec_VC1 == _codec ||
            cudaVideoCodec_H264 == _codec ||
            cudaVideoCodec_JPEG == _codec ||
            cudaVideoCodec_H264_SVC == _codec ||
            cudaVideoCodec_H264_MVC == _codec ||
            cudaVideoCodec_YV12 == _codec ||
            cudaVideoCodec_NV12 == _codec ||
            cudaVideoCodec_YUYV == _codec ||
            cudaVideoCodec_UYVY == _codec;

#if (CUDART_VERSION >= 6050)
        codecSupported |= cudaVideoCodec_HEVC == _codec;
#endif
#if (CUDART_VERSION >= 7050)
        codecSupported |= cudaVideoCodec_YUV420 == _codec;
#endif
#if  ((CUDART_VERSION == 7050) || (CUDART_VERSION >= 9000))
        codecSupported |= cudaVideoCodec_VP8 == _codec || cudaVideoCodec_VP9 == _codec;
#endif
#if (CUDART_VERSION >= 9000)
        codecSupported |= cudaVideoCodec_AV1;
#endif
        assert(codecSupported);
        assert(cudaVideoChromaFormat_Monochrome == _chromaFormat ||
            cudaVideoChromaFormat_420 == _chromaFormat ||
            cudaVideoChromaFormat_422 == _chromaFormat ||
            cudaVideoChromaFormat_444 == _chromaFormat);

#if (CUDART_VERSION >= 9000)
        // Check video format is supported by GPU's hardware video decoder
        CUVIDDECODECAPS decodeCaps = {};
        decodeCaps.eCodecType = _codec;
        decodeCaps.eChromaFormat = _chromaFormat;
        decodeCaps.nBitDepthMinus8 = m_video_format.nBitDepthMinus8;
        cudaSafeCall(cuCtxPushCurrent(m_cuda_ctx));
        cudaSafeCall(cuvidGetDecoderCaps(&decodeCaps));
        cudaSafeCall(cuCtxPopCurrent(NULL));

        if (!decodeCaps.bIsSupported) {
            printf("%s", std::to_string(decodeCaps.nBitDepthMinus8 + 8) + " bit " + GetVideoCodecString(_codec) + " with " + GetVideoChromaFormatString(_chromaFormat) + " chroma format is not supported by this GPU hardware video decoder.  Please refer to Nvidia's GPU Support Matrix to confirm your GPU supports hardware decoding of this video source.");
        }

        if (!(decodeCaps.nOutputFormatMask & (1 << surfaceFormat)))
        {
            if (decodeCaps.nOutputFormatMask & (1 << cudaVideoSurfaceFormat_NV12))
                surfaceFormat = cudaVideoSurfaceFormat_NV12;
            else if (decodeCaps.nOutputFormatMask & (1 << cudaVideoSurfaceFormat_P016))
                surfaceFormat = cudaVideoSurfaceFormat_P016;
            else if (decodeCaps.nOutputFormatMask & (1 << cudaVideoSurfaceFormat_YUV444))
                surfaceFormat = cudaVideoSurfaceFormat_YUV444;
            else if (decodeCaps.nOutputFormatMask & (1 << cudaVideoSurfaceFormat_YUV444_16Bit))
                surfaceFormat = cudaVideoSurfaceFormat_YUV444_16Bit;
            else
                printf("%s", "No supported output format found");
        }
        m_video_format.surfaceFormat = static_cast<cv::cudacodec::SurfaceFormat>(surfaceFormat);

        if (m_video_format.enableHistogram) {
            if (!decodeCaps.bIsHistogramSupported) {
                // assert(Error::StsBadArg, "Luma histogram output is not supported for current codec and/or on current device.");
            }

            if (decodeCaps.nCounterBitDepth != 32) {
                std::ostringstream error;
                error << "Luma histogram output disabled due to current device using " << decodeCaps.nCounterBitDepth << " bit bins. Histogram output only supports 32 bit bins.";
                //assert(Error::StsBadArg, error.str());
            }
            else {
                m_video_format.nCounterBitDepth = decodeCaps.nCounterBitDepth;
                m_video_format.nMaxHistogramBins = decodeCaps.nMaxHistogramBins;
            }
        }

        assert(m_video_format.ulWidth >= decodeCaps.nMinWidth &&
            m_video_format.ulHeight >= decodeCaps.nMinHeight &&
            m_video_format.ulWidth <= decodeCaps.nMaxWidth &&
            m_video_format.ulHeight <= decodeCaps.nMaxHeight);

        assert((m_video_format.width >> 4) * (m_video_format.height >> 4) <= decodeCaps.nMaxMBCount);
#else
        if (m_video_format.enableHistogram) {
            //  CV_Error(Error::StsBadArg, "Luma histogram output is not supported when CUDA Toolkit version <= 9.0.");
        }
#endif

        // Create video decoder
        CUVIDDECODECREATEINFO createInfo_ = {};
#if (CUDART_VERSION >= 9000)
        createInfo_.enableHistogram = m_video_format.enableHistogram;
        createInfo_.bitDepthMinus8 = m_video_format.nBitDepthMinus8;
        createInfo_.ulMaxWidth = m_video_format.ulMaxWidth;
        createInfo_.ulMaxHeight = m_video_format.ulMaxHeight;
#endif
        createInfo_.CodecType = _codec;
        createInfo_.ulWidth = m_video_format.ulWidth;
        createInfo_.ulHeight = m_video_format.ulHeight;
        createInfo_.ulNumDecodeSurfaces = m_video_format.ulNumDecodeSurfaces;
        createInfo_.ChromaFormat = _chromaFormat;
        createInfo_.OutputFormat = surfaceFormat;
        createInfo_.DeinterlaceMode = static_cast<cudaVideoDeinterlaceMode>(m_video_format.deinterlaceMode);
        createInfo_.ulTargetWidth = m_video_format.width;
        createInfo_.ulTargetHeight = m_video_format.height;
        createInfo_.display_area.left = m_video_format.displayArea.x;
        createInfo_.display_area.right = m_video_format.displayArea.x + m_video_format.displayArea.width;
        createInfo_.display_area.top = m_video_format.displayArea.y;
        createInfo_.display_area.bottom = m_video_format.displayArea.y + m_video_format.displayArea.height;
        createInfo_.target_rect.left = m_video_format.targetRoi.x;
        createInfo_.target_rect.right = m_video_format.targetRoi.x + m_video_format.targetRoi.width;
        createInfo_.target_rect.top = m_video_format.targetRoi.y;
        createInfo_.target_rect.bottom = m_video_format.targetRoi.y + m_video_format.targetRoi.height;
        createInfo_.ulNumOutputSurfaces = 2;
        createInfo_.ulCreationFlags = videoCreateFlags;
        createInfo_.vidLock = m_cuda_lock;
        cudaSafeCall(cuCtxPushCurrent(m_cuda_ctx));
        {
            // AutoLock autoLock(mtx_);
            std::lock_guard<std::mutex> lock(m_lock);
            cudaSafeCall(cuvidCreateDecoder(&m_cuda_decoder, &createInfo_));
        }
       


        /////////////////////////////////////////////

        if (m_video_format.colorSpaceStandard == cv::cudacodec::ColorSpaceStandard::Unspecified) {
            if (m_video_format.width > 1280 || m_video_format.height > 720)
            {
                m_video_format.colorSpaceStandard = cv::cudacodec::ColorSpaceStandard::BT709;
            }
            else
            {
                m_video_format.colorSpaceStandard = cv::cudacodec::ColorSpaceStandard::BT601;
            }
        }
        // createNVSurfaceToColorConverter
        m_yuvConverter = createNVSurfaceToColorConverter(m_video_format.colorSpaceStandard, m_video_format.videoFullRangeFlag);
        cudaSafeCall(cuCtxPopCurrent(NULL));
        return true;
	}

    void cvideo_decoder::cvtFromYuv(const cv::cuda::GpuMat& decodedFrame, cv::cuda::GpuMat& outFrame, const cv::cudacodec::SurfaceFormat surfaceFormat, const bool videoFullRangeFlag, cv::cuda::Stream& stream)
    {
        //if (colorFormat == ColorFormat::NV_YUV_SURFACE_FORMAT) {
        //    // decodedFrame.copyTo(outFrame, stream);
        //    return;
        //}
        if (!m_yuvConverter)
        {
            printf("[%s][%d]m_yuvConverter == nullptr \n", __FUNCTION__, __LINE__);
            
            return;
        }
        m_yuvConverter->convert(decodedFrame, outFrame, surfaceFormat, colorFormat, bitDepth, planar, videoFullRangeFlag, stream);
    }
	int32_t cvideo_decoder::reconfigure(const cv::cudacodec::FormatInfo& videoFormat)
	{
        if (videoFormat.nBitDepthMinus8 != m_video_format.nBitDepthMinus8 || videoFormat.nBitDepthChromaMinus8 != m_video_format.nBitDepthChromaMinus8) {
            printf("%s\n", "Reconfigure Not supported for bit depth change");
        }

        if (videoFormat.chromaFormat != m_video_format.chromaFormat) {
            printf("%s\n", "Reconfigure Not supported for chroma format change");
        }

        const bool decodeResChange = !(videoFormat.ulWidth == m_video_format.ulWidth && videoFormat.ulHeight == m_video_format.ulHeight);

        if ((videoFormat.ulWidth > m_video_format.ulMaxWidth) || (videoFormat.ulHeight > m_video_format.ulMaxHeight)) {
            // For VP9, let driver  handle the change if new width/height > maxwidth/maxheight
            if (videoFormat.codec != cv::cudacodec::Codec::VP9) {
                printf("%s\n", "Reconfigure Not supported when width/height > maxwidth/maxheight");
            }
        }

        if (!decodeResChange)
            return 1;

        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_video_format.ulNumDecodeSurfaces = videoFormat.ulNumDecodeSurfaces;
            m_video_format.ulWidth = videoFormat.ulWidth;
            m_video_format.ulHeight = videoFormat.ulHeight;
            m_video_format.targetRoi = videoFormat.targetRoi;
        }

        CUVIDRECONFIGUREDECODERINFO reconfigParams = { 0 };
        reconfigParams.ulWidth = m_video_format.ulWidth;
        reconfigParams.ulHeight = m_video_format.ulHeight;
        reconfigParams.display_area.left = m_video_format.displayArea.x;
        reconfigParams.display_area.right = m_video_format.displayArea.x + m_video_format.displayArea.width;
        reconfigParams.display_area.top = m_video_format.displayArea.y;
        reconfigParams.display_area.bottom = m_video_format.displayArea.y + m_video_format.displayArea.height;
        reconfigParams.ulTargetWidth = m_video_format.width;
        reconfigParams.ulTargetHeight = m_video_format.height;
        reconfigParams.target_rect.left = m_video_format.targetRoi.x;
        reconfigParams.target_rect.right = m_video_format.targetRoi.x + m_video_format.targetRoi.width;
        reconfigParams.target_rect.top = m_video_format.targetRoi.y;
        reconfigParams.target_rect.bottom = m_video_format.targetRoi.y + m_video_format.targetRoi.height;
        reconfigParams.ulNumDecodeSurfaces = m_video_format.ulNumDecodeSurfaces;

        cudaSafeCall(cuCtxPushCurrent(m_cuda_ctx));
        cudaSafeCall(cuvidReconfigureDecoder(m_cuda_decoder, &reconfigParams));
        cudaSafeCall(cuCtxPopCurrent(NULL));
        printf("%s\n", "Reconfiguring Decoder");
        return m_video_format.ulNumDecodeSurfaces;
	}
	void cvideo_decoder::release()
	{
        if (m_cuda_decoder)
        {
            cuvidDestroyDecoder(m_cuda_decoder);
            m_cuda_decoder = nullptr;
        }
	}
	bool  cvideo_decoder::inited()
	{
        std::lock_guard<std::mutex> lock(m_lock);
        return m_cuda_decoder;
	}
}



#endif // _C_VIDEO_PARSER_H_
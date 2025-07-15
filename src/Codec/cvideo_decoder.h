/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_DECODER_NV_H_
#define _C_VIDEO_DECODER_NV_H_
#include <mutex>
#include <opencv2/core.hpp>
#include <nvcuvid.h>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/cudacodec.hpp>
namespace  dsp
{
	class cvideo_decoder
	{
	public:
		cvideo_decoder(const cv::cudacodec::Codec& codec, const int32_t  minNumDecodeSufaces,
			cv::Size targetsSz, cv::Rect  srcRoi, cv::Rect targetRoi,
			const bool enableHistogram, CUcontext ctx, CUvideoctxlock lock);;
	/*	: m_cuda_ctx(NULL)
		, m_cuda_decoder(NULL)
			, m_video_format({}) {}*/
		virtual ~cvideo_decoder() {}

	public:
		/*bool init(const cv::cudacodec::Codec & codec, const int32_t  minNumDecodeSufaces,
			cv::Size targetsSz, cv::Rect  srcRoi, cv::Rect targetRoi, 
			const bool enableHistogram, CUcontext ctx, CUvideoctxlock lock);*/



        cudaVideoCodec codec() const { return static_cast<cudaVideoCodec>(m_video_format.codec); }
        int nDecodeSurfaces() const { return m_video_format.ulNumDecodeSurfaces; }
        cv::Size getTargetSz() const { return m_video_format.targetSz; }
        cv::Rect getSrcRoi() const { return m_video_format.srcRoi; }
        cv::Rect getTargetRoi() const { return m_video_format.targetRoi; }

        unsigned long frameWidth() const { return m_video_format.ulWidth; }
        unsigned long frameHeight() const { return m_video_format.ulHeight; }
		cv::cudacodec::FormatInfo format() { std::lock_guard<std::mutex> lock(m_lock); return m_video_format; }

        unsigned long targetWidth() { return m_video_format.width; }
        unsigned long targetHeight() { return m_video_format.height; }

        cudaVideoChromaFormat chromaFormat() const { return static_cast<cudaVideoChromaFormat>(m_video_format.chromaFormat); }
        int nBitDepthMinus8() const { return m_video_format.nBitDepthMinus8; }
        bool enableHistogram() const { return m_video_format.enableHistogram; }

        bool decodePicture(CUVIDPICPARAMS* picParams)
        {
            return cuvidDecodePicture(m_cuda_decoder, picParams) == CUDA_SUCCESS;
        }

		cv::cuda::GpuMat mapFrame(int picIdx, CUVIDPROCPARAMS& videoProcParams);

        void unmapFrame(cv::cuda::GpuMat& frame);
		void cvtFromYuv(const cv::cuda::GpuMat& decodedFrame, cv::cuda::GpuMat& outFrame, const cv::cudacodec::SurfaceFormat surfaceFormat, const bool videoFullRangeFlag, cv::cuda::Stream& stream = cv::cuda::Stream::Null());

	public:
		// parser data call 
		bool create(const cv::cudacodec::FormatInfo& video_foramt);
		int32_t reconfigure(const cv::cudacodec::FormatInfo& video_format);
		void release();
		bool  inited();
	private:
		CUcontext       m_cuda_ctx;
		CUvideoctxlock    m_cuda_lock;
		CUvideodecoder    m_cuda_decoder;
		cv::cudacodec::FormatInfo m_video_format;
		std::mutex			m_lock;

		std::shared_ptr<cv::cudacodec::NVSurfaceToColorConverter> m_yuvConverter = 0;
		cv::cudacodec::ColorFormat colorFormat = cv::cudacodec::ColorFormat::RGBA;
		cv::cudacodec::BitDepth bitDepth = cv::cudacodec::BitDepth::UNCHANGED;
		bool planar = false;
	};
}



#endif // _C_VIDEO_PARSER_H_
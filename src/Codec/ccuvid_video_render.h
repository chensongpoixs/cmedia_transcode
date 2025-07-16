/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_CUVID_VIDEO_RENDER_H_
#define _C_CUVID_VIDEO_RENDER_H_
#include <string_view>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <future>
#include <cstring>
#include <cstring>

#include <cuda.h>
#include "cvideo_parser.h"
#include "cvideo_decoder.h"
#include "cframe_queue.h"
#include <cuda_runtime.h>
namespace  dsp
{


    class VideoCtxAutoLock
    {
    public:
        VideoCtxAutoLock(CUvideoctxlock lock) : m_lock(lock) { cudaSafeCall(cuvidCtxLock(m_lock, 0)); }
        ~VideoCtxAutoLock() { cuvidCtxUnlock(m_lock, 0); }

    private:
        CUvideoctxlock m_lock;
    };
	class ccuvid_video_render
	{
	public:
        ccuvid_video_render(){}
        virtual ~ccuvid_video_render() { destroy(); }

    public:
        bool init(cv::cudacodec::Codec codec);


        void destroy();

        bool initd() const  { return m_init; }

    public:
        bool parseVideoData(const unsigned char* data, size_t size, int64_t timestamp, const bool containsKeyFrame);



        void frame_end_queue();
    public:

        bool internalGrab(cv::cuda::GpuMat& frame, int64_t& pts, cv::cuda::GpuMat& histogram, cudaStream_t stream = 0);
        bool aquireFrameInfo(std::pair<CUVIDPARSERDISPINFO, CUVIDPROCPARAMS>& frameInfo, cudaStream_t stream = 0);
        void releaseFrameInfo(const std::pair<CUVIDPARSERDISPINFO, CUVIDPROCPARAMS>& frameInfo);
        bool  nextFrame(cv::cuda::GpuMat& frame, int64_t &pts, cv::cuda::GpuMat& histogram, cudaStream_t stream);
    private:


      //  void _render_thread();



	private:
        
        std::shared_ptr<cframe_queue> m_frame_queue;
        std::shared_ptr<cvideo_decoder> m_video_decoder;
        std::shared_ptr<cvideo_parser> m_video_parser ;

        CUvideoctxlock m_cuda_lock;

        std::deque< std::pair<CUVIDPARSERDISPINFO, CUVIDPROCPARAMS> > m_frames;
        std::vector<RawPacket> m_rawPackets;
        cv::cuda::GpuMat m_lastFrame, m_lastHistogram;
        static const int m_decodedFrameIdx = 0;
        static const int m_extraDataIdx = 1;
        static const int m_rawPacketsBaseIdx = 2;
       
        cv::cudacodec::ColorFormat m_colorFormat = cv::cudacodec::ColorFormat::RGBA;
        cv::cudacodec::BitDepth m_bitDepth = cv::cudacodec::BitDepth::UNCHANGED;
        bool m_planar = false;
        static const std::string m_errorMsg;
        int m_iFrame = 0;
        CUcontext m_cuda_ctx = nullptr;


        std::thread    m_renader_thread;

        //std::shared_ptr<cv::cudacodec::NVSurfaceToColorConverter> m_yuvConverter = 0;
        std::atomic_bool      m_init = false;
	};
}



#endif // _C_VIDEO_PARSER_H_
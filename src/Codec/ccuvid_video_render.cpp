/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/
#include "ccuvid_video_render.h"
#include "ccuda_define.h"
//#include <opencv2/imgproc.hpp>
//#include <opencv2/highgui.hpp>
namespace  dsp
{
	bool ccuvid_video_render::init(cv::cudacodec::Codec codec)
	{
        cv::cuda::GpuMat temp(1, 1, CV_8UC1);
        temp.release();
        cudaSafeCall(cuCtxGetCurrent(&m_cuda_ctx));
        cudaSafeCall(cuvidCtxLockCreate(&m_cuda_lock, m_cuda_ctx));
        m_frame_queue.reset(new cframe_queue());
        m_video_decoder.reset(new cvideo_decoder(codec, 0 /*minNumDecodeSurfaces*/, cv::Size(), cv::Rect(), cv::Rect(), false, m_cuda_ctx, m_cuda_lock));
        m_video_parser.reset(new cvideo_parser(m_video_decoder, m_frame_queue, 0, 0));
        /*videoSource_->setVideoParser(m_video_parser);
        videoSource_->start();*/
       // waitForDecoderInit();
        //FormatInfo format = videoDecoder_->format();
#if NVIDIA_CALLBACK
        if (format.colorSpaceStandard == cv::cudacodec::ColorSpaceStandard::Unspecified) {
            if (format.width > 1280 || format.height > 720)
            {
                format.colorSpaceStandard = cv::cudacodec::ColorSpaceStandard::BT709;
            }
            else
            {
                format.colorSpaceStandard = cv::cudacodec::ColorSpaceStandard::BT601;
            }
        }
        // createNVSurfaceToColorConverter
        m_yuvConverter = createNVSurfaceToColorConverter(format.colorSpaceStandard, format.videoFullRangeFlag);
#endif 
       /* for (iFrame = videoSource_->getFirstFrameIdx(); iFrame < firstFrameIdx; iFrame++)
        {
            assert(skipFrame());
        }*/
        //videoSource_->updateFormat(format);



        m_init.store(true);

       // m_renader_thread = std::thread(&ccuvid_video_render::_render_thread, this);
		return true;
	}
    void ccuvid_video_render::destroy()
    {
        InfoL << "--->";
        m_init.store(false);
        
        if (m_video_decoder)
        {
            m_video_decoder->destroy();
        }
        if (m_video_parser)
        {
            m_video_parser->destroy();
        }
        if (m_cuda_ctx)
        {
            cudaSafeCall(cuvidCtxLockDestroy(m_cuda_lock));
            m_cuda_lock = nullptr;
            cudaSafeCall(cuCtxDestroy(m_cuda_ctx));
            m_cuda_ctx = nullptr;
       }
        
    }
    bool ccuvid_video_render::parseVideoData(const unsigned char* data, size_t size, int64_t timestamp, const bool containsKeyFrame)
    {
        if (!m_init)
        {
            WarnL << "parse Video Data  not init !!!";
            return false;
        }
        m_video_parser->parseVideoData(data, size, timestamp, false, containsKeyFrame, false);

        return true;
    }
    void ccuvid_video_render::frame_end_queue()
    {
        if (m_frame_queue)
        {
            m_frame_queue->endDecode();
        }
    }
    bool ccuvid_video_render::internalGrab(cv::cuda::GpuMat& frame, int64_t& pts, cv::cuda::GpuMat& histogram, cudaStream_t stream)
    {
        if (m_video_parser->hasError())
        {
            printf("[%s][%d]\n", __FUNCTION__, __LINE__);
        }
      //  InfoL << "";
        std::pair<CUVIDPARSERDISPINFO, CUVIDPROCPARAMS> frameInfo;
        if (!aquireFrameInfo(frameInfo, stream))
        {
            return false;
        }
       // InfoL << "";
        {
            VideoCtxAutoLock autoLock(m_cuda_lock);

            /*unsigned long long cuHistogramPtr = 0;
            const cudacodec::FormatInfo fmt = videoDecoder_->format();
            if (fmt.enableHistogram)
                frameInfo.second.histogram_dptr = &cuHistogramPtr;*/

                // map decoded video frame to CUDA surface
            cv::cuda::GpuMat decodedFrame = m_video_decoder->mapFrame(frameInfo.first.picture_index, frameInfo.second);
            pts = frameInfo.first.timestamp;
            /*if (fmt.enableHistogram) {
                const size_t histogramSz = 4 * fmt.nMaxHistogramBins;
                histogram.create(1, fmt.nMaxHistogramBins, CV_32S);
                cudaSafeCall(cuMemcpyDtoDAsync((CUdeviceptr)(histogram.data), cuHistogramPtr, histogramSz, StreamAccessor::getStream(stream)));
            }*/
            //  static uint8_t* y_data = new uint8_t[(1920 * 1080 * 4)];
             // static uint8_t* uv_data = new uint8_t[(1920 * 1080)];
             // cudaSafeCall(cuCtxSetCurrent(ctx));
            //  cudaSafeCall(cuCtxPushCurrent(cuda_ctx));
            /*  static uint8_t* y_data = NULL;
              static uint8_t* uv_data = NULL;*/

              //decodedFrame.download(&y_data, &uv_data);
        //    InfoL << "";
             // cudaSafeCall(cuCtxPopCurrent(NULL));
              //static FILE* out_file_ptr = ::fopen("test.yuv", "wb+");
             /* if (y_data)
              {
                  ::fwrite(y_data, 1, decodedFrame.height_ * decodedFrame.width_, out_file_ptr);
                  ::fflush(out_file_ptr);
              }*/
          //  frame=   decodedFrame.clone();
            m_video_decoder->cvtFromYuv(decodedFrame, frame, m_video_decoder->format().surfaceFormat, m_video_decoder->format().videoFullRangeFlag);
           // cvtFromYuv(decodedFrame, frame, videoDecoder_->format().surfaceFormat, videoDecoder_->format().videoFullRangeFlag, cuda::Stream::Null());
            /* cv::Mat  renderframe;
             frame.download(renderframe);
             cv::imshow("", renderframe);
             cv::waitKey(1);*/
             // unmap video frame
             // unmapFrame() synchronizes with the VideoDecode API (ensures the frame has finished decoding)
         //   InfoL << "";
            m_video_decoder->unmapFrame(decodedFrame);
        }
      //  InfoL << "";
        releaseFrameInfo(frameInfo);
     //   InfoL << "";
        m_iFrame++;
        return true;
    }
    bool ccuvid_video_render::aquireFrameInfo(std::pair<CUVIDPARSERDISPINFO, CUVIDPROCPARAMS>& frameInfo, cudaStream_t stream)
    {
        if (m_frames.empty())
        {
            CUVIDPARSERDISPINFO displayInfo;
            m_rawPackets.clear();
            for (;;)
            {
                if (m_frame_queue->dequeue(displayInfo, m_rawPackets))
                {
                    break;
                }

                if (m_video_parser->hasError())
                {
                    printf("[%s][%d]\n", __FUNCTION__, __LINE__);
                }

                if (m_frame_queue->isEndOfDecode())
                {
                    return false;
                }

                // Wait a bit
               // Thread::sleep(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            bool isProgressive = displayInfo.progressive_frame != 0;
            const int num_fields = isProgressive ? 1 : 2 + displayInfo.repeat_first_field;

            for (int active_field = 0; active_field < num_fields; ++active_field)
            {
                CUVIDPROCPARAMS videoProcParams;
                std::memset(&videoProcParams, 0, sizeof(CUVIDPROCPARAMS));

                videoProcParams.progressive_frame = displayInfo.progressive_frame;
                videoProcParams.second_field = active_field;
                videoProcParams.top_field_first = displayInfo.top_field_first;
                videoProcParams.unpaired_field = (num_fields == 1);
                videoProcParams.output_stream = stream;// StreamAccessor::getStream(stream);

                m_frames.push_back(std::make_pair(displayInfo, videoProcParams));
            }
        }
        else {
            for (auto& frame : m_frames)
            {
                frame.second.output_stream = stream;//StreamAccessor::getStream(stream);
            }
        }

        if (m_frames.empty())
        {
            return false;
        }

        frameInfo = m_frames.front();
        m_frames.pop_front();
        return true;
    }
    void ccuvid_video_render::releaseFrameInfo(const std::pair<CUVIDPARSERDISPINFO, CUVIDPROCPARAMS>& frameInfo)
    {
        if (m_frames.empty())
        {
            m_frame_queue->releaseFrame(frameInfo.first);
        }
    }
    bool ccuvid_video_render::nextFrame(cv::cuda::GpuMat& frame, int64_t& pts, cv::cuda::GpuMat& histogram, cudaStream_t stream)
    {
        if (!m_init)
        {
           // std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return false;
        }
        if (!internalGrab(frame, pts, histogram, stream))
        {
            return false;
        }
      //  InfoL << "";
        return true;
    }
   /* void ccuvid_video_render::_render_thread()
    {
        cv::cuda::GpuMat frame, tmp;
        cv::Mat frameHost, frameHostGs, frameFromDevice, unused;
        while (true)
        {
            if (!nextFrame(frame, tmp, 0))
            {

            }
            else
            {
                frame.download(frameFromDevice);
                cv::imshow("====", frameFromDevice);
                cv::waitKey(1);
            }
        }
    }*/
}



 
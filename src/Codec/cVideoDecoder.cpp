/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/
#include "cVideoDecoder.h"
#include "Util/logger.h"
#ifdef _MSC_VER
 

#elif defined(__GNUC__)

#else

#endif
#include "NvCodecUtils.h"
#include "cVideoEncoder.h"
#include "ext-codec/H264.h"
#include "ext-codec/H265.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudawarping.hpp> // cv::cuda::resize
#include "Common/ctranscode_info_mgr.h"

namespace mediakit {
//cVideoDecoder::cVideoDecoder() {}
//cVideoDecoder::~cVideoDecoder() {}

    static int32_t g_video_decoder_count = 0;
    cVideoDecoder::~cVideoDecoder()
    {
        InfoL << "--->" << ", count =" << m_count << ", transcode_info =" << m_transcode_info.toString();
        destroy();
    }
    bool cVideoDecoder::init(DISPATHFRAME callback /*std::shared_ptr<FrameDispatcher> ptr*/,  const  MediaTuple&   media)
	{
        m_count = ++g_video_decoder_count;


        m_media = media;
        const  dsp::ctranscode_info* transcode_ptr = dsp::ctranscode_info_mgr::Instance().find(m_media);
        if (!transcode_ptr)
        {
            WarnL << "not find media " << m_media.shortUrl();
           // return;
        }
        m_transcode_info = *transcode_ptr;
        int32_t fps = m_transcode_info.get_fps();
        if (fps < 10)
        {
            fps = 25;
        }
        m_frame_samples = 90000 / 25;
        m_cuvid_video_render = std::make_shared<dsp::ccuvid_video_render>();
       // m_cuvid_video_render->init(cv::cudacodec::Codec::H264);
        m_encoder = std::make_shared<cVideoEncoder>();
        m_stoped.store(false);
        m_frame_callback = callback;
        //  _FrameWriterInterface = ptr;
        m_thread = std::thread(&cVideoDecoder::_work_thread, this);
        m_encoder_thread = std::thread(&cVideoDecoder::_encoder_thread, this);
        return true;
        //1920x1080
        int width = 1920;
        int height = 1080;
        const Rect cropRect = { 0, 0, 1920, 1080 };
        const Dim resizeDim = {};
        ck(cuInit(0));
        int nGpu = 0;
        int32_t iGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        if (iGpu < 0 || iGpu >= nGpu) {
            std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
            return 1;
        }
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        std::cout << "GPU in use: " << szDeviceName << std::endl;
        CUcontext cuContext = NULL;
        ck(cuCtxCreate(&cuContext, 0, cuDevice));
        m_nv_decoder = /*std::make_shared<NvDecoder>*/ 
            new NvDecoder( std::move(cuContext), width, height, false, cudaVideoCodec_H264, NULL, false, false, &cropRect, &resizeDim, 4096, 4096);
		//const AVCodec *codec = ::avcodec_find_decoder(AV_CODEC_ID_H264); //::avcodec_find_decoder_by_name("");
		//if (!codec)
		//{
  //          ErrorL << "avcodec not find h264 !!!";
  //          return false;
		//}

  //        m_parser = av_parser_init(codec->id);
  //      if (!m_parser) {
  //          ErrorL << "parser not found\n";
  //          return false;
  //         // exit(1);
  //      }
  //      m_c = ::avcodec_alloc_context3(codec);
  //      if (!m_c) {
  //          ErrorL << "avcodec_alloc_context3 init  !!!";
  //          return false;
  //      }


		//if (avcodec_open2(m_c, codec, NULL) < 0) {
  //          ErrorL <<"Could not open codec\n";
  //          return false;
  //      }
        m_stoped.store(false);
        m_frame_callback = callback;
      //  _FrameWriterInterface = ptr;
        m_thread = std::thread(&cVideoDecoder::_work_thread, this);
		return true;
    }

    void cVideoDecoder::stop()
    {
        m_stoped.store(true);
        
        m_condition.notify_all();
        if (m_cuvid_video_render)
        {
            m_cuvid_video_render->frame_end_queue();
        }
    }

    void cVideoDecoder::destroy()
    {
        stop();
        InfoL << " thread join able ...";
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        InfoL << " thread join able  thread exit ..";
        if (m_encoder_thread.joinable())
        {
            m_encoder_thread.join();
        }
        InfoL << " thread join able encoder exit end OK !!!";

        m_packet_queue.clear();

        InfoL <<   "cuvid video render destroy !!!";
        if (m_cuvid_video_render)
        {
            m_cuvid_video_render->destroy();

        }
        InfoL << "cuvid video render destroy OK !!!";
        if (m_encoder)
        {
            m_encoder->destroy();
        }
        InfoL << "cuvid video encoder  destroy OK !!!";
    }

    void cVideoDecoder::push_frame(/*const char *  data, int32_t size*/ const std::shared_ptr<Frame> &frame)
    {
    
        /*AVPacket pkt ;
        int32_t ret = ::av_new_packet(&pkt, frame->size());
        memcpy(pkt.data, frame->data(), frame->size());*/
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_packet_queue.push_back(frame);
            //std::cout << "video decoder = " << m_count << "][ totoal video decoder num = " << g_video_decoder_count << "]" << std::endl;
           // InfoL <<   "video decoder = " << m_count << "][ totoal video decoder num = " << g_video_decoder_count << "]";
        }
        m_condition.notify_one();
       
    }

    CodecId cVideoDecoder::get_video_codec()   
    {
       /* if (m_encoder)
        {
            return m_encoder->
        }*/
        return m_transcode_info.get_codec();
        //return CodecId();
    }

    void cVideoDecoder::onEncoded(const std::vector<std::vector<uint8_t>>& vPacket, const std::vector<uint64_t>& pts)
    {
        //static FILE* out_file_ptr = NULL;
        if (m_frame_callback && !m_stoped)
        {
            int32_t packet_index = 0;
            for (const std::vector<uint8_t >& d : vPacket) 
            {
                if (d.size()  <5)
                {
                    continue;
               }
                if (m_stoped)
                {
                    break;
                }
               /* if (!out_file_ptr)
                {
                    out_file_ptr = fopen("test____dd.h264", "wb+");
                }
                fwrite(&d[0], d.size(), 1, out_file_ptr);
                fflush(out_file_ptr);*/
                if (m_transcode_info.get_codec() == mediakit::CodecId::CodecH265)
                {
                    auto frame1 = /*mediakit::*/ FrameImp::create</*mediakit::*/ H265Frame>();
                    frame1->_prefix_size = 4;
                    frame1->_dts = pts[packet_index] /** m_frame_samples*/;
                  //printf("[pts = %llu]\n", pts[packet_index]); 
                //  InfoL << "[pts = "<< pts[packet_index] <<"]";
                    frame1->_pts = pts[packet_index] /** m_frame_samples*/;
                    frame1->setIndex(0); 
                    ++packet_index; 
                    frame1->_buffer.append((const char*)&d[0], d.size());
                 
                    m_frame_callback(frame1);
                }
                else
                {
                    auto frame1 = /*mediakit::*/ FrameImp::create</*mediakit::*/ H264Frame>();
                    frame1->_prefix_size = 4;
                    frame1->_dts = pts[packet_index] /** m_frame_samples*/;
                   // InfoL << "[pts = " << pts[packet_index] << "]";
                    // printf("[pts = %llu]\n", pts[packet_index]);
                    //frame->dts();

                    //packet_pts[packet_index];
                    //frame->dts();

                   // packet_pts[packet_index]; // //frame->dts();
                    frame1->_pts = pts[packet_index] /** m_frame_samples*/;
                    frame1->setIndex(0);
                    //frame->dts();
                    //packet_pts[packet_index]; //

                    //packet_pts[packet_index]; // frame->pts();
                    ++packet_index;
                    // frame1->_buffer.assign("\x00\x00\x00\x01", 4); // 添加264头
                    frame1->_buffer.append((const char*)&d[0], d.size());
                    if (false)
                    {
                        auto ptr = frame1->data() + frame1->prefixSize();
                        switch (H264_TYPE(ptr[0])) {
                        case H264Frame::NAL_SPS: {
                            {

                                InfoL << "==================_sps --->";

                            }
                        }
                        case H264Frame::NAL_PPS: {
                            {


                                InfoL << "===============_pps --->";

                            }
                        }
                        default: break;
                        }
                    }
                    /* fwrite(reinterpret_cast<char *>(d.data()), 1, d.size(), out_file_ptr);
                    fflush(out_file_ptr); */
                    //  auto frame2 = std::make_shared<FrameStamp>(frame, (frame1->dts(), frame1->pts()), 2);
                    m_frame_callback(frame1);
                }
            }
        }
    
    }

    bool cVideoDecoder::setFrameIntervalP(const int frameIntervalP)
    {
        InfoL << "media transcode info " << m_transcode_info.toString() << ", P  = " << frameIntervalP;
        return true;
    }


    void cVideoDecoder::onEncodingFinished()
    {
    }

    void cVideoDecoder::_work_thread()
    {
        std::shared_ptr<Frame> packet_ptr = NULL;
        while (!m_stoped)
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_condition.wait(lock, [this]() { return m_packet_queue.size() > 0 || m_stoped; });
            }


            while (!m_packet_queue.empty() && !m_stoped) {
                {
                    std::lock_guard<std::mutex> lock { m_lock };
                    packet_ptr = m_packet_queue.front();
                   // m_packet_queue.pop_front();
                }

                if (!packet_ptr) 
                {
                    {
                        std::lock_guard<std::mutex> lock{ m_lock };
                        //packet_ptr = &m_packet_queue.front();
                        m_packet_queue.pop_front();
                    }
                    continue;
                }

                _handler_packet_item( ( packet_ptr));

               // av_packet_unref(packet_ptr);
               // av_packet_free(&packet_ptr);
                packet_ptr = NULL;
                {
                    std::lock_guard<std::mutex> lock { m_lock };
                    //packet_ptr = &m_packet_queue.front();
                     m_packet_queue.pop_front();
                }

            }
        }
    }
    void cVideoDecoder::_encoder_thread()
    {

       
       // m_encoder->init(1920, 1080, m_transcode_info.get_fps(), m_transcode_info.get_codec());
        m_encoder->init(m_transcode_info);
        cv::cuda::GpuMat frame, tmp;
        cv::Mat frameHost, frameHostGs, frameFromDevice, unused;
        int64_t pts = 0;

        cv::cuda::GpuMat  encode_frame;
      //  encode_frame.create(cv::Size(m_transcode_info.get_width(), m_transcode_info.get_height()), cv::INTER_LINEAR);
        int64_t   frame_ms = (1000000) / m_transcode_info.get_fps();
        cv::cuda::Stream resize_stream = cv::cuda::Stream::Stream();
       // std::chrono::microseconds   pre_cur ;
        int64_t  cur_frame_ms = 0;
        while (!m_stoped)
        {
            std::chrono::microseconds pre_mils = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch());
          
            if (m_cuvid_video_render->nextFrame(frame, pts, tmp, 0))
            {
               // InfoL << "";
               // frame.channels();
            /*  frame.download(frameFromDevice);
               cv::imshow("====", frameFromDevice);
               frameFromDevice.release();
                cv::waitKey(1); */
              //  InfoL << "";
                //(InputArray src, OutputArray dst, Size dsize, double fx=0, double fy=0, int interpolation = INTER_LINEAR, Stream& stream = Stream::Null());
                std::chrono::microseconds resize_mils = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
               
                cv::cuda::resize(frame, encode_frame, cv::Size(m_transcode_info.get_width(), m_transcode_info.get_height()), 0, 0, cv::INTER_NEAREST, resize_stream);
                std::chrono::microseconds cur_mils = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
                std::chrono::microseconds diff_mils = resize_mils - pre_mils;;
                InfoL << "nvidia resize ----> " << diff_mils.count();
                m_encoder->encode(encode_frame, pts, this);
               
                encode_frame.release();
              //  InfoL << "";
              //  frameFromDevice.release();
                //frame.release();
                {
                    std::chrono::microseconds cur_mils = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch());
                    std::chrono::microseconds diff_mils = cur_mils - pre_mils;
                    cur_frame_ms += frame_ms;
                    if (diff_mils.count() < cur_frame_ms)
                    {
                        std::this_thread::sleep_for(std::chrono::microseconds(cur_frame_ms - diff_mils.count()));
                        cur_frame_ms = 0;
                    }
                    else
                    {
                        cur_frame_ms -= (diff_mils.count() - cur_frame_ms);
                    }
                
                }
            }
            /*else if (!m_stoped)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }*/
           
           /* else
            {
                pre_cur = frame_ms - diff_mils.count();
            }*/
            //InfoL << "";
        }
        
        InfoL << "encoder work  thread exit !!!";
    }
    void cVideoDecoder::_handler_packet_item(std::shared_ptr<Frame> &frame) 
    {
    
        if (!m_cuvid_video_render->initd())
        {
            cv::cudacodec::Codec   codec = cv::cudacodec::Codec::H264;
            if (frame->getCodecId() == mediakit::CodecH264)
            {
                codec = cv::cudacodec::Codec::H264;
            }
            else if (frame->getCodecId() == mediakit::CodecH265)
            {
                codec = cv::cudacodec::Codec::HEVC;
            }
            else
            {
                codec = cv::cudacodec::Codec::H264;
            }
            m_cuvid_video_render->init(codec);
        }

        m_cuvid_video_render->parseVideoData((const unsigned char*)frame->data(), frame->size(), frame->dts(), frame->keyFrame());


        return;

      //  std::shared_ptr<FrameImp> frame1 = std::dynamic_pointer_cast<FrameImp>(frame);
        int32_t ret = 0;
        int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
        uint8_t *pVideo = NULL, **ppFrame;

        if (!m_nv_decoder->Decode((const uint8_t*)frame->data(), frame->size(), &ppFrame, &nFrameReturned))
        {
            WarnL << "nv decoder      frame  failed !!!";
        }
        if (!nFrame && nFrameReturned)
        {
          //  InfoL << m_nv_decoder->GetVideoInfo();
        }
        else
        {
            WarnL << "nv decoder -------->   ";
        }
        if (false)
        {
            auto ptr = frame->data() + frame->prefixSize();
            switch (H264_TYPE(ptr[0])) {
                case H264Frame::NAL_SPS: {
                    {

                        InfoL << "==========in frame========_sps --->";
                    }
                }
                case H264Frame::NAL_PPS: {
                    {

                        InfoL << "=======in frame ========_pps --->";
                    }
                }
                default: break;
            }
        }
        {
            int type = H265_TYPE(frame->data()[frame->prefixSize()]);
            bool ret = true;
            switch (type) {
                case H265Frame::NAL_VPS: {
                    InfoL << "=======in H265 frame ========VPS --->";
                    break;
                }
                case H265Frame::NAL_SPS: {
                    InfoL << "=======in H265 frame ========_Sps --->";
                    break;
                }
                case H265Frame::NAL_PPS: {
                    InfoL << "=======in H265 frame ========_pps --->";
                }
                default: {
                    // 判断是否是I帧, 并且如果是,那判断前面是否插入过config帧, 如果插入过就不插入了
                   
                    break;
                }
            }
        }
       //InfoL << "nFrameReturned  << " << nFrameReturned << ", pts = " << frame->pts() << ", dts = " << frame->dts();
      //  std::cout << "nFrameReturned =" << nFrameReturned << ", pts = " << frame->pts() << ", dts = " << frame->dts() << std::endl;
        //m_frame_callback(frame);
        if (nFrameReturned <= 0)
        {
            return;
        }

      /*   static FILE *out_file_ptr = NULL;
        if (!out_file_ptr)
        {
            out_file_ptr = fopen("test.h265", "wb+");
        } */
        if (!m_encoder) {
            m_encoder = std::make_shared<cVideoEncoder>();
            m_encoder->init(1920,1080, 25); // m_c->framerate);
        }

        std::vector<std::vector<uint8_t>> packet;
        std::vector<uint64_t> packet_pts;
        for (int i = 0; i < nFrameReturned; i++) 
        {
            packet.clear();
            if (m_encoder) {
                m_encoder->encode(ppFrame[i], m_nv_decoder->GetFrameSize(), frame->pts(), packet , packet_pts /*, _FrameWriterInterface*/);
                if (m_frame_callback)
                {
                    int32_t packet_index = 0;
                    for (std::vector<uint8_t >& d : packet)
                    {
                       /* if (d.size() < 6)
                        {
                            WarnL << " nv packet size -> size = " << d.size();
                            continue;
                        }*/
                        /*frame1->_buffer.clear();
                        frame1->_prefix_size = 4;
                        frame1->_buffer.append((const char *)&d[0] + 4, d.size() - 4);*/
                        auto frame1 = /*mediakit::*/ FrameImp::create</*mediakit::*/ H264Frame>();
                        frame1->_prefix_size = 4;
                        frame1->_dts = 0;
                        //frame->dts();
                        
                        //packet_pts[packet_index];
                        //frame->dts();
                         
                       // packet_pts[packet_index]; // //frame->dts();
                        frame1->_pts = 0;
                        frame1->setIndex(0);
                        //frame->dts();
                        //packet_pts[packet_index]; //
                        
                        //packet_pts[packet_index]; // frame->pts();
                        ++packet_index;
                        // frame1->_buffer.assign("\x00\x00\x00\x01", 4); // 添加264头
                        frame1->_buffer.append((const char *) & d[0], d.size());
                        if (false)
                        {
                            auto ptr = frame1->data() + frame1->prefixSize();
                            switch (H264_TYPE(ptr[0])) {
                                case H264Frame::NAL_SPS: {
                                    {
                                        
                                        InfoL << "==================_sps --->";
                                       
                                    }
                                }
                                case H264Frame::NAL_PPS: {
                                    {
                                        

                                        InfoL << "===============_pps --->";
                                        
                                    }
                                }
                                default: break;
                            }
                        }
                        /* fwrite(reinterpret_cast<char *>(d.data()), 1, d.size(), out_file_ptr);
                        fflush(out_file_ptr); */
                      //  auto frame2 = std::make_shared<FrameStamp>(frame, (frame1->dts(), frame1->pts()), 2);
                        m_frame_callback( frame1 );
                    }
                }
            }
            
            // fpOut.write(reinterpret_cast<char*>(ppFrame[i]), dec.GetFrameSize());
        }
      
      
 
    }
   
} // namespace chen
 
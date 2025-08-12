/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

 #include "cVideoEncoder.h"
#include "Util/logger.h"
//#include "H264Encoder.h"
#include "ext-codec/H264.h"
#include <string>
#include "Network/Buffer.h"
#include "NvCodecUtils.h"
#include "cvideo_writer.h"
 
namespace mediakit {
    cVideoEncoder::~cVideoEncoder()
    {
        InfoL << "----> ";
    }
    bool cVideoEncoder::init(int32_t width, int32_t height, int32_t fps, const mediakit::CodecId& codec)
	{

      //  encoder_params.nvPreset = cv::cudacodec::EncodePreset::ENC_PRESET_P5;
       // encoder_params.encodingProfile = cv::cudacodec::EncodeProfile::ENC_H264_PROFILE_MAIN;
      //  encoder_params.rateControlMode = cv::cudacodec::EncodeParamsRcMode::ENC_PARAMS_RC_CBR;
#ifdef _MSC_VER
        encoder_params.tuningInfo = cv::cudacodec::EncodeTuningInfo::ENC_TUNING_INFO_LOSSLESS; 
        encoder_params.tuningInfo = cv::cudacodec::EncodeTuningInfo::ENC_TUNING_INFO_LOSSLESS;
        encoder_params.rateControlMode = cv::cudacodec::EncodeParamsRcMode::ENC_PARAMS_RC_CONSTQP;
        encoder_params.videoFullRangeFlag = true;
       // encoder_params.constQp = {28, 31, 25};
       /* encoder_params.averageBitRate = 8000 * 1000;
        encoder_params.maxBitRate = 10000 * 1000;*/
      //  encoder_params.multiPassEncoding = cv::cudacodec::EncodeMultiPass::ENC_TWO_PASS_FULL_RESOLUTION;
      //  encoder_params.videoFullRangeFlag = true;
        
        m_fps = fps;
        if (codec == mediakit::CodecH264)
        {
            m_codec = cv::cudacodec::Codec::H264;
       }
        else if(codec == mediakit::CodecH265)
        {
            m_codec = cv::cudacodec::Codec::HEVC;
        }
        else
        {
            m_codec = cv::cudacodec::Codec::H264;
        }


        return true;
        int iGpu = 0;
         ck(cuInit(0));
        int nGpu = 0;
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


        m_encoder = new NvEncoderCuda(cuContext, width, height, NV_ENC_BUFFER_FORMAT_NV12);


        NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
        NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
        initializeParams.encodeConfig = &encodeConfig;
        m_encoder->CreateDefaultEncoderParams(&initializeParams, NV_ENC_CODEC_HEVC_GUID, NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID);
        initializeParams.encodeWidth = width;
        initializeParams.encodeHeight = height;
        initializeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
       // initializeParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
        initializeParams.encodeGUID = NV_ENC_CODEC_HEVC_GUID;
        initializeParams.presetGUID = NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
        initializeParams.enableEncodeAsync = 0;
        initializeParams.frameRateDen = 1;
        initializeParams.enablePTD = 1;
        initializeParams.reportSliceOffsets = 0;
        initializeParams.enableSubFrameWrite = 0;

       // initializeParams.tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
        initializeParams.frameRateNum = 25;
        initializeParams.encodeConfig->gopLength = 120;
        if (initializeParams.encodeGUID == NV_ENC_CODEC_H264_GUID)
        {
            ///////////////
            // H.264 specific settings
            ///
            //  NV_ENC_H264_PROFILE_BASELINE_GUI
            // initializeParams.encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.enableIntraRefresh = 1;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.intraRefreshPeriod = 180;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.intraRefreshCnt = 180;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.idrPeriod = 120; //
            // NVENC_INFINITE_GOPLENGTH; //
            // NVENC_INFINITE_GOPLENGTH;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.repeatSPSPPS = 1;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.disableSPSPPS = 0;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputAUD = 1;
            // h264_config->qpPrimeYZeroTransformBypassFlag = 1;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.qpPrimeYZeroTransformBypassFlag = 1;
            /*    h264_config->disableSPSPPS = 0;
                h264_config->outputAUD = 1;*/
            /*
             * Slice mode - set the slice mode to "entire frame as a single slice" because WebRTC implementation doesn't work well with slicing. The default
             * slicing mode produces (rarely, but especially under packet loss) grey full screen or just top half of it.
             */
            // slice 大小
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceMode = 3;
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceModeData = 1;
            // `outputPictureTimingSEI` is used in CBR mode to fill video frame with data to match the requested bitrate.
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputPictureTimingSEI = 1;
            // initializeParams.encodeConfig->encodeCodecConfig.h264Config.enableFillerDataInsertion = 1;
       
            // NVENC_INFINITE_GOPLENGTH;

            // h264_config->useBFramesAsRef
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_EACH; //
            /////////////////////////  vui params /////////////////////////
            //   NV_ENC_CONFIG_H264_VUI_PARAMETERS *vui_params = &initializeParams.encodeConfig->encodeCodecConfig.h264Config.h264VUIParameters;
            //   vui_params->videoSignalTypePresentFlag = 1;
            //   vui_params->videoFullRangeFlag = 1;//
            ////   (voi->range == VIDEO_RANGE_FULL);
            //   vui_params->colourDescriptionPresentFlag = 1;
            //   vui_params->colourPrimaries = 1;
            //   vui_params->transferCharacteristics = 1;
            //   vui_params->colourMatrix = 1;
            /*switch (voi->colorspace) {
                case VIDEO_CS_601:
                    vui_params->colourPrimaries = 6;
                    vui_params->transferCharacteristics = 6;
                    vui_params->colourMatrix = 6;
                    break;
                case VIDEO_CS_DEFAULT:
                case VIDEO_CS_709:
                    vui_params->colourPrimaries = 1;
                    vui_params->transferCharacteristics = 1;
                    vui_params->colourMatrix = 1;
                    break;
                case VIDEO_CS_SRGB:
                    vui_params->colourPrimaries = 1;
                    vui_params->transferCharacteristics = 13;
                    vui_params->colourMatrix = 1;
                    break;
                default: break;
            }*/
            // g_cfg.get_uint32(ECI_EncoderVideoGop); // NVENC_INFINITE_GOPLENGTH;//
            //  initializeParams.encodeConfig->rcParams.averageBitRate = g_cfg.get_uint32(ECI_RtcAvgRate) * 1000 ;
            //  initializeParams.encodeConfig->rcParams.maxBitRate = g_cfg.get_uint32(ECI_RtcMaxRate) * 1000;
            //  initializeParams.encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;// NV_ENC_PARAMS_RC_VBR_HQ;// NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
            //  initializeParams.encodeConfig->rcParams.qpMapMode = NV_ENC_QP_MAP_DELTA;
            
             NV_ENC_RC_PARAMS &RateControlParams = initializeParams.encodeConfig->rcParams;
#define DEFAULT_BITRATE (1000000u)
            uint32_t const MinQP = static_cast<uint32_t>(1);
            uint32_t const MaxQP = static_cast<uint32_t>(51);
            RateControlParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
            RateControlParams.averageBitRate = 500000; // DEFAULT_BITRATE;
            RateControlParams.maxBitRate = 800000; // DEFAULT_BITRATE; // Not used for CBR
            //  RateControlParams.multiPass = NV_ENC_TWO_PASS_FULL_RESOLUTION;
            RateControlParams.minQP = { MinQP, MinQP, MinQP };
            RateControlParams.maxQP = { MaxQP, MaxQP, MaxQP };
            RateControlParams.enableMinQP = 1;
            RateControlParams.enableMaxQP = 1;
        }
        else
        {
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.enableIntraRefresh = 1;
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.intraRefreshPeriod = 180;
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.intraRefreshCnt = 180;
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.idrPeriod = 120; //
            // NVENC_INFINITE_GOPLENGTH; //
            // NVENC_INFINITE_GOPLENGTH;
        /*    initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.repeatSPSPPS = 1;
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.disableSPSPPS = 0;
            initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.outputAUD = 1;*/
             initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.sliceMode = 3;
             initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.sliceModeData = 1;
             initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.outputBufferingPeriodSEI = 1;
             initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.outputPictureTimingSEI = 1;
             initializeParams.encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
          //   NV_ENC_RC_PARAMS &RateControlParams = initializeParams.encodeConfig->rcParams;
          ////  RateControlParams.g
          //  //uint32_t const MinQP = static_cast<uint32_t>(1);
          //  //uint32_t const MaxQP = static_cast<uint32_t>(51);
          //  RateControlParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
          //  RateControlParams.averageBitRate = 500000; // DEFAULT_BITRATE;
          //  RateControlParams.maxBitRate = 800000; // DEFAULT_BITRATE; // Not used for CBR
          //  RateControlParams.enableAQ = 1;
          //  RateControlParams.enableLookahead = 1;
          //  RateControlParams.vbvBufferSize = 500000;
          //  RateControlParams.constQP = {28, 31, 25};
            ////  RateControlParams.multiPass = NV_ENC_TWO_PASS_FULL_RESOLUTION;
            //RateControlParams.minQP = { MinQP, MinQP, MinQP };
            //RateControlParams.maxQP = { MaxQP, MaxQP, MaxQP };
            //RateControlParams.enableMinQP = 1;
            //RateControlParams.enableMaxQP = 1;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.repeatSPSPPS = 1;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.disableSPSPPS = 0;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.outputAUD = 1;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.idrPeriod = 180;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.sliceMode = 3;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.sliceModeData = 1;
           //// initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.useConstrainedIntraPred
           // initializeParams.encodeConfig->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
           // initializeParams.encodeConfig->frameIntervalP = 5;
           // initializeParams.encodeConfig->gopLength = 180;
            //initializeParams.encodeConfig->encodeCodecConfig.hevcConfig
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.inputBitDepth = is_10_bit(enc) ? NV_ENC_BIT_DEPTH_10 : NV_ENC_BIT_DEPTH_8;
           // initializeParams.encodeConfig->encodeCodecConfig.hevcConfig.outputBitDepth = profile_is_10bpc ? NV_ENC_BIT_DEPTH_10 : NV_ENC_BIT_DEPTH_8;
        }
       
        /*if (RateControlParams.rateControlMode == NV_ENC_PARAMS_RC_CBR)
        {
            initializeParams.encodeConfig->encodeCodecConfig.h264Config.enableFillerDataInsertion = 1;
        }*/
        #if 0

        //initializeParams.encodeConfig->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
        //initializeParams.tuningInfo 
        initializeParams.encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
        initializeParams.frameRateNum = fps;
        initializeParams.encodeConfig->rcParams.averageBitRate = 4000000;
        initializeParams.encodeConfig->rcParams.maxBitRate = 4000000;
       // initializeParams.encodeConfig->rcParams.enableMinQP = true;
      //  initializeParams.encodeConfig->rcParams.enableMaxQP = true;
      //  initializeParams.encodeConfig->rcParams.minQP = 1;
      //  initializeParams.encodeConfig->rcParams.maxQP = 51;
       // initializeParams.encodeConfig->rcParams.constQP = {};
        initializeParams.encodeConfig->frameIntervalP = 3;
        //initializeParams.encodeConfig->rcParams.m = 4000000;
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.idrPeriod = 120;
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.chromaFormatIDC = 3;
        // pEncodeCLIOptions->SetInitParams(&initializeParams, NV_ENC_BUFFER_FORMAT_NV12);
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.enableIntraRefresh = 1;
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.intraRefreshPeriod = IntraRefreshPeriodFrames;
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.intraRefreshCnt = IntraRefreshCountFrames;
        /*
         * Repeat SPS/PPS - sends sequence and picture parameter info with every IDR frame - maximum stabilisation of the stream when IDR is sent.
         */
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.repeatSPSPPS = 1;

        /*
         * Slice mode - set the slice mode to "entire frame as a single slice" because WebRTC implementation doesn't work well with slicing. The default slicing
         * mode produces (rarely, but especially under packet loss) grey full screen or just top half of it.
         */
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceMode = 0;
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceModeData = 0;

        /*
         * These put extra meta data into the frames that allow Firefox to join mid stream.
         */
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputFramePackingSEI = 1;
        initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputRecoveryPointSEI = 1;
        #endif 
        m_encoder->CreateEncoder(&initializeParams);
      //  NvEncoderCuda enc(cuContext, nWidth, nHeight, eFormat);
		//const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);


		// if (!codec) 
  //       {
  //         /// fprintf(stderr, "Codec '%s' not found\n", codec_name);
  //          //exit(1);
  //           WarnL << "Codec 'h264' not found\n";
  //           return false;
  //      }

  //      m_c = avcodec_alloc_context3(codec);
  //      if (!m_c) 
  //      {
  //          //fprintf(stderr, "Could not allocate video codec context\n");
  //          //exit(1);
  //          WarnL << "Could not allocate video codec context";
  //          return false;
  //      }



  //       /* put sample parameters */
  //      m_c->bit_rate = 400000;
  //      /* resolution must be a multiple of two */
  //      m_c->width = width;
  //      m_c->height = height;
  //      /* frames per second */
  //      m_c->time_base = { 1, fps };
  //      m_c->framerate = { fps, 1 };

  //      /* emit one intra frame every ten frames
  //       * check frame pict_type before passing frame
  //       * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
  //       * then gop_size is ignored and the output of encoder
  //       * will always be I frame irrespective to gop_size
  //       */
  //      m_c->gop_size = 10;
  //      m_c->max_b_frames = 1;
  //      m_c->pix_fmt = AV_PIX_FMT_YUV420P;



  //      if (codec->id == AV_CODEC_ID_H264)
  //      {
  //          av_opt_set(m_c->priv_data, "preset", "slow", 0);
  //      }

  //      /* open it */
  //      int32_t ret = avcodec_open2(m_c, codec, NULL);
  //      if (ret < 0) {
  //        /*  fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
  //          exit(1);*/

  //          WarnL << "Could not open codec: ";//
  //        //<< av_err2str(ret);
  //      }

		return true;


#elif defined(__GNUC__)

return true;
#else
// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö§ï¿½ÖµÄ±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½Ô¼ï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
return false;
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
	}
     static const int32_t DEFAULT_BIT = 1000;
    bool cVideoEncoder::init(const dsp::ctranscode_info& info)
    {
#ifdef _MSC_VER
        encoder_params.tuningInfo = cv::cudacodec::EncodeTuningInfo::ENC_TUNING_INFO_LOSSLESS;
       // encoder_params.encodingProfile = cv::cudacodec::EncodeProfile::ENC_H264_PROFILE_MAIN;
        encoder_params.tuningInfo = cv::cudacodec::EncodeTuningInfo::ENC_TUNING_INFO_LOSSLESS;
       encoder_params.rateControlMode = cv::cudacodec::EncodeParamsRcMode::ENC_PARAMS_RC_CONSTQP;
        encoder_params.videoFullRangeFlag = true;
        // encoder_params.constQp = {28, 31, 25};
        /* encoder_params.averageBitRate = 8000 * 1000;
         encoder_params.maxBitRate = 10000 * 1000;*/
         //  encoder_params.multiPassEncoding = cv::cudacodec::EncodeMultiPass::ENC_TWO_PASS_FULL_RESOLUTION;
         //  encoder_params.videoFullRangeFlag = true;
       
        encoder_params.averageBitRate = info.get_average_bitrate() * DEFAULT_BIT;
        encoder_params.maxBitRate = info.get_max_bitrate() * DEFAULT_BIT;
        encoder_params.gopLength = info.get_gop();
        encoder_params.idrPeriod = info.get_gop();
      /*  encoder_params.constQp.qpInterB = info.get_p_qp();
        encoder_params.constQp.qpInterP = info.get_p_qp();
        encoder_params.constQp.qpIntra = info.get_i_qp();*/
        m_fps = info.get_fps();
        if (info.get_codec() == mediakit::CodecH264)
        {
            m_codec = cv::cudacodec::Codec::H264;
        }
        else if (info.get_codec() == mediakit::CodecH265)
        {
            m_codec = cv::cudacodec::Codec::HEVC;
        }
        else
        {
            m_codec = cv::cudacodec::Codec::H264;
        }


        if (info.get_rate_controlmode() == 0)
        {
            encoder_params.rateControlMode = cv::cudacodec::EncodeParamsRcMode::ENC_PARAMS_RC_CONSTQP;
        }
        else if (info.get_rate_controlmode() == 1)
        {
            encoder_params.rateControlMode = cv::cudacodec::EncodeParamsRcMode::ENC_PARAMS_RC_VBR;
        }
        else if (info.get_rate_controlmode() == 2)
        {
            encoder_params.rateControlMode = cv::cudacodec::EncodeParamsRcMode::ENC_PARAMS_RC_CBR;
        }


        return true;
#elif defined(__GNUC__)
        //video_info_param   video_param;
        m_param.width = info.get_width();
        m_param.height = info.get_height();
       // video_param.height = height;
        m_fps = info.get_fps();
         
        if (info.get_codec() == mediakit::CodecH264)
        {
            m_codec = MPP_VIDEO_CodingAVC;
        }
        else if (info.get_codec() == mediakit::CodecH265)
        {
            m_codec = MPP_VIDEO_CodingHEVC;
        }
        else
        {
            m_codec = MPP_VIDEO_CodingAVC;
        }
        // default  rk3588 NV12
        m_param.fmt = 0;// MPP_FMT_YUV420SP_VU;
        m_param.input_fps = 25;
        m_param.output_fps = 25;
        m_param.type = m_codec;
        m_param.avg_bitrate = info.get_average_bitrate() * DEFAULT_BIT;
        m_param.max_bitrate = info.get_max_bitrate() * DEFAULT_BIT;
        m_param.min_bitrate = info.get_average_bitrate() * DEFAULT_BIT;
        m_param.gop_size = info.get_gop();
        m_param.rc_mode = MPP_ENC_RC_MODE_CBR;
        //m_encoder.init(m_codec, video_param);
        /*
        video_param.width = width;
				video_param.height = height;
				static const int32_t   default_bitrate = 1000;
				video_param.avg_bitrate = 500 * default_bitrate;
				video_param.min_bitrate = 500 * default_bitrate;
				video_param.max_bitrate = 500 * default_bitrate;
				video_param.gop_size = 250;
				video_param.fmt = fmt;
				video_param.input_fps = 25;
				video_param.output_fps = 25;
				video_param.rc_mode = MPP_ENC_RC_MODE_CBR;
        */

        return true;
#else
        // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö§ï¿½ÖµÄ±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½Ô¼ï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
        return false;
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
        return false;
    }

    void cVideoEncoder::destroy()
    {
    }


    void cVideoEncoder::encode(
        const uint8_t *data, int32_t size, int64_t pts, std::vector<std::vector<uint8_t>> &vPacket,
        std::vector<uint64_t> &ptss /*AVFrame *frame*/ /*, std::shared_ptr<mediakit::FrameDispatcher> Interface*/)
    {
        int32_t ret = 0;

#ifdef _MSC_VER
        
            const NvEncInputFrame *encoderInputFrame = m_encoder->GetNextInputFrame();
            NvEncoderCuda::CopyToDeviceFrame(
                (CUcontext)m_encoder->GetDevice(), (void *)data, 0, (CUdeviceptr)encoderInputFrame->inputPtr, (int)encoderInputFrame->pitch, m_encoder->GetEncodeWidth(),
                m_encoder->GetEncodeHeight(), CU_MEMORYTYPE_HOST, encoderInputFrame->bufferFormat, encoderInputFrame->chromaOffsets,
                encoderInputFrame->numChromaPlanes);
            NV_ENC_PIC_PARAMS params = { 0 };
            params.version = NV_ENC_PIC_PARAMS_VER;
            params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
            
            params.inputTimeStamp = (uint64_t)pts;
          
            params.frameIdx = (uint32_t)pts;
            
            m_encoder->EncodeFrame(vPacket, ptss, &params);
#elif defined(__GNUC__)

        return  ;
#else
        // ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö§ï¿½ÖµÄ±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Òªï¿½Ô¼ï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
        return  ;
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
           /* static FILE *out_file_ptr = NULL;
            if (!out_file_ptr)
            {
                out_file_ptr = fopen("test.h265", "wb+");
            }*/


           /*  if (out_file_ptr)
            {
                for (std::vector<uint8_t> p : vPacket)
                {
                    ::fwrite(&p[0], 1, p.size(), out_file_ptr);
                    ::fflush(out_file_ptr);
                }
                
            } */
//#if 0
//        
//        ret =  avcodec_send_frame(m_c, frame);
//        if (ret < 0) {
//           // fprintf(stderr, "Error sending a frame for encoding\n");
//            WarnL << "Error sending a frame for encoding\n";
//            return;
//        }
//        AVPacket* pkt = av_packet_alloc();
//        while (ret >= 0) {
//            ret = avcodec_receive_packet(m_c, pkt);
//            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
//            {
//                break;
//            }
//            else if (ret < 0) {
//               // fprintf(stderr, "Error during encoding\n");
//                WarnL << "Error during encoding\n";
//                break;
//               // exit(1);
//            }
//
//            printf("Write packet %3" PRId64 " (size=%5d)\n", pkt->pts, pkt->size);
//
//
//
//            auto frame1 = /*mediakit::*/FrameImp::create</*mediakit::*/H264Frame>();
//            frame1->_prefix_size = 4;
//            frame1->_dts = pkt->dts;
//            frame1->_pts = pkt->pts;
//            frame1->_buffer.assign("\x00\x00\x00\x01", 4); // 添加264头
//            frame1->_buffer.append((const char *)pkt->data, pkt->size);
//           /* if (Interface)
//            {
//                Interface->input_decoder_encoder_Frame(frame1);
//            }*/
//           // fwrite(pkt->data, 1, pkt->size, outfile);
//            av_packet_unref(pkt);
//          //  break;
//        }
//        av_packet_free(&pkt);
//        pkt = NULL;
//        #endif 
    }
 

   //static  std::shared_ptr<cv::cudacodec::VideoWriter> __createVideoWriter(std::shared_ptr <cv::cudacodec::EncoderCallback> callback, const cv::Size frameSize, const cv::cudacodec::Codec codec, const double fps, const cv::cudacodec::ColorFormat colorFormat,
   //     const cv::cudacodec::EncoderParams& params)
   // {
   //     /*CV_Assert(params.idrPeriod >= params.gopLength);
   //     static  std::shared_ptr <cv::cudacodec::EncoderCallback> encoderCallback;*/
   //    // cv::cuda::Stream  stream ;
   //    // encoderCallback.reset(new RawVideoWriter(fileName));
   //    cv::cuda::Stream stream = cv::cuda::Stream::Null();
   //     return std::make_shared<dsp::VideoWriterImpl>(callback, frameSize, codec, fps, colorFormat, params, stream);
   // }
#ifdef _MSC_VER
    void cVideoEncoder::encode(const cv::Mat& frame, int64_t pts, cv::cudacodec::EncoderCallback * callback)
    {
       // InfoL << "";
 
        if (!writer)
        {
            cv::cuda::Stream stream = cv::cuda::Stream::Null();
            writer = std::make_shared<dsp::VideoWriterImpl>(callback, frame.size(), m_codec, m_fps, writerColorFormat, encoder_params, stream);
             // writer =   cv::cudacodec::__createVideoWriter(callback, frame.size(), codec, fps, writerColorFormat, encoder_params);
        }
      //  InfoL << "";
       // frame.download(frameFromDevice);
        writer->write(frame, pts);
 
    }
    void cVideoEncoder::encode(const cv::cuda::GpuMat& frame, int64_t pts, cv::cudacodec::EncoderCallback* callback)
    {

        if (!writer)
        {
            cv::cuda::Stream stream = cv::cuda::Stream::Null();
            writer = std::make_shared<dsp::VideoWriterImpl>(callback, frame.size(), m_codec, m_fps, writerColorFormat, encoder_params, stream);
            // writer =   cv::cudacodec::__createVideoWriter(callback, frame.size(), codec, fps, writerColorFormat, encoder_params);
        }

        // frame.download(frameFromDevice);
        writer->write(frame, pts);

    }
#elif defined(__GNUC__)
    bool cVideoEncoder::encode(MppFrame& frame, int64_t pts, dsp::EncoderCallback* callback)
    {
        if (!frame)
        {
            return false;
        }
        if (!m_encoder)
        {
            
            m_encoder = new dsp::crockchip_encoder();
            RK_U32 width = mpp_frame_get_width(frame);
            RK_U32 height = mpp_frame_get_height(frame);
            MppFrameFormat fmt = mpp_frame_get_fmt(frame);
            m_param.width = width;
            m_param.height = height;
            m_param.fmt = 0;// fmt; //VDPP_FMT_NV12
            m_encoder->init((MppCodingType)m_param.type, m_param);
            m_encoder->set_encoder(callback);
        }
        m_encoder->push(frame, pts);
        return true;
    }
#else
        
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
} // namespace chen

 
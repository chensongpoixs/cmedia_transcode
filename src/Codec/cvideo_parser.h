/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_VIDEO_PARSER_H_
#define _C_VIDEO_PARSER_H_

#include <nvcuvid.h>
#include <opencv2/cudacodec.hpp>
#include <memory>
#include "cvideo_decoder.h"
#include "cframe_queue.h"
#include "ccuda_define.h"
namespace  dsp
{
	class  cvideo_parser 
	{
	public:
        cvideo_parser(std::shared_ptr< cvideo_decoder>/*VideoDecoder**/ videoDecoder, std::shared_ptr< cframe_queue> frameQueue, const bool allowFrameDrop, const bool udpSource)
			/*:
			videoDecoder_(videoDecoder), frameQueue_(frameQueue), allowFrameDrop_(allowFrameDrop)*/
		: m_unparse_pakcets(0)
		, m_max_unparsed_packets(20)
		, m_current_frame_packets()
		, m_has_error(false)
		, m_allow_frame_drop(false)
		, m_video_decder(videoDecoder)
		, m_frame_queue(frameQueue)
		, m_parser(NULL){
			if (udpSource) {
				m_max_unparsed_packets = 0;
			}
			CUVIDPARSERPARAMS params;
			std::memset(&params, 0, sizeof(CUVIDPARSERPARAMS));

			params.CodecType = videoDecoder->codec();
			params.ulMaxNumDecodeSurfaces = 1;
			params.ulMaxDisplayDelay = 1; // this flag is needed so the parser will push frames out to the decoder as quickly as it can
			params.pUserData = this;
			params.pfnSequenceCallback = HandleVideoSequence;    // Called before decoding frames and/or whenever there is a format change
			params.pfnDecodePicture = HandlePictureDecode;    // Called when a picture is ready to be decoded (decode order)
			params.pfnDisplayPicture = HandlePictureDisplay;   // Called whenever a picture is ready to be displayed (display order)

			cudaSafeCall(cuvidCreateVideoParser(&m_parser, &params));
		}
        virtual ~cvideo_parser() {}


		
	public:
		bool parseVideoData(const unsigned char* data, size_t size, int64_t timestamp, const bool rawMode, const bool containsKeyFrame, bool endOfStream);

		bool hasError() const { return m_has_error; }

		bool udpSource() const { return  m_max_unparsed_packets == 0; }

		bool allowFrameDrops() const { return m_allow_frame_drop; }
	private:


		// Called when the decoder encounters a video format change (or initial sequence header)
		static int CUDAAPI HandleVideoSequence(void* pUserData, CUVIDEOFORMAT* pFormat);

		// Called by the video parser to decode a single picture
		// Since the parser will deliver data as fast as it can, we need to make sure that the picture
		// index we're attempting to use for decode is no longer used for display
		static int CUDAAPI HandlePictureDecode(void* pUserData, CUVIDPICPARAMS* pPicParams);

		// Called by the video parser to display a video frame (in the case of field pictures, there may be
		// 2 decode calls per 1 display call, since two fields make up one frame)
		static int CUDAAPI HandlePictureDisplay(void* pUserData, CUVIDPARSERDISPINFO* pPicParams);
	private:
		int32_t m_unparse_pakcets;
		int32_t m_max_unparsed_packets;
		std::vector<RawPacket>  m_current_frame_packets;
		volatile  bool m_has_error;
		bool m_allow_frame_drop;

		std::shared_ptr<cvideo_decoder> m_video_decder;
		std::shared_ptr<cframe_queue>   m_frame_queue;
		CUvideoparser  m_parser;



		//int32_t 

       // CUvideosource m_video_source;
       // cv::cudacodec::FormatInfo m_format;
	};
}



#endif // _C_VIDEO_PARSER_H_
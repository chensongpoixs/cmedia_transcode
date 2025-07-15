/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/
 #include "cvideo_parser.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
namespace dsp {
#define max(x, y)   (x) > (y) ? (x) : (y)
#define min(x, y)   (x) > (y)? (y): (x)

	bool cvideo_parser::parseVideoData(const unsigned char* data, size_t size, int64_t timestamp,  const bool rawMode, const bool containsKeyFrame, bool endOfStream)
	{
        CUVIDSOURCEDATAPACKET packet;
        std::memset(&packet, 0, sizeof(CUVIDSOURCEDATAPACKET));

        packet.flags = CUVID_PKT_TIMESTAMP;
        if (endOfStream)
            packet.flags |= CUVID_PKT_ENDOFSTREAM;
      //  packet.flags = flags | CUVID_PKT_TIMESTAMP;
        packet.timestamp = timestamp;
        
        packet.payload_size = static_cast<unsigned long>(size);
        packet.payload = data;

        if (rawMode)
        {
            m_current_frame_packets.push_back(RawPacket(data, size, containsKeyFrame));
        }

        CUresult retVal = CUDA_SUCCESS;
        try {
            retVal = cuvidParseVideoData(m_parser, &packet);
        }
        catch (const std::exception& e) {
            //CV_LOG_ERROR(NULL, e.msg);
            m_has_error = true;
            m_frame_queue->endDecode();
            return false;
        }

        if (retVal != CUDA_SUCCESS) {
            m_has_error = true;
            m_frame_queue->endDecode();
            return false;
        }
        //return true;
        ++m_unparse_pakcets;
        if (m_max_unparsed_packets && m_unparse_pakcets > m_max_unparsed_packets)
        {
            printf("Maxium number of packets ( %u) parsed without decoding a frame or reconfiguring the decoder, if reading from \
            a live source consider initializing with VideoReaderInitParams::udpSource == true.", m_max_unparsed_packets);
            m_has_error = true;
            m_frame_queue->endDecode();
            return false;
        }

        if (endOfStream)
        {
            m_frame_queue->endDecode();
        }

        return !m_frame_queue->isEndOfDecode();
	}
	int CUDAAPI cvideo_parser::HandleVideoSequence(void* pUserData, CUVIDEOFORMAT* format)
	{
        cvideo_parser* thiz = static_cast<cvideo_parser*>(pUserData);

        thiz->m_unparse_pakcets = 0;

        cv::cudacodec:: FormatInfo newFormat;
        newFormat.videoFullRangeFlag = format->video_signal_description.video_full_range_flag;
        newFormat.colorSpaceStandard = static_cast<cv::cudacodec::ColorSpaceStandard>(format->video_signal_description.matrix_coefficients);
        newFormat.codec = static_cast<cv::cudacodec::Codec>(format->codec);
        newFormat.chromaFormat = static_cast<cv::cudacodec::ChromaFormat>(format->chroma_format);
        newFormat.nBitDepthMinus8 = format->bit_depth_luma_minus8;
        newFormat.nBitDepthChromaMinus8 = format->bit_depth_chroma_minus8;
        newFormat.ulWidth = format->coded_width;
        newFormat.ulHeight = format->coded_height;
        newFormat.fps = format->frame_rate.numerator / static_cast<float>(format->frame_rate.denominator);
        newFormat.targetSz = thiz->m_video_decder->getTargetSz();
        newFormat.srcRoi = thiz->m_video_decder->getSrcRoi();
        if (newFormat.srcRoi.empty()) {
            newFormat.displayArea = cv::Rect(cv::Point(format->display_area.left, format->display_area.top), cv::Point(format->display_area.right, format->display_area.bottom));
            if (newFormat.targetSz.empty())
                newFormat.targetSz = cv::Size((format->display_area.right - format->display_area.left), (format->display_area.bottom - format->display_area.top));
        }
        else
        {
            newFormat.displayArea = newFormat.srcRoi;
        }
        //newFormat.displayArea = Rect(format->display_area.left, format->display_area.top, format->display_area.right, format->display_area.bottom);// Rect(Point(format->display_area.left, format->display_area.top), Point(format->display_area.right, format->display_area.bottom));
        newFormat.width = newFormat.targetSz.width ? newFormat.targetSz.width : format->coded_width;
        newFormat.height = newFormat.targetSz.height ? newFormat.targetSz.height : format->coded_height;
        newFormat.targetRoi = thiz->m_video_decder->getTargetRoi();
        newFormat.ulNumDecodeSurfaces = min((!thiz->m_allow_frame_drop) ? max(thiz->m_video_decder->nDecodeSurfaces(), static_cast<int>(format->min_num_decode_surfaces)) :
            (format->min_num_decode_surfaces * 2), 32);
        if (format->progressive_sequence)
        {
            newFormat.deinterlaceMode = cv::cudacodec::Weave;
        }
        else
        {
            newFormat.deinterlaceMode = cv::cudacodec::Adaptive;
        }
        int maxW = 0, maxH = 0;
        // AV1 has max width/height of sequence in sequence header
        if (format->codec == cudaVideoCodec_AV1 && format->seqhdr_data_length > 0)
        {
            CUVIDEOFORMATEX* vidFormatEx = (CUVIDEOFORMATEX*)format;
            maxW = vidFormatEx->av1.max_width;
            maxH = vidFormatEx->av1.max_height;
        }
        if (maxW < (int)format->coded_width)
            maxW = format->coded_width;
        if (maxH < (int)format->coded_height)
            maxH = format->coded_height;
        newFormat.ulMaxWidth = maxW;
        newFormat.ulMaxHeight = maxH;
        newFormat.enableHistogram = thiz->m_video_decder->enableHistogram();

       // thiz->m_frame_queue->waitUntilEmpty();
        int retVal = newFormat.ulNumDecodeSurfaces;
        /*if (thiz->m_video_decder->inited()) {
            retVal = thiz->m_video_decder->reconfigure(newFormat);
            if (retVal > 1 && newFormat.ulNumDecodeSurfaces != thiz->m_frame_queue->getMaxSz())
                thiz->m_frame_queue->resize(newFormat.ulNumDecodeSurfaces);
        }
        else*/
        {
            thiz->m_frame_queue->init(newFormat.ulNumDecodeSurfaces);
            thiz->m_video_decder->create(newFormat);
        }
        return retVal;
	}
	int CUDAAPI cvideo_parser::HandlePictureDecode(void* userData, CUVIDPICPARAMS* picParams)
	{
        cvideo_parser* thiz = static_cast<cvideo_parser*>(userData);

        thiz->m_unparse_pakcets = 0;

        bool isFrameAvailable = thiz->m_frame_queue->waitUntilFrameAvailable(picParams->CurrPicIdx, thiz->m_allow_frame_drop);
        if (!isFrameAvailable)
        {
            return false;
        }

        if (!thiz->m_video_decder->decodePicture(picParams))
        {
            printf("%s\n", "Decoding failed!");
            thiz->m_has_error = true;
            return false;
        }

        return true;
	}
	int CUDAAPI cvideo_parser::HandlePictureDisplay(void* userData, CUVIDPARSERDISPINFO* picParams)
	{
        cvideo_parser* thiz = static_cast<cvideo_parser*>(userData);

        thiz->m_unparse_pakcets = 0;
       
        
        thiz->m_frame_queue->enqueue(picParams, thiz->m_current_frame_packets);
        thiz->m_current_frame_packets.clear();
        return true;
	}
}

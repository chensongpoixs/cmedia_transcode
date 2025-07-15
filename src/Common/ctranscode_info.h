/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_TRANSCODE_INFO_H_
#define _C_TRANSCODE_INFO_H_
#include <iostream>
//#include "Common/Frame.h"
#include "Extension/Frame.h"
namespace  dsp
{
	

	 


	class ctranscode_info
	{
	public:
		explicit ctranscode_info() 
			: m_open(false)
			, m_width(1920)
			, m_height(1080)
			, m_fps(25)
			, m_codec(mediakit::CodecH264)
			, m_rate_controlmode(0)
			, m_profile(0)
			, m_gop(250)
			, m_I_QP(26)
			, m_P_QP(36)
			, m_average_bitrate(300)
			, m_max_bitrate(500)
		{}
		virtual ~ctranscode_info() {}
	
	public:
		void set_open(bool value);
		bool get_open()  const  { return m_open; } 

		void set_width(int32_t value);
		int32_t get_width() const  {
			return m_width;
		} 

		void set_height(int32_t value);
		int32_t get_height() const { return m_height; }


		void set_fps(int32_t fps);
		int32_t get_fps()  const  { return m_fps; }
		
		void set_codec(mediakit::CodecId codec);
		mediakit::CodecId get_codec()  const  { return  m_codec; }



		void set_rate_controlmode(int32_t value);
		int32_t get_rate_controlmode() const  { return  m_rate_controlmode; }


		void set_profile(int32_t value);
		int32_t get_profile()  const  { return  m_profile; }

		void set_gop(int32_t value);
		int32_t get_gop()  const  { return  m_gop; }


		void set_i_qp(int32_t value);
		int32_t get_i_qp()  const  { return  m_I_QP; }


		//void set_p_qp(int32_t value);
		//int32_t get_p_qp() /*const  { return  m_P_QP; }


		void set_p_qp(int32_t value);
		int32_t get_p_qp()  const  { return  m_P_QP; }


		void set_average_bitrate(int32_t value);
		int32_t get_average_bitrate()  const  { return  m_average_bitrate; }

		void set_max_bitrate(int32_t value);
		int32_t get_max_bitrate()  const { return  m_max_bitrate; }


		std::string toString()  const ;
	private:
		bool		 m_open;
		int32_t      m_width;
		int32_t      m_height;
		int32_t      m_fps;
		//cv::cudacodec::Codec  
		mediakit::CodecId   m_codec;
	 

		int32_t     m_rate_controlmode;
		int32_t     m_profile;
		int32_t     m_gop;
		int32_t     m_I_QP;
		int32_t     m_P_QP;
		int32_t     m_average_bitrate;
		int32_t     m_max_bitrate;

	};
}

#endif // _C_TRANSCODE_INFO_MGR_H_
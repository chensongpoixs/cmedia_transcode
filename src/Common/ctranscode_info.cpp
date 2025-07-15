/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

 
#include "ctranscode_info.h"


namespace  dsp
{


 
	void ctranscode_info::set_open(bool value)
	{
		m_open = value;
	}

	void ctranscode_info::set_width(int32_t value)
	{
		m_width = value;
	}

	void ctranscode_info::set_height(int32_t value)
	{
		m_height = value;
	}

	void ctranscode_info::set_fps(int32_t fps)
	{
		m_fps = fps;
	}

	void ctranscode_info::set_codec(mediakit::CodecId codec)
	{
		m_codec = codec;
	}

	void ctranscode_info::set_rate_controlmode(int32_t value)
	{
		m_rate_controlmode = value;
	}

	void ctranscode_info::set_profile(int32_t value)
	{
		m_profile = value;
	}

	void ctranscode_info::set_gop(int32_t value)
	{
		m_gop = value;
	}

	void ctranscode_info::set_i_qp(int32_t value)
	{
		m_I_QP = value;
	}

	void ctranscode_info::set_p_qp(int32_t value)
	{
		m_P_QP = value;
	}

	void ctranscode_info::set_average_bitrate(int32_t value)
	{
		m_average_bitrate = value;
	}

	void ctranscode_info::set_max_bitrate(int32_t value)
	{
		m_max_bitrate = value;
	}

	std::string ctranscode_info::toString() const
	{
		std::ostringstream  cmd;

		/*
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
		int32_t     m_max_bitrate;*/



		cmd << "[open = " << m_open << "]"
			<< "[width = " << m_width << "]"
			<< "[height = " << m_height << "]"
			<< "[fps = " << m_fps << "]"
			<< "[codec = " << m_codec << "]"
			<< "[controlmode = " << m_rate_controlmode << "]"
			<< "[profile = " << m_profile << "]"
			<< "[gop = " << m_gop << "]"
			<< "[I qp = " << m_I_QP << "]"
			<< "[p QP = " << m_P_QP << "]"
			<< "[average bitrate = " << m_average_bitrate << "]"
			<< "[max bitrate =" << m_max_bitrate << "]";

		return cmd.str();
	}

}
 
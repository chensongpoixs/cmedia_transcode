/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/
#include "ctranscode_info_mgr.h"

namespace dsp
{



	ctranscode_info_mgr& ctranscode_info_mgr::Instance()
	{
		// TODO: 在此处插入 return 语句
		static ctranscode_info_mgr instance;
		return instance;
	}

	bool   ctranscode_info_mgr::save_transcode_info(const mediakit::MediaTuple& tuple, const ctranscode_info& info)
	{

		clock_guard  lock(m_all_transcode_info_lock);

		MTRANSCODE_INFO_MAP::iterator iter =  m_all_transcode_info[tuple.app].find(tuple.stream);
		if (iter != m_all_transcode_info[tuple.app].end())
		{
			//
			WarnL << "[app = "<< tuple.app <<"][ stream = " << tuple.stream <<"]old transcode info = " << iter->second.toString();
		}
		m_all_transcode_info[tuple.app][tuple.stream] = info;
		InfoL << "[app = " << tuple.app << "][ stream = " << tuple.stream << "]new transcode info = " << info.toString();
		return true;
	}

	const ctranscode_info* ctranscode_info_mgr::find(const mediakit::MediaTuple& tuple)  
	{
		clock_guard  lock(m_all_transcode_info_lock);

		MTRANSCODE_INFO_MAP::const_iterator iter = m_all_transcode_info[tuple.app].find(tuple.stream);
		if (iter != m_all_transcode_info[tuple.app].end())
		{
			return &iter->second;
		}
		return nullptr;
	}
}
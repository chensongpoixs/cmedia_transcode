/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_TRANSCODE_INFO_MGR_H_
#define _C_TRANSCODE_INFO_MGR_H_

#include "ctranscode_info.h"
#include <unordered_map>
#include <string>
#include "Common/MediaSink.h"
#include "Record/Recorder.h"
#include <mutex>
namespace  dsp
{



	class ctranscode_info_mgr
	{
	private:
		typedef    std::unordered_map<std::string, ctranscode_info>                                             MTRANSCODE_INFO_MAP;
		typedef    std::mutex																					clock_type;
		typedef    std::lock_guard<clock_type>																	clock_guard;
	public:
		explicit ctranscode_info_mgr()
		: m_all_transcode_info(){}
		virtual ~ctranscode_info_mgr() {}
	
	public:
		static ctranscode_info_mgr& Instance();
	public:


		bool   save_transcode_info(const mediakit::MediaTuple& tuple, const ctranscode_info & info);

		const ctranscode_info* find(const mediakit::MediaTuple& tuple)   ;

	private:
		clock_type										m_all_transcode_info_lock;
		std::unordered_map<std::string, MTRANSCODE_INFO_MAP>								m_all_transcode_info;
	};
}

#endif // _C_TRANSCODE_INFO_MGR_H_
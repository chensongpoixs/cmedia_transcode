/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/
#include "cframe_queue.h"
#include "Util/logger.h"
#ifdef _MSC_VER
namespace  dsp
{
	RawPacket::RawPacket(const unsigned char* data_, const size_t size, const bool containsKeyFrame_) :
		data(data_, data_ + size), containsKeyFrame(containsKeyFrame_) {};

	 

	cframe_queue::~cframe_queue()
	{
		InfoL << "--->";
	}

	bool cframe_queue::init(const int32_t max_size)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_is_frame_in_use) 
		{
			return false;
		}
		m_max_size = max_size;
		m_display_queue = std::vector<CUVIDPARSERDISPINFO>(m_max_size, CUVIDPARSERDISPINFO());
		m_is_frame_in_use = new volatile int[m_max_size];
		std::memset((void*)m_is_frame_in_use, 0, sizeof(*m_is_frame_in_use) * m_max_size);
	}
	void cframe_queue::destroy()
	{
		if (m_is_frame_in_use)
		{
			delete[] m_is_frame_in_use;
		}
		m_display_queue.clear();
		m_max_size = 0;
	}
	bool cframe_queue::resize(const int32_t new_size) 
	{
		if (new_size == m_max_size)
		{
			return true;
		}
		if (!m_is_frame_in_use)
		{
			return init(new_size);
			 ;
		}
		std::lock_guard<std::mutex> lock(m_lock);
		const int maxSzOld = m_max_size; 
		m_max_size = new_size;
		const auto displayQueueOld = m_display_queue;
		m_display_queue = std::vector<CUVIDPARSERDISPINFO>(m_max_size, CUVIDPARSERDISPINFO());
		for (int i = m_read_position; i < m_read_position + m_frames_in_queue; i++)
		{
			m_display_queue.at(i % m_display_queue.size()) = displayQueueOld.at(i % displayQueueOld.size());
		}
		const volatile int* const isFrameInUseOld = m_is_frame_in_use;
		m_is_frame_in_use = new volatile int[m_max_size];
		std::memset((void*)m_is_frame_in_use, 0, sizeof(*m_is_frame_in_use) * m_max_size);
		int32_t   max_min_size = (m_max_size > maxSzOld) ? maxSzOld : m_max_size;
		std::memcpy((void*)m_is_frame_in_use, (void*)isFrameInUseOld, sizeof(*isFrameInUseOld) * max_min_size);
		delete[] isFrameInUseOld;
		return true;
	}
 
	bool cframe_queue::waitUntilFrameAvailable(int32_t picture_index, const bool all_frame_drop )
	{
		while (isInUse(picture_index))
		{
			if (all_frame_drop && dequeueUntil(picture_index))
			{
				break;
			}
			// Decoder is getting too far ahead from display
			//Thread::sleep(1);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			if (isEndOfDecode())
			{
				return false;
			}

		}

		return true;
	}

	bool cframe_queue::waitUntilEmpty() 
	{
		while (m_frames_in_queue) {
			//Thread::sleep(1);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			if (isEndOfDecode())
				return false;
		}
		return true;
	}
	void cframe_queue::enqueue(const CUVIDPARSERDISPINFO* picParams, const std::vector<RawPacket> rawPackets) {
		m_is_frame_in_use[picParams->picture_index] = true;

		// Wait until we have a free entry in the display queue (should never block if we have enough entries)
		do
		{
			bool isFramePlaced = false;

			{
				std::lock_guard<std::mutex> lock(m_lock);

				if (m_frames_in_queue < m_max_size)
				{
					const int writePosition = (m_read_position + m_frames_in_queue) % m_max_size;
					m_display_queue.at(writePosition) = *picParams;
					for (const auto& rawPacket : rawPackets)
					{
						m_rawpacket_queue.push(rawPacket);
					}
					m_frames_in_queue++;
					isFramePlaced = true;
				}
			}

			if (isFramePlaced) // Done
			{
				break;
			}

			// Wait a bit
			//Thread::sleep(1);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} while (!isEndOfDecode());
	}
	bool cframe_queue::dequeue(CUVIDPARSERDISPINFO& displayInfo, std::vector<RawPacket>& rawPackets) {
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_frames_in_queue > 0)
		{
			int32_t entry = m_read_position;
			displayInfo = m_display_queue.at(entry);
			while (!m_rawpacket_queue.empty()) {
				rawPackets.push_back(m_rawpacket_queue.front());
				m_rawpacket_queue.pop();
			}
			m_read_position = (entry + 1) % m_max_size;
			m_frames_in_queue--;
			m_is_frame_in_use[displayInfo.picture_index] = 2;
			return true;
		}

		return false;
	}
	bool cframe_queue::dequeueUntil(const int32_t pictureIndex) 
	{
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_is_frame_in_use[pictureIndex] != 1)
		{
			return false;
		}
		for (int i = 0; i < m_frames_in_queue; i++) {
			const bool found = m_display_queue.at(m_read_position).picture_index == pictureIndex;
			m_is_frame_in_use[m_display_queue.at(m_read_position).picture_index] = 0;
			m_frames_in_queue--;
			m_read_position = (m_read_position + 1) % m_max_size;
			if (found)
			{
				return true;
			}
		}
		return false;
	}

}


 #endif
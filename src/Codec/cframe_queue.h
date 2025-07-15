/***********************************************************************************************
created: 		2025-06-15

author:			chensong

purpose:		nv_cuda_ decoder
************************************************************************************************/

#ifndef _C_FRAME_QUEUE_H_
#define _C_FRAME_QUEUE_H_
#include <queue>
#include <vector>
#include <mutex>
#include <nvcuvid.h>
namespace  dsp
{


	class RawPacket 
    {
    public:
        RawPacket(const unsigned char *_data, const size_t _size = 0, const bool _containsKeyFrame = false);
        const unsigned char *Data() const noexcept { return data.data(); }
        size_t Size() const noexcept { return data.size(); }
        bool ContainsKeyFrame() const noexcept { return containsKeyFrame; }

    private:
        std::vector<unsigned char> data;
        bool containsKeyFrame = false;
    };
	class  cframe_queue 
	{
		public:
            cframe_queue()
                : m_is_frame_in_use(NULL)
                , m_endof_decode(0)
                , m_frames_in_queue(0)
                , m_read_position(0)
                , m_display_queue()
                , m_max_size(0)
                , m_rawpacket_queue(){ }
           virtual ~cframe_queue() {}


        public:
           bool init(const int32_t max_size);
           bool resize(const int32_t new_size);
           void endDecode() { m_endof_decode = true; }
           bool isEndOfDecode() const { return m_endof_decode != 0; }


           bool waitUntilFrameAvailable(int32_t picture_index, const bool all_frame_drop =false);

           bool waitUntilEmpty();
           void enqueue(const CUVIDPARSERDISPINFO *picParams, const std::vector<RawPacket> rawPackets);
           bool dequeue(CUVIDPARSERDISPINFO &displayInfo, std::vector<RawPacket> &rawPackets);
           bool dequeueUntil(const int32_t pictureIndex);

           void releaseFrame(const CUVIDPARSERDISPINFO &picParams) { m_is_frame_in_use[picParams.picture_index] = 0; }

           int getMaxSz() { return m_max_size; }

       private:
           bool isInUse(int pictureIndex) const { return m_is_frame_in_use[pictureIndex] != 0; }

    public:
        private:
            std::mutex m_lock;
            volatile int32_t* m_is_frame_in_use;
            volatile int32_t m_endof_decode;
            int32_t m_frames_in_queue;
            int32_t m_read_position;
            std::vector<CUVIDPARSERDISPINFO> m_display_queue;
            int32_t m_max_size;
            std::queue<RawPacket> m_rawpacket_queue;
	};
	
}



#endif // _C_FRAME_QUEUE_H_
#ifndef _SHARED_VIDEO_FRAME_H_
#define _SHARED_VIDEO_FRAME_H_
#include "SharedMemory.hpp"
#include <chrono>
#include <thread>
#include <atomic>

namespace sjq {
	class SharedVideoFrame {
	public:
		static constexpr uint32_t MAX_BUF_SIZE = 1920 * 1080 * 4;

		enum pixel_format :int {
			RGB = 0,
			BGR = 1,
			RGBA = 2,
			BGRA = 3
		};

		struct VideoFrame {
			uint64_t timestamp = 0;
			uint32_t width = 0;
			uint32_t height = 0;
			pixel_format format;
			uint8_t buf[MAX_BUF_SIZE];
		};

	private:
		struct VideoFrameData {
			uint64_t frame_count = 0;
			VideoFrame video_frame[2];
		};

		std::string m_shmName;
		SharedMemory m_shm;
		VideoFrameData* m_pVideoFrameData = nullptr;
		std::thread m_thdNotify;
		HANDLE m_hNotifyEvent = NULL;
		HANDLE m_hQuitEvent = NULL;
		std::atomic_bool m_bQuit = false;

	public:
		SharedVideoFrame(const std::string& name) :m_shmName(name) {
			std::string strNotifyEvtName = m_shmName + "_event";
			m_hNotifyEvent = CreateEventA(nullptr, false, true, strNotifyEvtName.c_str());
			m_hQuitEvent = CreateEventA(nullptr, false, true, nullptr);
		}
		~SharedVideoFrame() {
			StopNotifyThread();
			if (m_thdNotify.joinable()) {
				m_thdNotify.join();
			}
			CloseHandle(m_hNotifyEvent);
			CloseHandle(m_hQuitEvent);
		}

		bool Create() {
			if (m_shm.Create(m_shmName, sizeof(VideoFrameData))) {
				m_pVideoFrameData = (VideoFrameData*)m_shm.GetPtr();
				return true;
			}
			return false;
		}

		bool Open() {
			if (m_shm.Open(m_shmName)) {
				m_pVideoFrameData = (VideoFrameData*)m_shm.GetPtr();
				return true;
			}
			return false;
		}

		uint64_t SetVideoFrame(uint32_t width, uint32_t height, pixel_format format, const uint8_t* data) {
			uint32_t data_size = width * 4 * height;
			if (format == pixel_format::RGB || format == pixel_format::BGR) {
				data_size = ((width * 3 + 3) & ~3) * height;
			}

			auto now = std::chrono::system_clock::now();
			auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

			auto frame_count = m_pVideoFrameData->frame_count + 1;
			auto w = frame_count & 0x1;

			m_pVideoFrameData->video_frame[w].width = width;
			m_pVideoFrameData->video_frame[w].height = height;
			m_pVideoFrameData->video_frame[w].format = format;
			m_pVideoFrameData->video_frame[w].timestamp = timestamp;
			memcpy(m_pVideoFrameData->video_frame[w].buf, data, data_size);

			m_pVideoFrameData->frame_count = frame_count;

			SetEvent(m_hNotifyEvent);

			return frame_count;
		}

		const VideoFrame* GetVideoFrame()const {
			auto r = m_pVideoFrameData->frame_count & 0x1;
			return &m_pVideoFrameData->video_frame[r];
		}

		void StartNotifyThread(const std::function<void(const VideoFrame*)>& notify) {
			HANDLE hEvtArray[2] = {
				m_hQuitEvent,
				m_hNotifyEvent
			};

			m_bQuit = false;
			m_thdNotify = std::thread([&]() {
				while (!m_bQuit) {
					DWORD dwWait = WaitForMultipleObjects(2, hEvtArray, FALSE, 1000);
					if (dwWait - WAIT_OBJECT_0 == 0) {
						//m_hQuitEvent
						break;
					}
					else if (dwWait - WAIT_OBJECT_0 == 1) {
						//m_hNotifyEvent
						auto r = m_pVideoFrameData->frame_count & 0x1;
						notify(&m_pVideoFrameData->video_frame[r]);
					}
				}
				});
		}
		void StopNotifyThread() {
			m_bQuit = true;
			SetEvent(m_hQuitEvent);
		}
	};
}

#endif//_SHARED_VIDEO_FRAME_H_

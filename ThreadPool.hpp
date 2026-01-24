#ifndef _THREAD_POOL_HPP_
#define _THREAD_POOL_HPP_

#include <thread>
#include <list>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace sjq {
	class ThreadPool {

		const uint32_t m_nMaxThreadNum = 4;
		std::atomic_uint32_t m_nBusyThreadNum = 0;
		std::vector<std::thread> m_allThreadQue;
		std::list <std::function<void()>> m_taskQue;

		std::atomic_bool m_Stop = false;
		std::mutex m_cvMtx;
		std::condition_variable m_cv;

		class Counter {
			std::atomic_uint32_t& m_nCounter;
		public:
			Counter(std::atomic_uint32_t& cnt) :m_nCounter(cnt) { ++m_nCounter; }
			~Counter() { --m_nCounter; }
		};

		void work() {
			while (!m_Stop) {
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock(m_cvMtx);
					m_cv.wait(lock, [this] { return m_Stop || !m_taskQue.empty(); });

					if (m_Stop)
						break;

					task = m_taskQue.front();
					m_taskQue.pop_front();
				}
				
				if (task) {
					Counter cntr(m_nBusyThreadNum);
					task();
				}
			}
		}

	public:
		ThreadPool() = default;
		explicit ThreadPool(uint32_t nMaxThreadNum):m_nMaxThreadNum(nMaxThreadNum){
		}
		~ThreadPool()
		{
			m_Stop = true;
			m_cv.notify_all();
			for (auto& thd : m_allThreadQue) {
				if (thd.joinable()) {
					thd.join();
				}
			}
		}

		void AddTask(std::function<void()> task) {
			if (m_Stop) {
				return;
			}
			std::unique_lock<std::mutex> lock(m_cvMtx);

			if (m_nBusyThreadNum.load(std::memory_order_relaxed) >= m_allThreadQue.size() 
				&& m_allThreadQue.size() < m_nMaxThreadNum) {
				m_allThreadQue.emplace_back(&ThreadPool::work, this);
			}

			m_taskQue.push_back(task);
			m_cv.notify_one();
		}

	};
}

#endif // !_THREAD_POOL_HPP_

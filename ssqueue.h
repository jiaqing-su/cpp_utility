#ifndef _SSQUEUE_H_
#define _SSQUEUE_H_

namespace lockfree
{
	template <typename T>
	class ssqueue
	{
		int m_head = 0;
		int m_tail = 0;
		T*  m_buf = nullptr;
		const int m_size = 0;
		const int m_data_size = sizeof(T);
	public:
		ssqueue(int size) :m_size(size)
		{
			m_buf = new T[size];
		}

		~ssqueue()
		{
			delete [] m_buf;
		}

		bool push(const T& data)
		{
			int new_head = (m_head + 1) % m_size;
			if (new_head == m_tail)
				return false;//full
			memcpy(m_buf + m_head, &data, m_data_size);
			m_head = new_head;
			return true;
		}

		bool pop(T& data)
		{
			if (m_head == m_tail)
				return false;//empty
			memcpy(&data, m_buf + m_tail, m_data_size);
			m_tail = (m_tail + 1) % m_size;
			return true;
		}
	};
}

#endif // !_SSQUEUE_H_

#ifndef _TIME_WHEEL_H_
#define _TIME_WHEEL_H_

template <typename T_ID, typename T_FLAG>
class time_wheel
{
	struct XTASK
	{
		T_ID id;
		T_FLAG flag;
		XTASK& operator = (const XTASK& r){
			id = r.id;
			flag = r.flag;
			return *this;
		}
	};
	class cycle_list
	{
		friend class time_wheel;
	public:
		cycle_list():m_idx_head(0),m_idx_tail(0),m_data(0){}
		~cycle_list(){}
		bool push(const XTASK& id)
		{
			unsigned idx = (m_idx_head+1)%m_size;
			if(idx == m_idx_tail)
				return false;
			m_data[m_idx_head] = id;
			m_idx_head = idx;
			return true;
		}
		bool pop(XTASK& id)
		{
			if(m_idx_head == m_idx_tail)
				return false;
			id = m_data[m_idx_tail];
			m_idx_tail = (m_idx_tail+1)%m_size;
			return true;
		}
	private:
		unsigned m_idx_head;
		unsigned m_idx_tail;
		unsigned m_size;
		XTASK*   m_data;
	};

private:
	time_wheel(const time_wheel&);
	time_wheel& operator = (const time_wheel&);

public:
	time_wheel(int tick_count, int channel_count, unsigned list_size = 1024)
		:m_cur_tick(0)
		,m_tick_count(tick_count)
		,m_channel_count(channel_count)
	{
		m_wheel = new cycle_list[tick_count*channel_count];
		m_wheel[0].m_data = new XTASK[tick_count*channel_count*list_size];
		m_wheel[0].m_size = list_size;

		for (int i = 1; i < tick_count*channel_count; i++)
		{
			m_wheel[i].m_data = m_wheel[0].m_data + i*list_size;
			m_wheel[i].m_size = list_size;
		}
	}

	virtual~time_wheel()
	{
		if(m_wheel)
		{
			if(m_wheel[0].m_data)
				delete [] m_wheel[0].m_data;
			delete[] m_wheel;
		}
	}

	bool add_task(T_ID id, T_FLAG flag, int channel, int time_out_ticks)
	{
		XTASK task = {id, flag};

		time_out_ticks = time_out_ticks<1?1:time_out_ticks;
		time_out_ticks = time_out_ticks>(m_tick_count-1)?(m_tick_count-1):time_out_ticks;
		time_out_ticks = (time_out_ticks + m_cur_tick)%m_tick_count;

		channel = channel%m_channel_count;
		
		return m_wheel[time_out_ticks*m_channel_count + channel].push(task);
	}

	void tick_once()
	{
		XTASK task;
		for (int ch = 0; ch < m_channel_count; ch++)
		{
			while(m_wheel[m_cur_tick*m_channel_count + ch].pop(task))
			{
				on_time_out(task.id, task.flag, ch);
			}
		}

		m_cur_tick = (m_cur_tick + 1)%m_tick_count;
	}

	virtual void on_time_out(T_ID id, T_FLAG flag, int channel)
	{
		cout<<"time out channel: "<<channel<<" id:"<<id<<" flag:"<<flag<<endl;
	}

private:
	cycle_list*  m_wheel;
	int          m_cur_tick;
	const int    m_tick_count;
	const int    m_channel_count;
};

#endif//_TIME_WHEEL_H_
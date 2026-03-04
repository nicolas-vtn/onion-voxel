#pragma once

#include <mutex>
#include <queue>
#include <utility>

template <typename T> class ThreadSafeQueue
{
  public:
	void Push(T value)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Queue.push(std::move(value));
	}

	bool TryPop(T& out)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Queue.empty())
			return false;

		out = std::move(m_Queue.front());
		m_Queue.pop();
		return true;
	}

  private:
	std::queue<T> m_Queue;
	std::mutex m_Mutex;
};

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

/// @brief A thread-safe queue implementation that allows multiple producers and consumers to safely push and pop items.
/// @tparam T The type of items stored in the queue.
template <typename T> class ThreadSafeQueue
{
	// ----- Constructor / Destructor -----
  public:
	ThreadSafeQueue() = default;
	~ThreadSafeQueue() = default;

	// ----- Public API -----
  public:
	/// @brief Pushes a new item into the queue. This method is thread-safe and can be called by multiple producers concurrently.
	/// @param value The item to be pushed into the queue.
	void Push(T value)
	{
		{
			std::lock_guard lock(m_Mutex);
			m_Queue.push(std::move(value));
		}

		m_Cv.notify_one();
	}

	/// @brief Attempts to pop an item from the queue without blocking.
	/// @param out The variable to store the popped item.
	/// @return True if an item was successfully popped, false if the queue was empty.
	bool TryPop(T& out)
	{
		std::lock_guard lock(m_Mutex);

		if (m_Queue.empty())
			return false;

		out = std::move(m_Queue.front());
		m_Queue.pop();

		return true;
	}

	/// @brief Pops an item from the queue, blocking until an item is available or a stop is requested.
	/// @param value The variable to store the popped item.
	/// @param stopToken The stop token to allow interruption of the wait.
	/// @return True if an item was successfully popped, false if the wait was interrupted by a stop request.
	bool WaitPop(T& value, std::stop_token stopToken)
	{
		std::unique_lock lock(m_Mutex);

		m_Cv.wait(lock, stopToken, [&] { return !m_Queue.empty(); });

		if (m_Queue.empty())
			return false;

		value = std::move(m_Queue.front());
		m_Queue.pop();

		return true;
	}

	/// @brief Gets the current size of the queue.
	size_t Size() const
	{
		std::lock_guard lock(m_Mutex);
		return m_Queue.size();
	}

	// ----- Private Members -----
  private:
	std::queue<T> m_Queue;
	std::mutex m_Mutex;
	std::condition_variable_any m_Cv;
};

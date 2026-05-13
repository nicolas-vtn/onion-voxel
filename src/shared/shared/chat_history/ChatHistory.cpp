#include "ChatHistory.hpp"

#include <mutex>

namespace onion::voxel
{
	ChatHistory::~ChatHistory()
	{
		std::unique_lock lockReceived(m_MutexReceived);
		m_Received.clear();

		std::unique_lock lockSent(m_MutexSent);
		m_Sent.clear();
	}

	void ChatHistory::AddReceived(std::shared_ptr<const ChatMessage> message)
	{
		std::unique_lock lock(m_MutexReceived);
		Push(m_Received, std::move(message));
	}

	void ChatHistory::AddSent(std::shared_ptr<const ChatMessage> message)
	{
		std::unique_lock lock(m_MutexSent);
		Push(m_Sent, std::move(message));
	}

	std::vector<std::shared_ptr<const ChatMessage>> ChatHistory::GetReceivedHistory() const
	{
		std::shared_lock lock(m_MutexReceived);
		return m_Received;
	}

	std::vector<std::shared_ptr<const ChatMessage>> ChatHistory::GetSentHistory() const
	{
		std::shared_lock lock(m_MutexSent);
		return m_Sent;
	}

	void ChatHistory::Push(std::vector<std::shared_ptr<const ChatMessage>>& vec, std::shared_ptr<const ChatMessage> msg)
	{
		vec.insert(vec.begin(), std::move(msg));
		if (vec.size() > MaxMessages)
			vec.pop_back();
	}
} // namespace onion::voxel

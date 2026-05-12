#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>

#include "ChatMessage.hpp"

namespace onion::voxel
{
	class ChatHistory
	{
		// ----- Public API -----
	  public:
		static constexpr size_t MaxMessages = 1000;

		void AddReceived(std::shared_ptr<const ChatMessage> message);
		void AddSent(std::shared_ptr<const ChatMessage> message);

		std::vector<std::shared_ptr<const ChatMessage>> GetReceivedHistory() const;
		std::vector<std::shared_ptr<const ChatMessage>> GetSentHistory() const;

		// ----- Internals -----
	  private:
		static void Push(std::vector<std::shared_ptr<const ChatMessage>>& vec, std::shared_ptr<const ChatMessage> msg);

		mutable std::shared_mutex m_MutexReceived;
		std::vector<std::shared_ptr<const ChatMessage>> m_Received;

		mutable std::shared_mutex m_MutexSent;
		std::vector<std::shared_ptr<const ChatMessage>> m_Sent;
	};
} // namespace onion::voxel

#pragma once

#include "Participant.hpp"
#include "ChatRoom.hpp"
#include <boost/asio.hpp>

class ChatSession : public Participant, public std::enable_shared_from_this<ChatSession>// 
{
public:
	/*static std::shared_ptr<ChatSession> create(boost::asio::ip::tcp::socket socket, ChatRoom& room)
	{
		return std::make_shared<ChatSession>(std::move(socket), room);

	}*/
	ChatSession(boost::asio::ip::tcp::socket socket, ChatRoom& room);
	~ChatSession();
	void start();
	void deliver(const std::string& msg) override;
private:
	void do_read();
	boost::asio::ip::tcp::socket socket_;
	ChatRoom& room_;
	boost::asio::streambuf buffer_;
};
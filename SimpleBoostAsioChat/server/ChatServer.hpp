#pragma once

#include "ChatRoom.hpp"
#include <boost/asio.hpp>


class ChatServer 
{
public:
	ChatServer(boost::asio::io_context& io_context, short port);
private:
	void do_accept();
	boost::asio::ip::tcp::acceptor acceptor_;
	ChatRoom room_;
};
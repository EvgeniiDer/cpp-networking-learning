#include "ChatServer.hpp"
#include "ChatSession.hpp"


ChatServer::ChatServer(boost::asio::io_context& io_context, short port) 
	: acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
	do_accept();
}

void ChatServer::do_accept()
{
	acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) 
		{
			if (!ec)
			{
				std::make_shared<ChatSession>(std::move(socket), room_)->start();
			}
			do_accept();
		});
}


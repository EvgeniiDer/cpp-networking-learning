//#include "ChatSession.hpp"
//#include<iostream>
//
//ChatSession::ChatSession(boost::asio::ip::tcp::socket socket, ChatRoom& room) : socket_(std::move(socket)),
//room_(room)
//{
//
//}
//void ChatSession::start()
//{
//	room_.join(shared_from_this());// ������� shared_ptr �� ������� ������ � ����� join()
//
//	/* ���������:
//	 * 1. shared_from_this() �� ������ ����� ������, � ������������� �����������
//	 *    ����������� ���������� ������������ ��������
//	 *    - ���������� std::shared_ptr, ����������� �������� � ��� ������������ ����������
//	 *
//	 * 2. ������� ������:
//	 *    - ��� �������� ������� ����� std::make_shared()/std::shared_ptr()
//	 *      �������� ������ ������� ������
//	 *    - shared_from_this() �������� ������ � ����� �������� � ����������� ��� �� 1
//	 *
//	 * 3. ���������� � ������ ������:
//	 *    - ����� join() ��������� shared_ptr ��� ��������, ��� ������
//	 *      �� ����� �����, ���� ������� (room_) ��� ����������
//	 *    - ������������ (�������� shared_ptr �� this) ������� �� �:
//	 *      �) �������� ���������� ����� ��������
//	 *      �) �������� �������� (undefined behavior)
//	 *
//	 * 4. ����������� ����������:
//	 *    - ������ ��� ������ ���������� ��� ����������� shared_ptr
//	 *      (����� - ���������� std::bad_weak_ptr)
//	 *    - �������� ����� � ������������/����������� �������
//	 *    - ������ ������������ ��� ��������� ��������
//	 */
//	boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
//	std::cout << "[New member]" << endpoint.address().to_string() << " : " << endpoint.port() << " Entered in chat" << std::endl;
//	do_read();
//}
//void ChatSession::do_read()
//{
//	std::shared_ptr<ChatSession> self = shared_from_this();/* ���������:
// * 1. �� �� ������ ����� ������ ������ ChatSession
// * 2. �� �� ������ ������ �� "�������" ������
// * 3. �� ������ ����� shared_ptr, �������:
// *    - ��������� �� ��� ������������ ������ (this)
// *    - ��������� �������� ���� �������� � ������� shared_ptr
// *    - ����������� ������� ������ �� ������
// */
//	boost::asio::async_read_until(socket_, buffer_, '\n', [this, self](boost::system::error_code ec, std::size_t length) 
//		{
//			if (!ec) 
//				{
//					//std::istream is(&buffer_);
//					//std::istreambuf_iterator<char> beegin_iterator(is);
//					//std::istreambuf_iterator<char> end_iterator;
//					//std::string message(beegin_iterator,end_iterator); // �������� ��������, ��������� �� ���������
//				std::istream is(&buffer_);
//				std::string message;
//				std::getline(is, message);
//				// ������� ��� ����������� � ���������
//				boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
//				std::string full_message = "[" + endpoint.address().to_string() + ":" + std::to_string(endpoint.port()) + "]: " + message;
//
//				room_.broadcast(full_message);
//				do_read();
//			}
//			else {
//				boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
//				std::cout << "[Member Leave] "
//					<< endpoint.address().to_string() << ":" << endpoint.port() << " leave chat.\n";
//				room_.leave(shared_from_this());
//			}
//		});
//}
//void ChatSession::deliver(const std::string& msg)
//{
//	std::shared_ptr<std::string> shared_message = std::make_shared<std::string>(msg);
//	std::shared_ptr<ChatSession> self = shared_from_this();
//	boost::asio::async_write(socket_, boost::asio::buffer(*shared_message), [this, self](boost::system::error_code ec, std::size_t length) {
//		if (ec)
//		{
//			std::cerr << "Write Error: " << ec.message() << std::endl;
//	        //  �� ����� ���� ����� ������ ������ ��������� �� ��������� ������ ������� ��� �����
//			room_.leave(self);
//		}
//		});
//}
// ChatSession.cpp

#include "ChatSession.hpp"
#include <iostream>

ChatSession::ChatSession(boost::asio::ip::tcp::socket socket, ChatRoom& room)
	: socket_(std::move(socket)), room_(room) 
{
}


ChatSession::~ChatSession()
{
	
	std::cout << "[Debug] Session is being destroyed." << std::endl;
}

void ChatSession::start()
{
	
	room_.join(shared_from_this());

	boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
	std::cout << "[New member] " << endpoint.address().to_string() << ":" << endpoint.port() << " entered the chat." << std::endl;

	do_read();
}

void ChatSession::do_read()
{
	
	auto self = shared_from_this();

	boost::asio::async_read_until(socket_, buffer_, '\n',
		[this, self](boost::system::error_code ec, std::size_t )
		{
			if (!ec)
			{
				std::istream is(&buffer_);
				std::string message;
				std::getline(is, message); 
				
				if (message.empty())
				{
					do_read(); 
				}

				
				boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
				std::string full_message = "[" + endpoint.address().to_string() + ":" + std::to_string(endpoint.port()) + "]: " + message + "\n";

				
				room_.broadcast(full_message);

				
				do_read();
			}
			else
			{
				
				boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
				std::cout << "[Member Left] "
					<< endpoint.address().to_string() << ":" << endpoint.port() << " left the chat. Reason: " << ec.message() << std::endl;

				
				room_.leave(self);
			}
		});
}

void ChatSession::deliver(const std::string& msg)
{

	auto shared_msg = std::make_shared<std::string>(msg);

	
	auto self = shared_from_this();

	boost::asio::async_write(socket_, boost::asio::buffer(*shared_msg),
		[this, self, shared_msg](boost::system::error_code ec, std::size_t /*length*/)
		{
			if (ec)
			{
				std::cerr << "Write Error: " << ec.message() << ". Removing participant." << std::endl;
				
				room_.leave(self);
			}
		});
}
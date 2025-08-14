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
//	room_.join(shared_from_this());// Передаём shared_ptr на текущий объект в метод join()
//
//	/* Пояснение:
//	 * 1. shared_from_this() НЕ создаёт новый объект, а предоставляет возможность
//	 *    совместного управления существующим объектом
//	 *    - Возвращает std::shared_ptr, разделяющий владение с уже существующим указателем
//	 *
//	 * 2. Принцип работы:
//	 *    - При создании объекта через std::make_shared()/std::shared_ptr()
//	 *      создаётся единый счётчик ссылок
//	 *    - shared_from_this() получает доступ к этому счётчику и увеличивает его на 1
//	 *
//	 * 3. Назначение в данном случае:
//	 *    - Метод join() принимает shared_ptr для гарантии, что объект
//	 *      не будет удалён, пока комната (room_) его использует
//	 *    - Альтернатива (создание shared_ptr из this) привела бы к:
//	 *      а) Двойному управлению одним объектом
//	 *      б) Двойному удалению (undefined behavior)
//	 *
//	 * 4. Критические требования:
//	 *    - Объект уже должен находиться под управлением shared_ptr
//	 *      (иначе - исключение std::bad_weak_ptr)
//	 *    - Запрещён вызов в конструкторе/деструкторе объекта
//	 *    - Нельзя использовать для временных объектов
//	 */
//	boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();
//	std::cout << "[New member]" << endpoint.address().to_string() << " : " << endpoint.port() << " Entered in chat" << std::endl;
//	do_read();
//}
//void ChatSession::do_read()
//{
//	std::shared_ptr<ChatSession> self = shared_from_this();/* Пояснение:
// * 1. Мы НЕ создаём новый объект класса ChatSession
// * 2. Мы НЕ создаём ссылку на "будущий" объект
// * 3. Мы создаём новый shared_ptr, который:
// *    - Указывает на УЖЕ СУЩЕСТВУЮЩИЙ объект (this)
// *    - Разделяет владение этим объектом с другими shared_ptr
// *    - Увеличивает счётчик ссылок на объект
// */
//	boost::asio::async_read_until(socket_, buffer_, '\n', [this, self](boost::system::error_code ec, std::size_t length) 
//		{
//			if (!ec) 
//				{
//					//std::istream is(&buffer_);
//					//std::istreambuf_iterator<char> beegin_iterator(is);
//					//std::istreambuf_iterator<char> end_iterator;
//					//std::string message(beegin_iterator,end_iterator); // Конечный итератор, созданный по умолчанию
//				std::istream is(&buffer_);
//				std::string message;
//				std::getline(is, message);
//				// Добавим имя отправителя к сообщению
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
//	        //  По идеии если вышла ошибка значит участника не подключен значит удаляем его нахер
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
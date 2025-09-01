#include<zmq.hpp>
#include<iostream>
#include<string>
#include<chrono>
#include<thread>


int main()
while (true) {
	// Создаем сообщение
	std::string message = "Сообщение " + std::to_string(count++);

	// Отправляем сообщение
	zmq::message_t zmq_message(message.size());
	memcpy(zmq_message.data(), message.c_str(), message.size());

	publisher.send(zmq_message, zmq::send_flags::none);
	std::cout << "Отправлено: " << message << std::endl;

	// Ждем 1 секунду
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
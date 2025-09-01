#include<zmq.hpp>
#include<iostream>
#include<string>
#include<chrono>
#include<thread>


int main()
while (true) {
	// ������� ���������
	std::string message = "��������� " + std::to_string(count++);

	// ���������� ���������
	zmq::message_t zmq_message(message.size());
	memcpy(zmq_message.data(), message.c_str(), message.size());

	publisher.send(zmq_message, zmq::send_flags::none);
	std::cout << "����������: " << message << std::endl;

	// ���� 1 �������
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
#include<iostream>
#include"client.hpp"

int main()
{
	try {
		std::string host = "127.0.0.1";
		std::string port = "9090";


		std::cout << "Enter your name: ";
		std::string nickname;
		std::getline(std::cin, nickname);

		boost::asio::io_context io_context;
		boost::asio::ip::tcp::socket socket(io_context);
		boost::asio::ip::tcp::resolver resolver(io_context);
		boost::asio::connect(socket, resolver.resolve(host, port));
		std::cout << "Connected to the server! You can start typing." << std::endl;

		std::thread t([&io_context]() {
			io_context.run();
			});

		start_async_read(socket);

		std::string line;
		while (std::getline(std::cin, line))
		{
			if (line.empty())
				continue;
			line += "\n";
			boost::asio::write(socket, boost::asio::buffer(line));
		}
		socket.close();
		t.join();
	}
	catch (std::exception& ex)
	{
		std::cerr << "Cant connetct to the server." << std::endl;
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	return 0;
}
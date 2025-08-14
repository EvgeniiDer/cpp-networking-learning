#include"ChatServer.hpp"
#include<iostream>
#include<boost/asio.hpp>



int main(int argc, char** argv)
{
	try {
		const short  port = 9090;

		boost::asio::io_context io_context;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard =
		boost::asio::make_work_guard(io_context);
		ChatServer server(io_context, port);

		
		std::cout << "[SERVER] server is running and listening port: " << port << std::endl;
		
		io_context.run();

	}
	catch (std::exception& e)
	{
		std::cerr << "Critical error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}

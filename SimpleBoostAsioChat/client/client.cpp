#include"client.hpp"
boost::asio::streambuf read_bufer;
void start_async_read(boost::asio::ip::tcp::socket& socket)
{
	
	boost::asio::async_read_until(socket, read_bufer, '\n', [&socket](const boost::system::error_code ec, std::size_t bytes_transferred)
		{
			if (!ec)
			{
				std::istream is(&read_bufer);
				std::istreambuf_iterator<char>begin_iterator(is);
				std::istreambuf_iterator<char>end_iterator;
				std::string message(begin_iterator, end_iterator);
				
				std::cout << message << std::endl;

				start_async_read(socket);
			}
			else {
				std::cout << "Connection with [SERVER] lost" << std::endl;
				socket.close();
			}
		});
}

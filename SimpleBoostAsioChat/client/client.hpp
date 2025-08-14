#pragma once

#include<iostream>
#include<string>
#include<thread>
#include<boost/asio.hpp>

void start_async_read(boost::asio::ip::tcp::socket& socket);

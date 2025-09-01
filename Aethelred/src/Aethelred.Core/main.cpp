#include"AethelredCore.hpp"


#include<iostream>
int main(int argc, char** argv)
{
	const std::string gateway_address = "tcp://localhost:5555";

	const std::string core_publisher_address = "tcp://*:5556";

	try {
		AethelredCore core(gateway_address, core_publisher_address);
		core.run();
	}
	catch (const std::exception& e) {
		std::cerr << "A fatal error occurred in AethelredCore: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
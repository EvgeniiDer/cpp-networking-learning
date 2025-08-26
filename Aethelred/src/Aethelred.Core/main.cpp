#include"AethelredCore.hpp"


#include<iostream>
int main(int argc, char** argv)
{
	AethelredCore temp("tcp://*:5555");
	temp.run();
	return 0;
}
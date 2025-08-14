#pragma once

#include<string>
#include<memory>

//Interface

class Participant {
public:
	virtual ~Participant() = default;
	virtual void deliver(const std::string& msg) = 0;
private:

};
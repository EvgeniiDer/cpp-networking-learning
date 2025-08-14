#pragma once

#include"Participant.hpp"
#include<set>



class ChatRoom {
public:
	void join(std::shared_ptr<Participant> participant);
	void leave(std::shared_ptr<Participant> participant);
	void broadcast(const std::string& msg);
private:
	std::set <std::shared_ptr<Participant>> participants_;
};
#include"ChatRoom.hpp"

void ChatRoom::join(std::shared_ptr<Participant> participant)
{
	participants_.insert(participant);
}
void ChatRoom::leave(std::shared_ptr<Participant> participant)
{
	participants_.erase(participant);
}
void ChatRoom::broadcast(const std::string& msg)
{
	auto participants_copy = participants_;
	for (std::set<std::shared_ptr<Participant>>::const_iterator it = participants_copy.begin(); it != participants_copy.end(); ++it)
	{
		const std::shared_ptr<Participant>& participant = *it;
		participant->deliver(msg);
	}
}
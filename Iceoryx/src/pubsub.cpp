#include<iceoryx_posh/popo/publisher.hpp>
#include<iceoryx_posh/popo/subscriber.hpp>
#include<iceoryx_posh/runtime/posh_runtime.hpp>

#include"../../interface/pub_interface.hpp"
#include"../../interface/sub_interface.hpp"
#include"../../interface/ping_pong_interface.hpp"
#include<cstring>

#include<string>
#include<cstring>
#include<chrono>
#include<ctime>
#include<iostream>

struct Message{
	unsigned long int timestamp;
	short id;
	size_t len;
};

class Publisher: public TestMiddlewarePub{
private:
	std::string _name;
	iox::popo::Publisher* pub;
public:
	Publisher(std::string& topic,	
		  int msgCount, 
		  int prior, 
		  int cpu_index,
            	  int min_msg_size, 
		  int max_msg_size, 
		  int step, 
		  int interval, 
		  int msgs_before_step,
            	  std::string &filename, 
		  int topic_priority
			): TestMiddlewarePub(topic, msgCount, prior, cpu_index, min_msg_size,
			       max_msg_size, step, interval, msgs_before_step, 
			       filename, topic_priority)
	{
		iox::runtime::PoshRuntime::getInstance(std::string("/")+filename);
		char param[100];
		if(topic.length()<100) memcpy(param,topic.c_str(),topic.length()+1);
		else{
			memcpy(param,topic.c_str(),99);
			param[99]='\0';
		}
		pub=new iox::popo::Publisher({"Iceoryx",param});
		pub->offer();
	}

	~Publisher(){
		pub->stopOffer();
		delete pub;
	}

	unsigned long publish(short id, unsigned size) override{
		unsigned long time=std::chrono::duration_cast<std::chrono::
                nanoseconds>(std::chrono::high_resolution_clock::
                now().time_since_epoch()).count();

		auto sample=static_cast<Message*>(pub->allocateChunk(size));
		sample->id=id;
		sample->timestamp=time;
		sample->len=size-sizeof(Message);
		memset(sample+sizeof(Message),'a',sample->len);

		time=std::chrono::duration_cast<std::chrono::
                nanoseconds>(std::chrono::high_resolution_clock::
                now().time_since_epoch()).count()-time;

		pub->sendChunk(sample);
		return time;
	}

};


template<class MsgType>
class Subscriber: public TestMiddlewareSub<MsgType>{
private:
	std::string _name;
	iox::popo::Subscriber* sub;
public:
	Subscriber(std::string &topic, 
			int msgCount, 
			int prior,
			int cpu_index, 
			std::string &filename, 
			int topic_priority
			): TestMiddlewareSub<MsgType>(topic, msgCount, prior,
            			cpu_index, filename, topic_priority)
	{
		iox::runtime::PoshRuntime::getInstance(std::string("/")+filename);
		char param[100];
		if(topic.length()<100) memcpy(param,topic.c_str(),topic.length()+1);
		else{
			memcpy(param,topic.c_str(),99);
			param[99]='\0';
		}
		sub=new iox::popo::Subscriber({"Iceoryx",param});
		sub->subscribe(10);
	}

	~Subscriber(){
		sub->unsubscribe();
		delete sub;
	}
	
	short get_id(MsgType &msg) override{
		return msg.id;
	}
	
	unsigned long get_timestamp(MsgType &msg) override{
		return msg.timestamp;
	}
	
	bool receive() override{
		const void* chunk=nullptr;
		bool get=sub->getChunk(&chunk);
		if(get){
			auto sample=static_cast<const Message*>(chunk);
			unsigned long time=std::chrono::duration_cast<std::chrono::
				nanoseconds>(std::chrono::high_resolution_clock::
				now().time_since_epoch()).count();
			Message msg;
			msg.timestamp=sample->timestamp;
			msg.id=sample->id;
			msg.len=sample->len;
			std::string str((char*)sample+sizeof(Message),msg.len);
			sub->releaseChunk(chunk);

			time=std::chrono::duration_cast<std::chrono::
				nanoseconds>(std::chrono::high_resolution_clock::
				now().time_since_epoch()).count()-time;
			TestMiddlewareSub<MsgType>::write_received_msg(msg,time);

		}
		return get;
	}

};


template<class MsgType>
class PingPong: public TestMiddlewarePingPong<MsgType>{
private:
	std::string _name;
	iox::popo::Publisher* pub;
	iox::popo::Subscriber* sub;
public:
	PingPong(std::string& topic,	
		  int msgCount, 
		  int prior, 
		  int cpu_index,
            	  std::string &filename, 
		  int topic_priority,
		  int interval, 
		  int msg_size, 
		  bool isFirst
			): TestMiddlewarePingPong<MsgType>(topic, msgCount, prior, cpu_index, filename,
				topic_priority, interval, msg_size, isFirst)
	{
		iox::runtime::PoshRuntime::getInstance(std::string("/")+filename);
		char param[100];
		if(topic.length()<100) memcpy(param,topic.c_str(),topic.length()+1);
		else{
			memcpy(param,topic.c_str(),99);
			param[99]='\0';
		}
		if(isFirst) pub=new iox::popo::Publisher({"Iceoryx",param,"first"});
		else pub=new iox::popo::Publisher({"Iceoryx",param,"second"});
		pub->offer();
		if(isFirst) sub=new iox::popo::Subscriber({"Iceoryx",param,"second"});
		else sub=new iox::popo::Subscriber({"Iceoryx",param,"first"});
		sub->subscribe(10);
	}

	~PingPong(){
		pub->stopOffer();
		delete pub;
		sub->unsubscribe();
		delete sub;
	}

	void publish(short id, unsigned size) override{

		auto sample=static_cast<Message*>(pub->allocateChunk(size));
		sample->id=id;
		sample->timestamp=std::chrono::duration_cast<std::chrono::
				nanoseconds>(std::chrono::high_resolution_clock::
				now().time_since_epoch()).count();
		sample->len=size-sizeof(Message);
		memset(sample+sizeof(Message),'a',sample->len);

		pub->sendChunk(sample);
		return ;
	}
	
	short get_id(MsgType &msg) override{
		return msg.id;
	}
	
	unsigned long get_timestamp(MsgType &msg) override{
		return msg.timestamp;
	}
	
	bool receive() override{
		const void* chunk=nullptr;
		bool get=sub->getChunk(&chunk);
		if(get){
			auto sample=static_cast<const Message*>(chunk);
			Message msg;
			msg.timestamp=sample->timestamp;
			msg.id=sample->id;
			msg.len=sample->len;
			std::string str((char*)sample+sizeof(Message),msg.len);
			sub->releaseChunk(chunk);

			TestMiddlewarePingPong<MsgType>::write_received_msg(msg);

		}
		return get;
	}

};

int main(int argc, char** argv){
	if(argc<3){
		std::cout<<"Usage: pubsub <type> <config_file>"<<std::endl;
		return 1;
	}
	std::ifstream file(argv[2]);
	if(!file){
		std::cout<<"Can't open file "<<argv[2]<<std::endl;
		return 2;
	}
	nlohmann::json json;
	file>>json;
	file.close();
	if(!strcmp(argv[1],"publisher")){

		std::string topic=json["topic"];
		std::string filename=json["res_filenames"][0];
		int m_count=json["m_count"];
		int min_size=json["min_msg_size"];
		int max_size=json["max_msg_size"];
		int step=json["step"];
		int before_step=json["msgs_before_step"];
		int prior=json["priority"][0];
		int cpu=json["cpu_index"][0];
		int interval=json["interval"];
		int topic_prior=json["topic_priority"];

		std::string name=std::string("/pub");
		std::cout<<"Publisher"<<std::endl;
		Publisher pub(topic, m_count, prior, cpu,  min_size, max_size, step,
				interval, before_step, filename, topic_prior);
		pub.StartTest();
		std::cout<<"End Publisher"<<std::endl;
	}
	if(!strcmp(argv[1],"subscriber")){

		std::string topic=json["topic"];
		std::string filename=json["res_filenames"][1];
		int m_count=json["m_count"];
		int prior=json["priority"][1];
		int cpu=json["cpu_index"][1];
		int topic_prior=json["topic_priority"];

		std::string name=std::string("/sub");
		std::cout<<"Subscriber"<<std::endl;
		Subscriber<Message> sub(topic, m_count, prior, cpu, filename, topic_prior);
		sub.StartTest();
		std::cout<<"End Subscriber"<<std::endl;
	}
	if(!strcmp(argv[1],"ping_pong")||json["isPingPong"]){
		if(argc<4){
			std::cout<<"No config for PingPong"<<std::endl;
			return 2;
		}
		file=std::ifstream(argv[3]);
		if(!file){
			std::cout<<"Can't open file "<<argv[3]<<std::endl;
			return 2;
		}
		nlohmann::json json_pp;
		file>>json_pp;
		file.close();
		bool isFirst=json_pp["isPingPong"];
		int i;
		if(isFirst) i=0;
		else i=1;
		std::string topic=json_pp["topic"];
		std::string filename=json_pp["res_filenames"][i];
		int m_count=json_pp["m_count"];
		int min_size=json_pp["min_msg_size"];
		int prior=json_pp["priority"][i];
		int cpu=json_pp["cpu_index"][i];
		int interval=json_pp["interval"];
		int topic_prior=json_pp["topic_priority"];

		std::cout<<"PingPong"<<std::endl;
		PingPong<Message> ping_pong(topic, m_count, prior, cpu, filename, topic_prior,
						interval, min_size, isFirst);
		ping_pong.StartTest();
		std::cout<<"End PingPong"<<std::endl;
	}

	return 0;
}
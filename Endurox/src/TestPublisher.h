#ifndef TESTPUBLISHER_H_
#define TESTPUBLISHER_H_

#include <atmi.h>

#include "../../interface/pub_interface.hpp"

class TestPublisher : public TestMiddlewarePub
{
    public:

        TestPublisher(std::string topic,  int msgCount, int prior, int cpu_index,
                  int min_msg_size, int max_msg_size, int step, int interval, int msgs_before_step,
		  std::string &filename, int topic_priority);

        virtual ~TestPublisher();

	unsigned long publish(short id, unsigned size);

    private:
        char* strbuf;
};


#endif /* TESTPUBLISHER_H_ */

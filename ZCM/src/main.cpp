#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "../include/publisher.h"
#include "../include/subscriber.h"
#include "../include/ping_pong.h"

int main(int argc, char** argv)
{
    argparse::ArgumentParser parser("ZCM node argparsing");
    parser.add_argument("-c", "--config")
            .required()
            .help("-c --conf is required argument with config path");

    parser.add_argument("-t", "--type")
            .required()
            .help("-t --type is required argument with type of node");

    parser.add_argument("--first")
            .implicit_value(true)
            .default_value(false)
            .help("--first is required argument if ping_pong type is specified with type of node");

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << parser;
        exit(1);
    }

    std::string config_filename = parser.get<std::string>("-c");
    std::string type_name = parser.get<std::string>("-t");

    bool isFirst = parser.get<bool>("--first");

    nlohmann::json args;
    std::ifstream file(config_filename);
    if(!file.is_open()) {
        std::cout << "Cannot open file " << config_filename << std::endl;
        return 0;
    }
    file >> args;
    file.close();

    std::string topic1 = args["topic"][0];
    std::string topic2 = args["topic"][1];
    std::string filename_pub = args["res_filenames"][0];
    std::string filename_sub = args["res_filenames"][1];
    int m_count = args["m_count"];
    int priority_pub = args["priority"][0];
    int priority_sub = args["priority"][1];
    int cpu_index_pub = args["cpu_index"][0];
    int cpu_index_sub = args["cpu_index"][1];
    int min_msg_size = args["min_msg_size"];
    int max_msg_size = args["max_msg_size"];
    int step = args["step"];
    int msgs_before_step = args["msgs_before_step"];
    int interval = args["interval"];
    int topic_prior = args["topic_priority"];

    try {
        if (type_name == "publisher"){
            TestPublisher publisher(topic1, m_count, priority_pub, cpu_index_pub, min_msg_size, max_msg_size, step, interval,
                                    msgs_before_step, filename_pub, topic_prior);
            publisher.StartTest();
            return 0;
        }
        else if (type_name == "subscriber") {
            TestSubscriber subscriber(topic1, m_count, priority_sub, cpu_index_sub, filename_sub, topic_prior, max_msg_size);
            subscriber.StartTest();
            return 0;
        }
        else if (type_name == "ping_pong"){

            std::string filename;
            if(isFirst)
                filename = filename_pub;
            else
                filename = filename_sub;
            if(interval == 0){
                TestPingPong ping_pong(topic1, topic2, m_count, priority_pub,
                                       cpu_index_pub, filename, topic_prior, interval,
                                       min_msg_size, isFirst);
                ping_pong.StartTest();
            }
            else {
                TestPingPong ping_pong(topic1, topic2, m_count,
                                       priority_pub, cpu_index_pub, filename,
                                       topic_prior, interval, min_msg_size,
                                       max_msg_size, step, msgs_before_step, isFirst);
                ping_pong.StartTest();
            }
        }
        else{
            std::cout << "Wrong node type specified!" << std::endl;
            return 0;
        }
    }
    catch (test_exception& e){
        std::cout<< e.what() << std::endl;
        return -e.get_ret_code();
    }
    catch (std::exception& e){
        std::cout<< e.what()<< std::endl;
        return -MIDDLEWARE_ERROR;
    }

    return 0;
}


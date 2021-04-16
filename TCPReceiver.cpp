///usr/include
// Created by weiqin xu on 19/11/2020.
//

#include "TCPReceiver.hpp"


std::vector<std::string> app_buff;
bool interrupt_flag = false;
std::thread receiver;
std::thread query_thread;
std::mutex lock_tcp;
std::vector<std::string> query = {};
std::vector<std::string> query_buffer = {};
//std::string url = "localhost";
const char* url = "192.168.43.116";

// Shouldn't be used
extern std::vector<std::string> columns {"\"1\"","\"Timestamp_1\""};

std::string client_id = "testid";
// Harcode to test
long long time_reception;
struct mosquitto *mosq;
struct mosquitto *mosq_query;
std::regex regex("\\,");
bool isServer = false;
bool started_receiving = false;
bool data_waiting = false;
std::unordered_map<std::string, std::vector<std::vector<std::string>>> map;

std::vector<std::string> split(const std::string& input, const std::string& regex) {
    std::regex re(regex);
    std::sregex_token_iterator
            first{input.begin(), input.end(), re, -1},
            last;
    return {first, last};
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){
    char buf[1024];
    char topic_buf[1024];
    memcpy(buf, message->payload, 1024 * sizeof(char));
    memcpy(topic_buf, message->topic, 1024 * sizeof(char));
    std::string str = buf;
    std::string topic = topic_buf;

    str.erase(0,1);
    str.erase(str.end()-1,str.end());
    std::vector<std::string> out(
            std::sregex_token_iterator(str.begin(), str.end(), regex, -1),
            std::sregex_token_iterator()
    );

    // Network latence
    time_reception = std::chrono::system_clock::now().time_since_epoch() /std::chrono::milliseconds(1);
    //std::cout << time_reception - stoll(out[1]) << std::endl;
    
    //std::cout << out[0] << std::endl;

    lock_tcp.lock();
    if(map.find(topic) == map.end())
        map[topic] = {};

    map[topic].push_back(out);
    data_waiting = true;
    lock_tcp.unlock();
}

void receive_query(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){
    char buf[1024];
    memcpy(buf, message->payload, 1024 * sizeof(char));
    std::string tmp = buf;
    if(tmp.rfind("__start__",0) == 0){
        int pos = tmp.find('|');
        if(pos == -1){
            started_receiving = true;
        }
        else{
            tmp = tmp.substr(pos + 1);
            std::vector<std::string> ids(
                    std::sregex_token_iterator(tmp.begin(), tmp.end(), regex, -1),
                    std::sregex_token_iterator()
            );
            for(auto id : ids){
                if(id == client_id) {
                    started_receiving = true;
                    break;
                }
            }
        }
    }
    
    else if(started_receiving && tmp == "__end__"){
        lock_tcp.lock();
        for(auto copy : query_buffer){
            query.push_back(copy);
        }
        lock_tcp.unlock();
    }
    else if(started_receiving){
        query_buffer.push_back(tmp);
    }
}

void transmit_query(std::vector<std::string> queries, std::vector<std::string> ids){
    mosquitto_loop_start(mosq_query);
    std::string str = "__start__|";
    for(auto id : ids){
        str += id + ",";
    }
    str.pop_back();
    mosquitto_publish_v5(mosq_query, NULL, "query", str.length(), str.c_str(), 1, false, NULL);
    mosquitto_loop_write(mosq_query, 1000);
    for(auto tmp: queries) {
        mosquitto_publish_v5(mosq_query, NULL, "query", tmp.length(), tmp.c_str(), 1, false, NULL);
        mosquitto_loop_write(mosq_query, 1000);
    }
    str = "__end__";
    mosquitto_publish_v5(mosq_query, NULL, "query", str.length(), str.c_str(), 1, false, NULL);
    mosquitto_loop_write(mosq_query, 1000);
    mosquitto_loop_stop(mosq_query,false);
}

void tcpReceiver(){
    mosquitto_lib_init();
    
    if(isServer){
        mosq = mosquitto_new("SERVER_Succinct", false, NULL);
        mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        mosquitto_connect(mosq, url, 1883, 1000);
        mosquitto_message_callback_set(mosq, on_message);
        mosquitto_subscribe_v5(mosq, NULL, "anomaly", 2, 0, NULL);
        
        mosq_query = mosquitto_new(NULL, true, NULL);
        mosquitto_int_option(mosq_query, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        mosquitto_connect(mosq_query, url, 1883, 1000);
    }
    else{
        mosq = mosquitto_new(NULL, true, NULL);
        mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        mosquitto_connect(mosq, url, 1883, 1000);
        mosquitto_message_callback_set(mosq, on_message);
        mosquitto_subscribe_v5(mosq, NULL, "test", 2, 0, NULL);
        
        for(int i = 1; i <= 100; i++){
            mosquitto_subscribe_v5(mosq, NULL, std::to_string(i).c_str(), 2, 0, NULL);
        }

        mosq_query = mosquitto_new(NULL, true, NULL);
        mosquitto_int_option(mosq_query, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        mosquitto_connect(mosq_query, url, 1883, 1000);
        mosquitto_message_callback_set(mosq_query, receive_query);
        mosquitto_subscribe_v5(mosq_query, NULL, "query", 2, 0, NULL);
        mosquitto_loop_forever(mosq_query, -1, 1);
    }
    
    
}

void mosquitto_looping(){
    
    if(isServer){
        mosq = mosquitto_new("SERVER_Succinct", false, NULL);
        mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        mosquitto_connect(mosq, url, 1883, 1000);
        mosquitto_message_callback_set(mosq, on_message);
        mosquitto_subscribe_v5(mosq, NULL, "anomaly", 2, 0, NULL);
        
    }
    else{
        mosq = mosquitto_new(NULL, true, NULL);
        mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
        mosquitto_connect(mosq, url, 1883, 1000);
        mosquitto_message_callback_set(mosq, on_message);
        
        for(int i = 1; i <= 3; i++){
            mosquitto_subscribe_v5(mosq, NULL, ("Topic" + std::to_string(i)).c_str(), 2, 0, NULL);
        }
    }
    
    std::cout << "Mosquitto loop running" << std::endl;
    mosquitto_loop_forever(mosq, -1, 1);
}

void report_anomaly(std::string anom){
    mosquitto_publish_v5(mosq, NULL, "anomaly", anom.length(), anom.c_str(), 1, false, NULL);
}

std::vector<std::string> getQueries(){
    while(query.size() < 2){
        usleep(100);
    }
    return query;
}

std::vector<std::tuple<std::string, std::vector<std::vector<std::string>>>> getCSV(){
    lock_tcp.lock();
    // No data has been received yet
    if(!data_waiting){
        lock_tcp.unlock();
        return {};
    }
    data_waiting = false;
    std::vector<std::tuple<std::string, std::vector<std::vector<std::string>>>> tmp = {};

    // Creates a tuple for every sensor
    for(auto kv : map){
        std::string topic;
        std::vector<std::vector<std::string>> content;
        std::tie(topic, content) = kv;
        tmp.push_back(std::tuple<std::string, std::vector<std::vector<std::string>>>{topic, content});

        // Reset column names for next rows
        map[topic] = {};
    }

    
    lock_tcp.unlock();
    return tmp;
}


// Lance la Thread qui reçoit les données par TCP
void run_tcp_receiver(bool server){
    isServer = server;
    receiver = std::thread (tcpReceiver);
    query_thread = std::thread (mosquitto_looping);
}

// Arrête la thread TCP
void stop_tcp_receiver(){
    mosquitto_disconnect(mosq);
    receiver.join();
}

std::string erase_head(std::string str){
    //std::cout << "current str:" << str << " ";
    while(str.size()>0 && !(str.front() >= '0' && str.front() <= '9'))
        str.erase(str.begin());
        
    while(str.size()>0 && !(str.back() >= '0' && str.back() <= '9'))
        str.erase(str.end()-1);
    //std::cout << str << " ";
    return str;
}

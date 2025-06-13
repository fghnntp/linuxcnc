#ifndef _GATHER_SENDER_
#define _GATHER_SENDER_
#include <string>
#include <memory>
#include <sw/redis++/redis++.h>

class GatherSender {
    public:
    GatherSender();
    ~GatherSender();
    
    bool PushMsg(std::string&);
    void ClearMsg();
    void PushBatch(const std::vector<std::string>& batch);
    void Flush(); // 新增：强制flush缓冲

private:
    void InitRedis();
    // Redis相关
    std::unique_ptr<sw::redis::Redis> redis_;
    bool isReidsRight;
    // 批量缓冲
    std::vector<std::string> batch_buffer_;
    std::mutex batch_mutex_;
    const size_t BATCH_SIZE = 10000;

};
#endif
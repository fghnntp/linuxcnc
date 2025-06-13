#include "gather_sender.hh"


using namespace sw::redis;

GatherSender::GatherSender()
:isReidsRight(false)
{
    try {
        InitRedis();
    } catch (const std::exception& e) {
        isReidsRight = false;
    }
    
}

GatherSender::~GatherSender()
{
}

bool GatherSender::PushMsg(std::string& str)
{
    std::lock_guard<std::mutex> lock(batch_mutex_);
    batch_buffer_.emplace_back(std::move(str));
    if (batch_buffer_.size() >= BATCH_SIZE) {
        PushBatch(batch_buffer_);
        batch_buffer_.clear();
    }
    return true;
}

// bool GatherSender::PushMsg(std::string& str)
// {
//      if (!redis_ ) return false;
    
//     try {
//         // 构建Redis键
//         std::string key = "rtcp";
        
        
//         // // 使用Redis的LPUSH批量插入
//         // redis_->lpush(key, values.begin(), values.end());

//         redis_->lpush(key, str);
        
//         // 可选：限制列表长度，防止内存无限增长
//         // redis_->ltrim(key, 0, 9999); // 保留最新的10000个元素
        
//         return true;
        
//     } catch (const std::exception& e) {
//         return false;
//     }
// }

void GatherSender::PushBatch(const std::vector<std::string>& batch)
{
    if (!redis_ || batch.empty()) return;
    try {
        auto pipe = redis_->pipeline();
        for (const auto& msg : batch) {
            pipe.lpush("rtcp", msg);
        }
        pipe.exec();
        // 可选：限制列表长度，防止内存无限增长
        // redis_->ltrim("rtcp", 0, 9999);
    } catch (const std::exception& e) {
        // 可以加日志
    }
}

void GatherSender::Flush()
{
    std::lock_guard<std::mutex> lock(batch_mutex_);
    if (!batch_buffer_.empty()) {
        PushBatch(batch_buffer_);
        batch_buffer_.clear();
    }
}

void GatherSender::ClearMsg()
{
    redis_->del("rtcp");
}

void GatherSender::InitRedis()
{
    try {
        // Redis连接配置，根据实际情况修改
        ConnectionOptions connection_options;
        connection_options.host = "10.10.30.19";
        connection_options.port = 6379;
        connection_options.socket_timeout = std::chrono::milliseconds(100);
        
        redis_ = std::make_unique<Redis>(connection_options);
        
        // 测试连接
        redis_->ping();
        redis_->del("rtcp");
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Redis initialization failed: " + std::string(e.what()));
    }
}

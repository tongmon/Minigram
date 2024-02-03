#ifndef HEADER__FILE__MONGODBPOOL
#define HEADER__FILE__MONGODBPOOL

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>

#include <atomic>
#include <memory>
#include <mutex>

struct MongoConnectionInfo
{
    std::string db_host;
    std::string db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;
    int min_pool_size = 8;
    int max_pool_size = 8;
};

class MongoDBPool
{
    static inline std::atomic<std::shared_ptr<MongoDBPool>> instance = nullptr;
    static inline std::mutex mut;

    struct Deleter
    {
        void operator()(MongoDBPool *ptr)
        {
            delete ptr;
            instance.store(nullptr);
        }
    };
    friend Deleter;

    std::unique_ptr<mongocxx::instance> m_mongo_inst;
    std::unique_ptr<mongocxx::pool> m_mongo_pool;

    MongoDBPool(const MongoConnectionInfo &connection_info);
    ~MongoDBPool();

  public:
    static mongocxx::pool &Get(const MongoConnectionInfo &connection_info = {})
    {
        if (!instance.load(std::memory_order_acquire))
        {
            std::lock_guard<std::mutex> lock(mut);
            if (!instance.load(std::memory_order_relaxed))
                instance.store(std::shared_ptr<MongoDBPool>(new MongoDBPool(connection_info), Deleter{}), std::memory_order_release);
        }
        return *instance.load(std::memory_order_relaxed)->m_mongo_pool;
    }

    MongoDBPool(MongoDBPool const &) = delete;
    MongoDBPool(MongoDBPool &&) = delete;
    MongoDBPool &operator=(MongoDBPool const &) = delete;
    MongoDBPool &operator=(MongoDBPool &&) = delete;
};

#endif /* HEADER__FILE__MONGODBPOOL */

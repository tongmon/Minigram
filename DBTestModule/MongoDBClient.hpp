#ifndef HEADER__FILE__MONGODBCLIENT
#define HEADER__FILE__MONGODBCLIENT

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
};

class MongoDBClient
{
    static inline std::atomic<std::shared_ptr<MongoDBClient>> instance = nullptr;
    static inline std::mutex mut;

    struct Deleter
    {
        void operator()(MongoDBClient *ptr)
        {
            delete ptr;
            instance.store(nullptr);
        }
    };
    friend Deleter;

    std::unique_ptr<mongocxx::instance> m_mongo_inst;
    std::unique_ptr<mongocxx::client> m_mongo_client;

    MongoDBClient(const MongoConnectionInfo &connection_info);
    ~MongoDBClient();

  public:
    static mongocxx::client &Get(const MongoConnectionInfo &connection_info = {})
    {
        if (!instance.load(std::memory_order_acquire))
        {
            std::lock_guard<std::mutex> lock(mut);
            if (!instance.load(std::memory_order_relaxed))
                instance.store(std::shared_ptr<MongoDBClient>(new MongoDBClient(connection_info), Deleter{}), std::memory_order_release);
        }

        mut.lock();
        return *instance.load(std::memory_order_relaxed)->m_mongo_client;
    }

    static void Free()
    {
        mut.unlock();
    }

    MongoDBClient(MongoDBClient const &) = delete;
    MongoDBClient(MongoDBClient &&) = delete;
    MongoDBClient &operator=(MongoDBClient const &) = delete;
    MongoDBClient &operator=(MongoDBClient &&) = delete;
};

#endif /* HEADER__FILE__MONGODBCLIENT */

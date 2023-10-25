#ifndef HEADER__FILE__POSTGREDBPOOL
#define HEADER__FILE__POSTGREDBPOOL

#include <soci/postgresql/soci-postgresql.h>
#include <soci/soci.h>

#include <atomic>
#include <memory>
#include <mutex>

struct PostgreConnectionInfo
{
    std::string db_host;
    std::string db_port;
    std::string db_name;
    std::string db_user;
    std::string db_password;
    int pool_size = 8;
};

// TCPServer와의 결합은 없고 파생 Service에 대한 결합만 필요하기에 싱글턴으로 구성함
// DB 연결 풀을 담당함
class PostgreDBPool
{
    static inline std::atomic<std::shared_ptr<PostgreDBPool>> instance = nullptr;
    static inline std::mutex mut;

    struct Deleter
    {
        void operator()(PostgreDBPool *ptr)
        {
            delete ptr;
            instance.store(nullptr);
        }
    };
    friend Deleter;

    std::unique_ptr<soci::connection_pool> m_connection_pool;

    PostgreDBPool(const PostgreConnectionInfo &connection_info);
    ~PostgreDBPool();

  public:
    static soci::connection_pool &Get(const PostgreConnectionInfo &connection_info = {})
    {
        if (!instance.load(std::memory_order_acquire))
        {
            std::lock_guard<std::mutex> lock(mut);
            if (!instance.load(std::memory_order_relaxed))
                instance.store(std::shared_ptr<PostgreDBPool>(new PostgreDBPool(connection_info), Deleter{}), std::memory_order_release);
        }
        return *instance.load(std::memory_order_relaxed)->m_connection_pool;
    }

    PostgreDBPool(PostgreDBPool const &) = delete;
    PostgreDBPool(PostgreDBPool &&) = delete;
    PostgreDBPool &operator=(PostgreDBPool const &) = delete;
    PostgreDBPool &operator=(PostgreDBPool &&) = delete;
};

#endif /* HEADER__FILE__POSTGREDBPOOL */

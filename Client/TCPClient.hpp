#ifndef HEADER__FILE__TCPCLIENT
#define HEADER__FILE__TCPCLIENT

#include "NetworkBuffer.hpp"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

// Boost.Asio Windows 7 이상을 타겟으로 설정
// #define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <boost/asio.hpp>

class Session
{
    friend class TCPClient;

    boost::asio::ip::tcp::socket m_sock;
    boost::asio::ip::tcp::endpoint m_ep;
    NetworkBuffer m_request;

    boost::asio::streambuf m_response_buf;
    NetworkBuffer m_response;

    boost::system::error_code m_ec;

    unsigned int m_id;

    bool m_was_cancelled;
    std::mutex m_cancel_guard;

  public:
    Session(boost::asio::io_service &ios,
            const std::string &raw_ip_address,
            unsigned short port_num,
            unsigned int id)
        : m_sock(ios),
          m_ep(boost::asio::ip::address::from_string(raw_ip_address), port_num),
          m_id(id),
          m_was_cancelled(false)
    {
    }

    bool IsValid()
    {
        return m_ec == boost::system::errc::success;
    }

    NetworkBuffer &GetResponse()
    {
        return m_response;
    }

    unsigned int GetID()
    {
        return m_id;
    }
};

class TCPClient
{
    boost::asio::io_service m_ios;
    std::map<unsigned int, std::shared_ptr<Session>> m_active_sessions;
    std::mutex m_active_sessions_guard;
    std::unique_ptr<boost::asio::io_service::work> m_work;
    std::list<std::unique_ptr<std::thread>> m_threads;

  public:
    TCPClient(unsigned char num_of_threads);
    ~TCPClient();

    bool AsyncConnect(const std::string &raw_ip_address,
                      unsigned short port_num,
                      unsigned int request_id,
                      std::function<void(std::shared_ptr<Session>)> on_success_connection = {});

    template <typename T>
    void AsyncWrite(unsigned int request_id,
                    T &&request,
                    std::function<void(std::shared_ptr<Session>)> on_finish_write = {})
    {
        std::shared_ptr<Session> session;

        std::unique_lock<std::mutex> lock(m_active_sessions_guard);
        if (m_active_sessions.find(request_id) == m_active_sessions.end())
        {
            if (on_finish_write)
                on_finish_write(nullptr);
            return;
        }
        m_active_sessions[request_id]->m_request = std::forward<T>(request);
        session = m_active_sessions[request_id];
        lock.unlock();

        boost::asio::async_write(session->m_sock,
                                 session->m_request.AsioBuffer(),
                                 [this, session, on_finish_write](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                     if (ec != boost::system::errc::success)
                                     {
                                         session->m_ec = ec;
                                         CloseRequest(session->m_id);
                                         if (on_finish_write)
                                             on_finish_write(session);
                                         return;
                                     }

                                     std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);

                                     if (session->m_was_cancelled)
                                     {
                                         CloseRequest(session->m_id);
                                         if (on_finish_write)
                                             on_finish_write(session);
                                         return;
                                     }

                                     if (on_finish_write)
                                         on_finish_write(session);
                                 });
    }

    void AsyncRead(unsigned int request_id,
                   size_t buffer_size,
                   std::function<void(std::shared_ptr<Session>)> on_finish_read);

    void AsyncReadUntil(unsigned int request_id,
                        std::function<void(std::shared_ptr<Session>)> on_finish_read_until,
                        const std::string &delim = "\n");

    std::shared_ptr<Session> CloseRequest(unsigned int request_id);

    void CancelRequest(unsigned int request_id);

    void Close();

    unsigned int MakeRequestID();
};

#endif /* HEADER__FILE__TCPCLIENT */

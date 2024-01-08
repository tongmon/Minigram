#ifndef HEADER__FILE__SERVICE
#define HEADER__FILE__SERVICE

// Boost.Asio Windows 7 이상을 타겟으로 설정
// #define _WIN32_WINNT _WIN32_WINNT_WIN7

#include "NetworkBuffer.hpp"

#include <boost/asio.hpp>
#include <memory>
#include <string>

class WinQuickWindow;

class Service
{
  public:
    std::shared_ptr<boost::asio::ip::tcp::socket> sock;
    WinQuickWindow &window;

    boost::asio::streambuf server_response_buf;
    NetworkBuffer server_response;

    Service(WinQuickWindow &window, std::shared_ptr<boost::asio::ip::tcp::socket> sock);

    void StartHandling();
};

#endif /* HEADER__FILE__SERVICE */

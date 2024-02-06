#include "Service.hpp"
#include "MainContext.hpp"
#include "TCPClient.hpp"
#include "Utility.hpp"
#include "WinQuickWindow.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

Service::Service(WinQuickWindow &window, std::shared_ptr<boost::asio::ip::tcp::socket> sock)
    : window{window}, sock{sock}
{
}

void Service::StartHandling()
{
    boost::asio::async_read(*sock,
                            server_response_buf.prepare(server_response.GetHeaderSize()),
                            [this](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                if (ec != boost::system::errc::success)
                                {
                                    // 서버 처리가 비정상인 경우
                                    delete this;
                                    return;
                                }

                                server_response_buf.commit(bytes_transferred);
                                server_response = server_response_buf;

                                // std::istream strm(&m_server_request_buf);
                                // std::getline(strm, m_server_request);

                                // TCPHeader header(m_server_request);
                                // auto connection_type = header.GetConnectionType();
                                // auto data_size = header.GetDataSize();

                                boost::asio::async_read(*sock,
                                                        server_response_buf.prepare(server_response.GetDataSize()),
                                                        [this, connection_type = server_response.GetConnectionType()](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                                                            if (ec != boost::system::errc::success)
                                                            {
                                                                // 서버 처리가 비정상인 경우
                                                                delete this;
                                                                return;
                                                            }

                                                            server_response_buf.commit(bytes_transferred);
                                                            server_response = server_response_buf;

                                                            // std::istream strm(&m_server_request_buf);
                                                            // std::getline(strm, m_server_request);

                                                            switch (connection_type)
                                                            {
                                                            case LOGIN_CONNECTION_TYPE:
                                                                break;
                                                            case CHAT_RECIEVE_TYPE:
                                                                window.GetMainContext().RecieveChat(this);
                                                                break;
                                                            case SESSIONLIST_INITIAL_TYPE:
                                                                break;
                                                            case MESSAGE_READER_UPDATE_TYPE:
                                                                window.GetMainContext().RefreshReaderIds(this);
                                                                break;
                                                            case RECEIVE_CONTACT_REQUEST_TYPE:
                                                                window.GetMainContext().RecieveContactRequest(this);
                                                                break;
                                                            case RECEIVE_ADD_SESSION_TYPE:
                                                                window.GetMainContext().RecieveAddSession(this);
                                                                break;
                                                            default:
                                                                break;
                                                            }
                                                        });
                            });
}
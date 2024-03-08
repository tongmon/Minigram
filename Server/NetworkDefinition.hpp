#ifndef HEADER__FILE__NETWORKDEFINITION
#define HEADER__FILE__NETWORKDEFINITION

#include <cstdint>
#include <string>

constexpr char SERVER_IP[] = "127.0.0.1";
constexpr std::uint64_t SERVER_PORT = 4000;

enum ConnectionType : int64_t
{
    NONE_TYPE,
    LOGIN_CONNECTION_TYPE,
    CHAT_SEND_TYPE,
    CHAT_RECEIVE_TYPE,
    SESSIONLIST_INITIAL_TYPE,
    CONTACTLIST_INITIAL_TYPE,
    SIGNUP_TYPE,
    SESSION_ADD_TYPE,
    SEND_CONTACT_REQUEST_TYPE,
    DELETE_CONTACT_TYPE,
    RECEIVE_CONTACT_REQUEST_TYPE,
    SESSION_REFRESH_TYPE,
    DELETE_SESSION_TYPE,
    FETCH_MORE_MESSAGE_TYPE,
    MESSAGE_READER_UPDATE_TYPE,
    GET_CONTACT_REQUEST_LIST_TYPE,
    PROCESS_CONTACT_REQUEST_TYPE,
    LOGOUT_TYPE,
    RECEIVE_ADD_SESSION_TYPE,
    CONNECTION_TYPE_CNT
};

enum AccountRegisterResult : int64_t
{
    REGISTER_SUCCESS,
    REGISTER_DUPLICATION,
    REGISTER_CONNECTION_FAIL,
    REGISTER_PROCEEDING
};

enum LoginResult : int64_t
{
    LOGIN_SUCCESS,
    LOGIN_FAIL,
    LOGIN_CONNECTION_FAIL,
    LOGIN_PROCEEDING
};

enum ContactAddResult : int64_t
{
    CONTACTADD_SUCCESS,
    CONTACTADD_DUPLICATION,
    CONTACTADD_ID_NO_EXSIST,
    CONTACTADD_CONNECTION_FAIL
};

enum ContactRelationStatus : int64_t
{
    RELATION_SEND,
    RELATION_TAKE,
    RELATION_FRIEND,
    RELATION_HIDE,
    RELATION_BLOCKED
};

enum ChatType : int64_t
{
    UNDEFINED_TYPE,
    TEXT_CHAT,
    IMG_CHAT,
    VIDEO_CHAT,
    CHAT_TYPE_CNT
};

struct UserData
{
    std::string user_id;
    std::string user_name;
    std::string user_info;
    std::string user_img_path;
};

struct LoginData
{
    std::string ip;
    int port;
};

#endif /* HEADER__FILE__NETWORKDEFINITION */

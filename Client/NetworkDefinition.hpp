﻿#ifndef HEADER__FILE__NETWORKDEFINITION
#define HEADER__FILE__NETWORKDEFINITION

#include <cstdint>

constexpr char SERVER_IP[] = "127.0.0.1";
constexpr std::uint64_t SERVER_PORT = 4000;

enum ConnectionType : int64_t
{
    NONE_TYPE,
    LOGIN_CONNECTION_TYPE,
    CHAT_SEND_TYPE,
    CHAT_RECIEVE_TYPE,
    SESSIONLIST_INITIAL_TYPE,
    CONTACTLIST_INITIAL_TYPE,
    SIGNUP_TYPE,
    SESSION_ADD_TYPE,
    CONTACT_ADD_TYPE,
    SESSION_REFRESH_TYPE,
    FETCH_MORE_MESSAGE_TYPE,
    MESSAGE_READER_UPDATE_TYPE,
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
    RELATION_PROCEEDING,
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

#endif /* HEADER__FILE__NETWORKDEFINITION */

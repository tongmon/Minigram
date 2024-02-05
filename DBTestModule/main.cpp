#include "MongoDBClient.hpp"
#include "MongoDBPool.hpp"
#include "PostgreDBPool.hpp"

#include <chrono>
#include <iostream>
#include <regex>

void PostgreDBTestZone()
{
    PostgreDBPool::Get({"localhost", "5432", "Minigram", "tongstar", "@Lsy12131213", 4});
    auto postgre = std::make_unique<soci::session>(PostgreDBPool::Get());
}

void MongoDBTestZone()
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    /*
        MongoDBPool::Get({"localhost", "27017", "Minigram", "tongstar", "@Lsy12131213", 4, 4});

        auto ent = std::make_unique<mongocxx::pool::entry>(MongoDBPool::Get().acquire());
        mongocxx::client &client = *(*(ent));

        auto db = client["Minigram"];
        auto col = db["test_collection"];
        stream::document sort_doc{};
        sort_doc << "message_id" << -1;
        mongocxx::options::find opts;
        opts.sort(sort_doc.view()).limit(1);

        auto mongo_cursor = col.find({}, opts);
        if (mongo_cursor.begin() != mongo_cursor.end())
        {
            auto doc = *mongo_cursor.begin();
            auto t = doc["message_id"];
        }
    */

    // mongocxx::instance ints{};
    //
    // std::string session_info = "mongodb://%DB_USER%:%DB_PASSWORD%@%DB_HOST%:%DB_PORT%/%DB_NAME%?minPoolSize=%MIN_POOL_SIZE%&maxPoolSize=%MAX_POOL_SIZE%";
    // session_info = std::regex_replace(session_info, std::regex("%DB_HOST%"), "localhost");
    // session_info = std::regex_replace(session_info, std::regex("%DB_PORT%"), "27017");
    // session_info = std::regex_replace(session_info, std::regex("%DB_NAME%"), "Minigram");
    // session_info = std::regex_replace(session_info, std::regex("%DB_USER%"), "tongstar");
    // session_info = std::regex_replace(session_info, std::regex("%DB_PASSWORD%"), EncodeURL("@Lsy12131213"));
    // session_info = std::regex_replace(session_info, std::regex("%MIN_POOL_SIZE%"), std::to_string(4));
    // session_info = std::regex_replace(session_info, std::regex("%MAX_POOL_SIZE%"), std::to_string(4));
    //
    // mongocxx::uri uri(session_info);
    //
    // mongocxx::options::client client_options;
    // auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
    // client_options.server_api_opts(api);
    //
    // mongocxx::client client(uri, client_options);
    //
    // auto db = client["Minigram"];
    // auto col = db["test_collection"];
    //
    // stream::document sort_doc{};
    // sort_doc << "message_id" << -1;
    // mongocxx::options::find opts;
    // opts.sort(sort_doc.view()).limit(1);
    //
    // auto mongo_cursor = col.find({}, opts);
    // auto start_t = std::chrono::high_resolution_clock::now();
    // if (mongo_cursor.begin() != mongo_cursor.end())
    //{
    //    auto doc = *mongo_cursor.begin();
    //    auto t = doc["message_id"];
    //    int tt = 0;
    //}
    // auto end_t = std::chrono::high_resolution_clock::now();
    // auto ret = std::chrono::duration_cast<std::chrono::seconds>(end_t - start_t);
    // std::cout << ret.count() << "s\n";

    auto &client = MongoDBClient::Get({"localhost", "27017", "Minigram", "tongstar", "@Lsy12131213"});

    auto db = client["Minigram"];
    auto col = db["test_collection"];

    mongocxx::options::find opts;
    opts.limit(1);
    auto mongo_cursor = col.find({}, opts);
    auto id = (*mongo_cursor.begin())["message_id"].get_int32();

    // stream::document sort_doc{};
    // sort_doc << "message_id" << -1;
    // mongocxx::options::find opts;
    // opts.sort(sort_doc.view()).limit(1);
    //
    // auto mongo_cursor = col.find({}, opts);
    // auto start_t = std::chrono::high_resolution_clock::now();
    // if (mongo_cursor.begin() != mongo_cursor.end())
    //{
    //    auto doc = *mongo_cursor.begin();
    //    auto t = doc["message_id"];
    //}
    // auto end_t = std::chrono::high_resolution_clock::now();
    // auto ret = std::chrono::duration_cast<std::chrono::seconds>(end_t - start_t);
    // std::cout << ret.count() << "s\n";

    MongoDBClient::Free();

    // mongo_cursor = std::make_unique<mongocxx::cursor>(mongo_coll.find(basic::make_document(basic::kvp("send_date",
    //                                                                                                   basic::make_document(basic::kvp("$gt",
    //                                                                                                                                   types::b_date{tp})))),
    //                                                                   opts));

    // session_data["unread_count"] = mongo_coll.count_documents(basic::make_document(basic::kvp("send_date",
    //                                                                                           basic::make_document(basic::kvp("$gt",
    //                                                                                                                           msg_id)))));

    // #pragma region The way to use transaction in mongocxx
    //     // 일단 mongodb 트랜잭션은 replica set인 경우에만 사용이 가능하다.
    //     // local machine에서 replica set를 구성하는 방법을 알아야 테스트가 가능한디...
    //
    //     // replica set을 사용하지 않고 해결하려면... 특정 컬렉션을 따로 만들고 해당 컬랙션에 message_cnt를 카운트하는 도큐먼트를 넣어놓는다.
    //     // message_cnt를 읽고 수정할 때 find_one_and_update 함수를 이용하면 해당 message_cnt는 원자성이 지켜진다.
    //
    //     mongocxx::options::transaction transaction_opts; // 옵션 설정을 잘못하면 속도가 느려질 수 있음
    //
    //     mongocxx::read_concern rc;
    //     mongocxx::write_concern wc;
    //     mongocxx::read_preference rp;
    //
    //     rp.mode(mongocxx::read_preference::read_mode::k_primary);
    //     rc.acknowledge_level(mongocxx::read_concern::level::k_snapshot);
    //     wc.acknowledge_level(mongocxx::write_concern::level::k_majority);
    //
    //     transaction_opts.read_preference(rp);
    //     transaction_opts.read_concern(rc);
    //     transaction_opts.write_concern(wc);
    //
    //     auto session = client.start_session();
    //     session.start_transaction(); // transaction_opts이 있다면 여기 넣으면 됨
    //
    //     try
    //     {
    //         int32_t message_cnt = col.count_documents(session, basic::document().view());
    //
    //         basic::array readers;
    //         readers.append("tongstar");
    //         readers.append("yellowjam");
    //         col.insert_one(session, basic::make_document(basic::kvp("message_id", message_cnt),
    //                                                      basic::kvp("sender_id", "tongstar"),
    //                                                      basic::kvp("send_date", 1707098027701),
    //                                                      basic::kvp("content_type", 1),
    //                                                      basic::kvp("content", "Ok~ I can hear u."),
    //                                                      basic::kvp("reader_id", readers)));
    //
    //         session.commit_transaction();
    //     }
    //     catch (mongocxx::exception &e)
    //     {
    //         std::wcerr << e.what() << std::endl;
    //     }
    // #pragma endregion

    // stream::document sort_doc{};
    // sort_doc << "message_id" << -1;
    // mongocxx::options::find opts;
    // opts.sort(sort_doc.view()).limit(2);
    //
    // auto start_t = std::chrono::high_resolution_clock::now();
    // auto mongo_cursor = col.find({}, opts);
    // auto cur_it = ++mongo_cursor.begin();
    // if (cur_it != mongo_cursor.end())
    // {
    //     auto doc = *mongo_cursor.begin();
    //     auto t = doc["message_id"].get_int32();
    //     int tt = 0;
    // }
    // auto end_t = std::chrono::high_resolution_clock::now();
    // auto ret = std::chrono::duration_cast<std::chrono::seconds>(end_t - start_t);
    // std::cout << ret.count() << "s\n";
}

int main(int argc, char *argv[])
{
    MongoDBTestZone();

    return 0;
}
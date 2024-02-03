#include "MongoDBPool.hpp"
#include "PostgreDBPool.hpp"

int main(int argc, char *argv[])
{
    using namespace bsoncxx;
    using namespace bsoncxx::builder;

    // MongoDB 연결 정보 초기화
    MongoDBPool::Get({"localhost", "27017", "Minigram", "tongstar", "@Lsy12131213", 4, 4});

    // PostgreDB 연결 정보 초기화
    PostgreDBPool::Get({"localhost", "5432", "Minigram", "tongstar", "@Lsy12131213", 4});

    auto postgre = std::make_unique<soci::session>(PostgreDBPool::Get());

    auto mongo = MongoDBPool::Get().acquire();
    auto mongo_db = (*mongo)["Minigram"];
    auto mongo_coll = mongo_db["test_collection"];

    // auto opts = mongocxx::options::find{};
    // opts.sort(basic::make_document(basic::kvp("message_id", 1)).view()).limit(1);
    // auto mongo_cursor = mongo_coll.find({}, opts);

    stream::document sort_doc{};
    sort_doc << "message_id" << -1;
    mongocxx::options::find opts;
    opts.sort(sort_doc.view()).limit(1);
    auto mongo_cursor = mongo_coll.find({}, opts);

    if (mongo_cursor.begin() != mongo_cursor.end())
    {
        auto doc = *mongo_cursor.begin();
        auto t = doc["message_id"];
        int tt = 0;
    }

    // int64_t cur_date = cur_date = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    // auto sender_ary = basic::make_array("tongstar");
    // mongo_coll.insert_one(basic::make_document(basic::kvp("message_id", 0),
    //                                            basic::kvp("sender_id", "tongstar"),
    //                                            basic::kvp("send_date", cur_date),
    //                                            basic::kvp("content_type", 1),
    //                                            basic::kvp("content", "helloworld"),
    //                                            basic::kvp("reader_id", sender_ary)));

    return 0;
}
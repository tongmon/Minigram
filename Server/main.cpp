#include "MessengerService.hpp"
#include "MongoDBPool.hpp"
#include "NetworkDefinition.hpp"
#include "PostgreDBPool.hpp"
#include "TCPClient.hpp"
#include "TCPServer.hpp"

#include <boost/system.hpp>

// stb는 header-only library이기에 매크로를 한번 정의하고 포함해줘야 한다.
// 매크로는 프로젝트에 단 한번만 쓰여야 됨
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

int main(int argc, char *argv[])
{
    try
    {
        // MongoDB 연결 정보 초기화
        MongoDBPool::Get({"localhost", "27017", "Minigram", "tongstar", "@Lsy12131213"});

        // PostgreDB 연결 정보 초기화
        PostgreDBPool::Get(4, {"localhost", "5432", "Minigram", "tongstar", "@Lsy12131213"});

        // TCP 서버 생성
        TCPServer<MessengerService> server(std::make_shared<TCPClient>(2), SERVER_PORT, 2);

        char a;
        std::cin >> a;
    }
    catch (boost::system::system_error &e)
    {
        std::cout << "Error occured! Error code = "
                  << e.code() << ". Message: "
                  << e.what();
    }

    return 0;
}
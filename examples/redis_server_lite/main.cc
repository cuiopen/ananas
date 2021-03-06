#include <iostream>
#include "RedisContext.h"
#include "net/EventLoop.h"
#include "net/Application.h"

#include "RedisLog.h"

std::shared_ptr<ananas::Logger> logger;

void OnConnect(std::shared_ptr<RedisContext> ctx, ananas::Connection* conn)
{
    std::cout << "RedisContext.OnConnect " << conn->Identifier() << std::endl;
}

void OnNewConnection(ananas::Connection* conn)
{
    std::cout << "OnNewConnection " << conn->Identifier() << std::endl;

    auto ctx = std::make_shared<RedisContext>(conn);

    conn->SetOnConnect(std::bind(&OnConnect, ctx, std::placeholders::_1));
    conn->SetOnMessage(std::bind(&RedisContext::OnRecvAll, ctx, std::placeholders::_1,
                                                             std::placeholders::_2,
                                                             std::placeholders::_3));
}

int main()
{
    ananas::LogManager::Instance().Start();
    logger = ananas::LogManager::Instance().CreateLog(logALL, logALL, "logger_server_test");

    const uint16_t port = 6379;

    auto& app = ananas::Application::Instance();
    app.Listen("loopback", port, OnNewConnection);
    app.Run();
    return 0;
}


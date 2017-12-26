
#include <signal.h>
#include <cstring>
#include <cstdio>

#include "Application.h"
#include "AnanasLogo.h"
#include "Socket.h"
#include "EventLoopGroup.h"
#include "log/Logger.h"

static void SignalHandler(int num)
{
    ananas::Application::Instance().Exit();
}

static void InitSignal()
{
    struct sigaction sig;
    ::memset(&sig, 0, sizeof(sig));
   
    sig.sa_handler = SignalHandler;
    sigaction(SIGINT, &sig, NULL);
                                  
    // ignore sigpipe
    sig.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sig, NULL);

#ifdef ANANAS_LOGO
    // logo
    printf("%s\n", ananas::internal::logo);
#endif
}


namespace ananas
{

Application::~Application()
{
}

Application& Application::Instance()
{
    static Application app;
    return app;
}

void Application::SetWorkerGroup(EventLoopGroup* group)
{
    assert (state_ == State::eS_None);
    assert (!worker_ && "why you need more EventLoopGroup");
    worker_ = group;
}

void Application::Run()
{
    if (state_ == State::eS_Stopped)
        return;

    if (worker_)
        worker_->Start();

    BaseLoop()->Run();

    baseGroup_->Wait();
    printf("Stopped BaseEventLoopGroup ...\n");

    if (worker_)
    {
        worker_->Wait();
        printf("Stopped WorkerEventLoopGroup...\n");
    }

    // thread safe exit
    LogManager::Instance().Stop();
}

void Application::Exit()
{
    if (state_ == State::eS_Stopped)
        return;

    state_ = State::eS_Stopped;
    baseGroup_->Stop();
    if (worker_)
        worker_->Stop();
}

bool Application::IsExit() const
{
    return state_ == State::eS_Stopped;
}
    
EventLoop* Application::BaseLoop()
{
    return &base_;
}

void Application::Listen(const SocketAddr& listenAddr,
                         NewTcpConnCallback cb,
                         BindFailCallback bfcb)
{
    auto loop = BaseLoop();
    assert (loop->IsInSameLoop());
    loop->Execute([loop, listenAddr, cb, bfcb]()
                  {
                    if (!loop->Listen(listenAddr, std::move(cb)))
                        bfcb(false, listenAddr);
                    else
                        bfcb(true, listenAddr);
                  });
}

void Application::Listen(const char* ip,
                         uint16_t hostPort,
                         NewTcpConnCallback cb,
                         BindFailCallback bfcb)
{
    SocketAddr addr(ip, hostPort);
    Listen(addr, std::move(cb), std::move(bfcb));
}

void Application::ListenUDP(const SocketAddr& addr,
                            UDPMessageCallback mcb,
                            UDPCreateCallback ccb,
                            BindFailCallback bfcb)
{
    auto loop = BaseLoop();
    assert (loop->IsInSameLoop());
    loop->Execute([loop, addr, mcb, ccb, bfcb]()
                  {
                    if (!loop->ListenUDP(addr, std::move(mcb), std::move(ccb)))
                        bfcb(false, addr);
                    else
                        bfcb(true, addr);
                  });
}

void Application::ListenUDP(const char* ip, uint16_t hostPort,
                            UDPMessageCallback mcb,
                            UDPCreateCallback ccb,
                            BindFailCallback bfcb)
{
    SocketAddr addr(ip, hostPort);
    ListenUDP(addr, std::move(mcb), std::move(ccb), std::move(bfcb));
}

void Application::CreateClientUDP(UDPMessageCallback mcb,
                                  UDPCreateCallback ccb)
{
    auto loop = BaseLoop();
    assert (loop->IsInSameLoop());
    loop->Execute([loop, mcb, ccb]()
                  {
                    loop->CreateClientUDP(std::move(mcb), std::move(ccb));
                  });
}

void Application::Connect(const SocketAddr& dst,
                          NewTcpConnCallback nccb,
                          TcpConnFailCallback cfcb,
                          DurationMs timeout)
{
    auto loop = BaseLoop();
    assert (loop->IsInSameLoop());
    loop->Execute([loop, dst, nccb, cfcb, timeout]()
                  {
                     loop->Connect(dst,
                                   std::move(nccb),
                                   std::move(cfcb),
                                   timeout);
                  });
}

void Application::Connect(const char* ip,
                          uint16_t hostPort,
                          NewTcpConnCallback nccb,
                          TcpConnFailCallback cfcb,
                          DurationMs timeout)
{
    SocketAddr dst(ip, hostPort);
    Connect(dst, std::move(nccb), std::move(cfcb), timeout);
}

    
EventLoop* Application::Next()
{
    auto loop = worker_ ? worker_->Next() : nullptr;
    if (loop)
        return loop;

    return BaseLoop();
}

Application::Application() :
    baseGroup_(new EventLoopGroup),
    base_(baseGroup_.get()),
    worker_(nullptr),
    state_ {State::eS_None}
{
    InitSignal();
}

} // end namespace ananas


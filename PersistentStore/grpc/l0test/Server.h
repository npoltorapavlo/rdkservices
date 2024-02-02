#pragma once

#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

template <typename ACTUALSERVICE>
class Server : public ACTUALSERVICE {
private:
    Server<ACTUALSERVICE>(const Server<ACTUALSERVICE>&) = delete;
    Server<ACTUALSERVICE>& operator=(const Server<ACTUALSERVICE>&) = delete;

public:
    template <typename... Args>
    Server(const std::string& uri, Args&&... args)
        : ACTUALSERVICE(std::forward<Args>(args)...)
    {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(uri, grpc::InsecureServerCredentials());
        builder.RegisterService(this);
        server = builder.BuildAndStart();
    }
    ~Server() override
    {
        server->Shutdown();
    }

private:
    std::unique_ptr<grpc::Server> server;
};

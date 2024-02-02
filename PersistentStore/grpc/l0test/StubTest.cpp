#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Stub.h"
#include "SecureStorageServiceMock.h"
#include "Server.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::IsTrue;
using ::testing::NiceMock;
using ::testing::Test;

using WPEFramework::Plugin::Grpc::Stub;

using distp::gateway::secure_storage::v1::GetValueRequest;
using distp::gateway::secure_storage::v1::GetValueResponse;
using distp::gateway::secure_storage::v1::Key;
using distp::gateway::secure_storage::v1::Scope;
using distp::gateway::secure_storage::v1::Value;

const std::string URI = "0.0.0.0:50051";

class AStub : public Test {
protected:
    std::unique_ptr<Server<NiceMock<SecureStorageServiceMock>>> server;
    std::unique_ptr<Stub> stub;
    void SetUp() override
    {
        WPEFramework::Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_URI"), URI);
        server.reset(new Server<NiceMock<SecureStorageServiceMock>>(URI));
        stub.reset(new Stub());
    }
};

TEST_F(AStub, GetsValue)
{
    const std::string value = "value_1";
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
    const Scope scope = Scope::SCOPE_ACCOUNT;
    GetValueRequest req;
    ON_CALL(*server, GetValue(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const GetValueRequest* request, GetValueResponse* response) {
                req = (*request);
                auto v = new Value();
                v->set_value(value);
                auto k = new Key();
                k->set_key(request->key().key());
                k->set_app_id(request->key().app_id());
                k->set_scope(request->key().scope());
                v->set_allocated_key(k);
                response->set_allocated_value(v);
                return grpc::Status::OK;
            }));

    grpc::ClientContext context;
    GetValueRequest request;
    auto k = new Key();
    k->set_key(key);
    k->set_app_id(app_id);
    k->set_scope(scope);
    request.set_allocated_key(k);
    GetValueResponse response;
    auto status = (*stub)->GetValue(&context, request, &response);
    ASSERT_THAT(status.ok(), IsTrue());
    ASSERT_THAT(req.has_key(), IsTrue());
    EXPECT_THAT(req.key().key(), Eq(key));
    EXPECT_THAT(req.key().app_id(), Eq(app_id));
    EXPECT_THAT(req.key().scope(), Eq(scope));
    ASSERT_THAT(response.has_value(), IsTrue());
    EXPECT_THAT(response.value().value(), Eq(value));
    ASSERT_THAT(response.value().has_key(), IsTrue());
    EXPECT_THAT(response.value().key().key(), Eq(key));
    EXPECT_THAT(response.value().key().app_id(), Eq(app_id));
    EXPECT_THAT(response.value().key().scope(), Eq(scope));
}

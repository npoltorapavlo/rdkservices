#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Store2Type.h"
#include "../Stub.h"
#include "SecureStorageServiceMock.h"
#include "Server.h"

using ::testing::_;
using ::testing::Eq;
using ::testing::Gt;
using ::testing::Invoke;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::Le;
using ::testing::NiceMock;
using ::testing::Test;

using WPEFramework::Exchange::IStore2;
using WPEFramework::Plugin::Grpc::Store2Type;
using WPEFramework::Plugin::Grpc::Stub;

using ::distp::gateway::secure_storage::v1::DeleteAllValuesRequest;
using ::distp::gateway::secure_storage::v1::DeleteAllValuesResponse;
using ::distp::gateway::secure_storage::v1::DeleteValueRequest;
using ::distp::gateway::secure_storage::v1::DeleteValueResponse;
using ::distp::gateway::secure_storage::v1::GetValueRequest;
using ::distp::gateway::secure_storage::v1::GetValueResponse;
using ::distp::gateway::secure_storage::v1::Key;
using ::distp::gateway::secure_storage::v1::Scope;
using ::distp::gateway::secure_storage::v1::UpdateValueRequest;
using ::distp::gateway::secure_storage::v1::UpdateValueResponse;
using ::distp::gateway::secure_storage::v1::Value;

const std::string URI = "0.0.0.0:50051";

class AStore2 : public Test {
protected:
    std::unique_ptr<Server<NiceMock<SecureStorageServiceMock>>> server;
    WPEFramework::Core::ProxyType<IStore2> store2;
    void SetUp() override
    {
        WPEFramework::Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_URI"), URI);
        server.reset(new Server<NiceMock<SecureStorageServiceMock>>(URI));
        store2 = WPEFramework::Core::ProxyType<Store2Type<Stub>>::Create();
    }
};

TEST_F(AStore2, GetsValue)
{
    const std::string value = "value_1";
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
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

    string v;
    uint32_t t;
    ASSERT_THAT(store2->GetValue(IStore2::ScopeType::ACCOUNT, app_id, key, v, t), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.has_key(), IsTrue());
    EXPECT_THAT(req.key().key(), Eq(key));
    EXPECT_THAT(req.key().app_id(), Eq(app_id));
    EXPECT_THAT(req.key().scope(), Eq(Scope::SCOPE_ACCOUNT));
    EXPECT_THAT(v, Eq(value));
}

TEST_F(AStore2, GetsValueWithTtl)
{
    const std::string value = "value_1";
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
    const uint32_t ttl = 100;
    GetValueRequest req;
    ON_CALL(*server, GetValue(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const GetValueRequest* request, GetValueResponse* response) {
                req = (*request);
                auto v = new Value();
                v->set_value(value);
                auto t = new google::protobuf::Duration();
                t->set_seconds(ttl);
                v->set_allocated_ttl(t);
                auto k = new Key();
                k->set_key(request->key().key());
                k->set_app_id(request->key().app_id());
                k->set_scope(request->key().scope());
                v->set_allocated_key(k);
                response->set_allocated_value(v);
                return grpc::Status::OK;
            }));

    string v;
    uint32_t t;
    ASSERT_THAT(store2->GetValue(IStore2::ScopeType::ACCOUNT, app_id, key, v, t), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.has_key(), IsTrue());
    EXPECT_THAT(req.key().key(), Eq(key));
    EXPECT_THAT(req.key().app_id(), Eq(app_id));
    EXPECT_THAT(req.key().scope(), Eq(Scope::SCOPE_ACCOUNT));
    EXPECT_THAT(v, Eq(value));
    EXPECT_THAT(t, Eq(ttl));
}

TEST_F(AStore2, GetsValueWithExpireTime)
{
    const std::string value = "value_1";
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
    const uint32_t ttl = 100;
    GetValueRequest req;
    ON_CALL(*server, GetValue(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const GetValueRequest* request, GetValueResponse* response) {
                req = (*request);
                auto v = new Value();
                v->set_value(value);
                auto t = new google::protobuf::Timestamp();
                t->set_seconds(ttl + (WPEFramework::Core::Time::Now().Ticks() / WPEFramework::Core::Time::TicksPerMillisecond / 1000));
                v->set_allocated_expire_time(t);
                auto k = new Key();
                k->set_key(request->key().key());
                k->set_app_id(request->key().app_id());
                k->set_scope(request->key().scope());
                v->set_allocated_key(k);
                response->set_allocated_value(v);
                return grpc::Status::OK;
            }));

    string v;
    uint32_t t;
    ASSERT_THAT(store2->GetValue(IStore2::ScopeType::ACCOUNT, app_id, key, v, t), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.has_key(), IsTrue());
    EXPECT_THAT(req.key().key(), Eq(key));
    EXPECT_THAT(req.key().app_id(), Eq(app_id));
    EXPECT_THAT(req.key().scope(), Eq(Scope::SCOPE_ACCOUNT));
    EXPECT_THAT(v, Eq(value));
    EXPECT_THAT(t, Le(ttl));
    EXPECT_THAT(t, Gt(0));
}

TEST_F(AStore2, SetsValue)
{
    const std::string value = "value_1";
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
    UpdateValueRequest req;
    ON_CALL(*server, UpdateValue(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const UpdateValueRequest* request, UpdateValueResponse*) {
                req = (*request);
                return grpc::Status::OK;
            }));

    ASSERT_THAT(store2->SetValue(IStore2::ScopeType::ACCOUNT, app_id, key, value, 0), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.has_value(), IsTrue());
    EXPECT_THAT(req.value().value(), Eq(value));
    ASSERT_THAT(req.value().has_key(), IsTrue());
    EXPECT_THAT(req.value().key().key(), Eq(key));
    EXPECT_THAT(req.value().key().app_id(), Eq(app_id));
    EXPECT_THAT(req.value().key().scope(), Eq(Scope::SCOPE_ACCOUNT));
    ASSERT_THAT(req.value().has_ttl(), IsFalse());
}

TEST_F(AStore2, SetsValueWithTtl)
{
    const std::string value = "value_1";
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
    const uint32_t ttl = 100;
    UpdateValueRequest req;
    ON_CALL(*server, UpdateValue(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const UpdateValueRequest* request, UpdateValueResponse*) {
                req = (*request);
                return grpc::Status::OK;
            }));

    ASSERT_THAT(store2->SetValue(IStore2::ScopeType::ACCOUNT, app_id, key, value, ttl), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.has_value(), IsTrue());
    EXPECT_THAT(req.value().value(), Eq(value));
    ASSERT_THAT(req.value().has_key(), IsTrue());
    EXPECT_THAT(req.value().key().key(), Eq(key));
    EXPECT_THAT(req.value().key().app_id(), Eq(app_id));
    EXPECT_THAT(req.value().key().scope(), Eq(Scope::SCOPE_ACCOUNT));
    ASSERT_THAT(req.value().has_ttl(), IsTrue());
    EXPECT_THAT(req.value().ttl().seconds(), Eq(ttl));
}

TEST_F(AStore2, DeletesKey)
{
    const std::string key = "key_1";
    const std::string app_id = "app_id_1";
    DeleteValueRequest req;
    ON_CALL(*server, DeleteValue(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const DeleteValueRequest* request, DeleteValueResponse*) {
                req = (*request);
                return grpc::Status::OK;
            }));

    ASSERT_THAT(store2->DeleteKey(IStore2::ScopeType::ACCOUNT, app_id, key), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.has_key(), IsTrue());
    EXPECT_THAT(req.key().key(), Eq(key));
    EXPECT_THAT(req.key().app_id(), Eq(app_id));
    EXPECT_THAT(req.key().scope(), Eq(Scope::SCOPE_ACCOUNT));
}

TEST_F(AStore2, DeletesNamespace)
{
    const std::string app_id = "app_id_1";
    DeleteAllValuesRequest req;
    ON_CALL(*server, DeleteAllValues(_, _, _))
        .WillByDefault(Invoke(
            [&](::grpc::ServerContext*, const DeleteAllValuesRequest* request, DeleteAllValuesResponse*) {
                req = (*request);
                return grpc::Status::OK;
            }));

    ASSERT_THAT(store2->DeleteNamespace(IStore2::ScopeType::ACCOUNT, app_id), Eq(WPEFramework::Core::ERROR_NONE));
    ASSERT_THAT(req.app_id(), Eq(app_id));
    EXPECT_THAT(req.scope(), Eq(Scope::SCOPE_ACCOUNT));
}

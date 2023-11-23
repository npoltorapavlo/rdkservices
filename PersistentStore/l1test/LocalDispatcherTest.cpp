#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../PersistentStore.h"

#include "ServiceMock.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::IsTrue;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Test;

const std::string Path = "/tmp/persistentstore/l1test/localdispatchertest";
const uint32_t MaxSize = 100;
const uint32_t MaxValue = 3;
const uint32_t Limit = 100;

class ALocalDispatcher : public Test {
protected:
    NiceMock<ServiceMock> service;
    PluginHost::IPlugin* plugin;
    PluginHost::ILocalDispatcher* dispatcher;
    virtual ~ALocalDispatcher() override = default;
    virtual void SetUp() override
    {
        Core::File(Path).Destroy();

        plugin = Core::Service<PersistentStore>::Create<PluginHost::IPlugin>();
        dispatcher = plugin->QueryInterface<PluginHost::ILocalDispatcher>();
        ASSERT_THAT(dispatcher, NotNull());

        JsonObject config;
        config["path"] = Path;
        config["maxsize"] = MaxSize;
        config["maxvalue"] = MaxValue;
        config["limit"] = Limit;
        string configJsonStr;
        config.ToString(configJsonStr);

        ON_CALL(service, ConfigLine())
            .WillByDefault(
                ::testing::Return(configJsonStr));

        ASSERT_THAT(plugin->Initialize(&service), Eq(""));
    }
    virtual void TearDown() override
    {
        plugin->Deinitialize(&service);

        if (dispatcher)
            dispatcher->Release();
        plugin->Release();
    }
};

TEST_F(ALocalDispatcher, DoesNotGetValueInUnknownNamespace)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"x\",\"key\":\"x\"}", response), Eq(Core::ERROR_NOT_EXIST));
}

TEST_F(ALocalDispatcher, DoesNotGetValueWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"key\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotSetEmptyNamespace)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"\",\"key\":\"x\",\"value\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotSetEmptyKey)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"x\",\"key\":\"\",\"value\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotSetWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"key\":\"x\",\"value\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"x\",\"value\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"x\",\"key\":\"x\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotGetKeysWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getKeys", "", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, GetsKeysForUnknownNamespaceWithoutError)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getKeys", "{\"namespace\":\"ns1\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"success\":true}"));
}

TEST_F(ALocalDispatcher, GetsNamespaces)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getNamespaces", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"success\":true}"));
}

TEST_F(ALocalDispatcher, GetsStorageSizes)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getStorageSizes", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{}"));
}

// Deprecated
TEST_F(ALocalDispatcher, GetsStorageSize)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getStorageSize", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"namespaceSizes\":{},\"success\":true}"));
}

TEST_F(ALocalDispatcher, DoesNotGetNamespaceStorageLimitForUnknownNamespace)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getNamespaceStorageLimit", "{\"namespace\":\"x\"}", response), Eq(Core::ERROR_NOT_EXIST));
}

TEST_F(ALocalDispatcher, DoesNotGetStorageLimitWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getNamespaceStorageLimit", "", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotSetStorageLimitWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setNamespaceStorageLimit", "", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotDeleteKeyWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "deleteKey", "{\"key\":\"k1\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "deleteKey", "{\"namespace\":\"ns1\"}", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(ALocalDispatcher, DoesNotDeleteNamespaceWhenMandatoryParamsAreMissing)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "deleteNamespace", "", response), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

class ALocalDispatcherWithValues : public ALocalDispatcher {
protected:
    virtual ~ALocalDispatcherWithValues() override = default;
    virtual void SetUp() override
    {
        ALocalDispatcher::SetUp();

        string response;
        ASSERT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"ns1\",\"key\":\"k1\",\"value\":\"v1\"}", response), Eq(Core::ERROR_NONE));
        ASSERT_THAT(response, Eq("{\"success\":true}"));
        ASSERT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"ns1\",\"key\":\"k2\",\"value\":\"v2\"}", response), Eq(Core::ERROR_NONE));
        ASSERT_THAT(response, Eq("{\"success\":true}"));
        ASSERT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"ns2\",\"key\":\"k3\",\"value\":\"v3\"}", response), Eq(Core::ERROR_NONE));
        ASSERT_THAT(response, Eq("{\"success\":true}"));
    }
};

TEST_F(ALocalDispatcherWithValues, GetsValues)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"k1\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"value\":\"v1\",\"success\":true}"));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"k2\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"value\":\"v2\",\"success\":true}"));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns2\",\"key\":\"k3\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"value\":\"v3\",\"success\":true}"));
}

TEST_F(ALocalDispatcherWithValues, DoesNotGetUnknownKey)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"x\"}", response), Eq(Core::ERROR_UNKNOWN_KEY));
}

TEST_F(ALocalDispatcherWithValues, GetsKeys)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getKeys", "{\"namespace\":\"ns1\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"keys\":[\"k1\",\"k2\"],\"success\":true}"));
}

TEST_F(ALocalDispatcherWithValues, GetsNamespaces)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getNamespaces", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"namespaces\":[\"ns1\",\"ns2\"],\"success\":true}"));
}

TEST_F(ALocalDispatcherWithValues, GetsStorageSizes)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getStorageSizes", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"storageList\":[{\"namespace\":\"ns1\",\"size\":8},{\"namespace\":\"ns2\",\"size\":4}]}"));
}

// Deprecated
TEST_F(ALocalDispatcherWithValues, GetsStorageSize)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getStorageSize", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"namespaceSizes\":{\"ns1\":8,\"ns2\":4},\"success\":true}"));
}

TEST_F(ALocalDispatcherWithValues, DoesNotGetDeletedKey)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "deleteKey", "{\"namespace\":\"ns1\",\"key\":\"k1\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"success\":true}"));

    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"k1\"}", response), Eq(Core::ERROR_UNKNOWN_KEY));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"k2\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"value\":\"v2\",\"success\":true}"));
}

TEST_F(ALocalDispatcherWithValues, DoesNotGetKeyInDeletedNamespace)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "deleteNamespace", "{\"namespace\":\"ns1\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"success\":true}"));

    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"k1\"}", response), Eq(Core::ERROR_NOT_EXIST));
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"ns1\",\"key\":\"k2\"}", response), Eq(Core::ERROR_NOT_EXIST));
}

TEST_F(ALocalDispatcherWithValues, FlushesCacheWithoutError)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "flushCache", "", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"success\":true}"));
}

class ALocalDispatcherWithValuesAndLimit : public ALocalDispatcherWithValues {
protected:
    virtual ~ALocalDispatcherWithValuesAndLimit() override = default;
    virtual void SetUp() override
    {
        ALocalDispatcherWithValues::SetUp();

        string response;
        EXPECT_THAT(dispatcher->Invoke(0, 0, "", "setNamespaceStorageLimit", "{\"namespace\":\"ns1\",\"storageLimit\":\"10\"}", response), Eq(Core::ERROR_NONE));
    }
};

TEST_F(ALocalDispatcherWithValuesAndLimit, GetsNamespaceStorageLimit)
{
    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getNamespaceStorageLimit", "{\"namespace\":\"ns1\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"storageLimit\":10}"));
}

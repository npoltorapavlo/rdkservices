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

const std::string Path = "/tmp/persistentstore/l1test/legacypathtest";
const std::string LegacyPath = "/tmp/persistentstore/l1test/legacypathtest1";
const uint32_t MaxSize = 100;
const uint32_t MaxValue = 3;
const uint32_t Limit = 100;

class ALegacyPathWithValues : public Test {
protected:
    virtual ~ALegacyPathWithValues() override = default;
    virtual void SetUp() override
    {
        Core::File(LegacyPath).Destroy();

        PluginHost::IPlugin* plugin = Core::Service<PersistentStore>::Create<PluginHost::IPlugin>();
        PluginHost::ILocalDispatcher* dispatcher = plugin->QueryInterface<PluginHost::ILocalDispatcher>();
        ASSERT_THAT(dispatcher, NotNull());

        NiceMock<ServiceMock> service;

        JsonObject config;
        config["path"] = LegacyPath;
        config["maxsize"] = MaxSize;
        config["maxvalue"] = MaxValue;
        config["limit"] = Limit;
        string configJsonStr;
        config.ToString(configJsonStr);

        ON_CALL(service, ConfigLine())
            .WillByDefault(
                ::testing::Return(configJsonStr));

        ASSERT_THAT(plugin->Initialize(&service), Eq(""));

        string response;
        ASSERT_THAT(dispatcher->Invoke(0, 0, "", "setValue", "{\"namespace\":\"x\",\"key\":\"y\",\"value\":\"z\"}", response), Eq(Core::ERROR_NONE));
        ASSERT_THAT(response, Eq("{\"success\":true}"));

        plugin->Deinitialize(&service);

        dispatcher->Release();
        plugin->Release();
    }
};

TEST_F(ALegacyPathWithValues, IsCopiedByThePluginInitialize)
{
    Core::File(Path).Destroy();
    EXPECT_THAT(Core::File(Path).Exists(), IsFalse());
    EXPECT_THAT(Core::File(LegacyPath).Exists(), IsTrue());

    PluginHost::IPlugin* plugin = Core::Service<PersistentStore>::Create<PluginHost::IPlugin>();
    PluginHost::ILocalDispatcher* dispatcher = plugin->QueryInterface<PluginHost::ILocalDispatcher>();
    ASSERT_THAT(dispatcher, NotNull());

    NiceMock<ServiceMock> service;

    JsonObject config;
    config["path"] = Path;
    config["legacypath"] = LegacyPath;
    config["maxsize"] = MaxSize;
    config["maxvalue"] = MaxValue;
    config["limit"] = Limit;
    string configJsonStr;
    config.ToString(configJsonStr);

    ON_CALL(service, ConfigLine())
        .WillByDefault(
            ::testing::Return(configJsonStr));

    ASSERT_THAT(plugin->Initialize(&service), Eq(""));

    string response;
    EXPECT_THAT(dispatcher->Invoke(0, 0, "", "getValue", "{\"namespace\":\"x\",\"key\":\"y\"}", response), Eq(Core::ERROR_NONE));
    EXPECT_THAT(response, Eq("{\"value\":\"z\",\"success\":true}"));

    plugin->Deinitialize(&service);

    EXPECT_THAT(Core::File(Path).Exists(), IsTrue());
    EXPECT_THAT(Core::File(LegacyPath).Exists(), IsFalse());

    dispatcher->Release();
    plugin->Release();
}

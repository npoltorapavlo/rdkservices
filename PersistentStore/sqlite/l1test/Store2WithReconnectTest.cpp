#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Handle.h"
#include "../Store2Type.h"
#include "../Store2WithReconnectType.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

using ::testing::Eq;

const std::string Path = "/tmp/sqlite/l1test/store2withreconnecttest";
const uint32_t MaxSize = 100;
const uint32_t MaxValue = 10;
const uint32_t Limit = 50;

TEST(Store, SetsValueAfterFileDestroy)
{
    Core::File(Path).Destroy();
    // File is destroyed

    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_PATH"), Path);
    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXSIZE"), std::to_string(MaxSize));
    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXVALUE"), std::to_string(MaxValue));
    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_LIMIT"), std::to_string(Limit));
    auto store = Core::Service<Sqlite::Store2WithReconnectType<Sqlite::Store2Type<Sqlite::Handle>>>::Create<Exchange::IStore2>();

    ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_NONE));

    Core::File(Path).Destroy();
    // File is destroyed when sqlite connection is open

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key2", "value2", 0), Eq(Core::ERROR_NONE));
    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_UNKNOWN_KEY));
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key2", value, ttl), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq("value2"));
    EXPECT_THAT(ttl, Eq(0));

    store->Release();
}

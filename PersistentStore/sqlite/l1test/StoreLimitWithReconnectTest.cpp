#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Handle.h"
#include "../StoreLimitType.h"
#include "../StoreLimitWithReconnectType.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

using ::testing::Eq;

const std::string Path = "/tmp/sqlite/l1test/storelimitwithreconnecttest";
const uint32_t MaxSize = 100;
const uint32_t MaxValue = 10;
const uint32_t Limit = 50;

TEST(Store, SetsLimitAfterFileDestroy)
{
    Core::File(Path).Destroy();
    // File is destroyed

    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_PATH"), Path);
    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXSIZE"), std::to_string(MaxSize));
    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXVALUE"), std::to_string(MaxValue));
    Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_LIMIT"), std::to_string(Limit));
    auto limit = Core::Service<Sqlite::StoreLimitWithReconnectType<Sqlite::StoreLimitType<Sqlite::Handle>>>::Create<Exchange::IStoreLimit>();

    Core::File(Path).Destroy();
    // File is destroyed when sqlite connection is open

    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", 20), Eq(Core::ERROR_NONE));
    uint32_t value;
    EXPECT_THAT(limit->GetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", value), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq(20));

    limit->Release();
}

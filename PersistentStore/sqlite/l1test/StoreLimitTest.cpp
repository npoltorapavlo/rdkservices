#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Handle.h"
#include "../Store2Type.h"
#include "../StoreLimitType.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

using ::testing::Eq;
using ::testing::Test;

const std::string Path = "/tmp/sqlite/l1test/storelimittest";
const uint32_t MaxSize = 100;
const uint32_t MaxValue = 10;
const uint32_t Limit = 50;

class AStoreLimit : public Test {
protected:
    Exchange::IStoreLimit* limit;
    ~AStoreLimit() override = default;
    void SetUp() override
    {
        Core::File(Path).Destroy();
        // File is destroyed

        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_PATH"), Path);
        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXSIZE"), std::to_string(MaxSize));
        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXVALUE"), std::to_string(MaxValue));
        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_LIMIT"), std::to_string(Limit));
        limit = Core::Service<Sqlite::StoreLimitType<Sqlite::Handle>>::Create<Exchange::IStoreLimit>();
    }
    void TearDown() override
    {
        limit->Release();
    }
};

TEST_F(AStoreLimit, SetsLimitForUnknownNamespaceWithoutError)
{
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "x", 10), Eq(Core::ERROR_NONE));
}

TEST_F(AStoreLimit, DoesNotGetLimitForUnknownNamespace)
{
    uint32_t value;
    EXPECT_THAT(limit->GetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "x", value), Eq(Core::ERROR_NOT_EXIST));
}

TEST_F(AStoreLimit, DoesNotSetLimitForTooLargeNamespaceName)
{
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "12345678901", 10), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStoreLimit, DoesNotSetLimitWhenReachesMaxSize)
{
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567890", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567891", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567892", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567893", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567894", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567895", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567896", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567897", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567898", 10), Eq(Core::ERROR_NONE));
    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "1234567899", 10), Eq(Core::ERROR_NONE));
    // Size is 100

    EXPECT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "x", 10), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

class AStoreLimitWithValues : public AStoreLimit {
protected:
    ~AStoreLimitWithValues() override = default;
    void SetUp() override
    {
        AStoreLimit::SetUp();
        auto store = Core::Service<Sqlite::Store2Type<Sqlite::Handle>>::Create<Exchange::IStore2>();
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key2", "value2", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns2", "key1", "value1", 0), Eq(Core::ERROR_NONE));
        store->Release();
    }
};

TEST_F(AStoreLimitWithValues, GetsDefaultLimitForExistingNamespace)
{
    uint32_t value;
    EXPECT_THAT(limit->GetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", value), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq(Limit));
}

class AStoreLimitWithValuesAndLimit : public AStoreLimitWithValues {
protected:
    ~AStoreLimitWithValuesAndLimit() override = default;
    void SetUp() override
    {
        AStoreLimitWithValues::SetUp();
        ASSERT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", 30), Eq(Core::ERROR_NONE));
        // Namespace size is 20, limit 30
    }
};

TEST_F(AStoreLimitWithValuesAndLimit, GetsLimit)
{
    uint32_t value;
    EXPECT_THAT(limit->GetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", value), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq(30));
}

TEST_F(AStoreLimitWithValuesAndLimit, UpdatesLimit)
{
    ASSERT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", 40), Eq(Core::ERROR_NONE));
    uint32_t value;
    EXPECT_THAT(limit->GetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", value), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq(40));
}

TEST_F(AStoreLimitWithValuesAndLimit, DoesNotSetValueWhenReachesLimit)
{
    auto store = Core::Service<Sqlite::Store2Type<Sqlite::Handle>>::Create<Exchange::IStore2>();

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key3", "value3", 0), Eq(Core::ERROR_NONE));
    // Namespace size is 30, limit (30) reached

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key4", "value4", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));

    store->Release();
}

TEST_F(AStoreLimitWithValuesAndLimit, SetsValueAfterReachesAndUpdatesLimit)
{
    auto store = Core::Service<Sqlite::Store2Type<Sqlite::Handle>>::Create<Exchange::IStore2>();

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key3", "value3", 0), Eq(Core::ERROR_NONE));
    // Namespace size is 30, limit (30) reached
    ASSERT_THAT(limit->SetNamespaceStorageLimit(Exchange::IStore2::ScopeType::DEVICE, "ns1", 40), Eq(Core::ERROR_NONE));
    // Namespace size is 30, limit updated to 40

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key4", "value4", 0), Eq(Core::ERROR_NONE));

    store->Release();
}

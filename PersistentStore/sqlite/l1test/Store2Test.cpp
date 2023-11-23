#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../Handle.h"
#include "../Store2Type.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

using ::testing::Eq;
using ::testing::Le;
using ::testing::Test;

const std::string Path = "/tmp/sqlite/l1test/store2test";
const uint32_t MaxSize = 100;
const uint32_t MaxValue = 10;
const uint32_t Limit = 50;

class AStore : public Test {
protected:
    Exchange::IStore2* store;
    ~AStore() override = default;
    void SetUp() override
    {
        Core::File(Path).Destroy();
        // File is destroyed

        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_PATH"), Path);
        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXSIZE"), std::to_string(MaxSize));
        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_MAXVALUE"), std::to_string(MaxValue));
        Core::SystemInfo::SetEnvironment(_T("PERSISTENTSTORE_LIMIT"), std::to_string(Limit));
        store = Core::Service<Sqlite::Store2Type<Sqlite::Handle>>::Create<Exchange::IStore2>();
    }
    void TearDown() override
    {
        store->Release();
    }
};

TEST_F(AStore, DoesNotSetEmptyNamespace)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "", "x", "x", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStore, DoesNotSetEmptyKey)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "x", "", "x", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStore, DoesNotSetTooLargeNamespaceName)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "12345678901", "x", "x", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStore, DoesNotSetTooLargeKey)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "x", "12345678901", "x", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStore, DoesNotSetTooLargeValue)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "x", "x", "12345678901", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStore, DoesNotGetValueWhenEmpty)
{
    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "x", "x", value, ttl), Eq(Core::ERROR_NOT_EXIST));
}

TEST_F(AStore, DeletesUnknownKeyWithoutError)
{
    EXPECT_THAT(store->DeleteKey(Exchange::IStore2::ScopeType::DEVICE, "x", "x"), Eq(Core::ERROR_NONE));
}

TEST_F(AStore, DeletesUnknownNamespaceWithoutError)
{
    EXPECT_THAT(store->DeleteNamespace(Exchange::IStore2::ScopeType::DEVICE, "x"), Eq(Core::ERROR_NONE));
}

TEST_F(AStore, SetsEmptyValueWithoutError)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "", 0), Eq(Core::ERROR_NONE));
    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq(""));
    EXPECT_THAT(ttl, Eq(0));
}

TEST_F(AStore, GetsValueWithTtl)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 100), Eq(Core::ERROR_NONE));
    // Value with ttl added

    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq("value1"));
    EXPECT_THAT(ttl, Le(100));
}

TEST_F(AStore, DoesNotGetValueWhenTtlExpires)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 1), Eq(Core::ERROR_NONE));
    // Value with ttl added
    Core::Event lock(false, true);
    lock.Lock(1000);
    // Value with ttl expired

    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_UNKNOWN_KEY));
}

class StoreNotification : public Exchange::IStore2::INotification {
public:
    Core::Event valueChanged;
    StoreNotification()
        : valueChanged(false, true)
    {
    }
    void ValueChanged(const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, const string& value) override
    {
        EXPECT_THAT(scope, Eq(Exchange::IStore2::ScopeType::DEVICE));
        EXPECT_THAT(ns, Eq("ns1"));
        EXPECT_THAT(key, Eq("key1"));
        EXPECT_THAT(value, Eq("value1"));
        valueChanged.SetEvent();
    }

    BEGIN_INTERFACE_MAP(StoreNotification)
    INTERFACE_ENTRY(Exchange::IStore2::INotification)
    END_INTERFACE_MAP
};

TEST_F(AStore, TriggersNotificationWhenValueIsSet)
{
    Core::Sink<StoreNotification> sink;
    store->Register(&sink);

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_NONE));
    // Value is set

    EXPECT_THAT(sink.valueChanged.Lock(100), Eq(Core::ERROR_NONE));

    store->Unregister(&sink);
}

class AStoreWithValues : public AStore {
protected:
    ~AStoreWithValues() override = default;
    void SetUp() override
    {
        AStore::SetUp();
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_NONE));
    }
};

TEST_F(AStoreWithValues, DoesNotGetUnknownValueInExistingNamespace)
{
    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "x", value, ttl), Eq(Core::ERROR_UNKNOWN_KEY));
}

TEST_F(AStoreWithValues, DeletesUnknownKeyWithoutError)
{
    EXPECT_THAT(store->DeleteKey(Exchange::IStore2::ScopeType::DEVICE, "ns1", "x"), Eq(Core::ERROR_NONE));
}

TEST_F(AStoreWithValues, GetsValue)
{
    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq("value1"));
    EXPECT_THAT(ttl, Eq(0));
}

TEST_F(AStoreWithValues, UpdatesValue)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value2", 0), Eq(Core::ERROR_NONE));
    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_NONE));
    EXPECT_THAT(value, Eq("value2"));
    EXPECT_THAT(ttl, Eq(0));
}

TEST_F(AStoreWithValues, DoesNotGetDeletedValue)
{
    EXPECT_THAT(store->DeleteKey(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1"), Eq(Core::ERROR_NONE));
    // Value is deleted

    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_UNKNOWN_KEY));
}

TEST_F(AStoreWithValues, DoesNotGetValueInDeletedNamespace)
{
    EXPECT_THAT(store->DeleteNamespace(Exchange::IStore2::ScopeType::DEVICE, "ns1"), Eq(Core::ERROR_NONE));
    // Namespace is deleted

    string value;
    uint32_t ttl;
    EXPECT_THAT(store->GetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", value, ttl), Eq(Core::ERROR_NOT_EXIST));
}

class AStoreThatReachedMaxSize : public AStore {
protected:
    ~AStoreThatReachedMaxSize() override = default;
    void SetUp() override
    {
        AStore::SetUp();
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "1234567890", "1234567890", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns2", "1234567890", "1234567890", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns3", "1234567890", "1234567890", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns4", "1234567890", "1234567890", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns5", "1234", "1", 0), Eq(Core::ERROR_NONE));
        // Size is 100
    }
};

TEST_F(AStoreThatReachedMaxSize, DoesNotSetValue)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns2", "key1", "value1", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStoreThatReachedMaxSize, SetsValueAfterDeletesValue)
{
    EXPECT_THAT(store->DeleteKey(Exchange::IStore2::ScopeType::DEVICE, "ns1", "1234567890"), Eq(Core::ERROR_NONE));
    // Size is 80

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_NONE));
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns2", "key1", "value1", 0), Eq(Core::ERROR_NONE));
}

class AStoreThatReachedLimit : public AStore {
protected:
    ~AStoreThatReachedLimit() override = default;
    void SetUp() override
    {
        AStore::SetUp();
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "1234567890", "1234567890", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "1234567891", "1234567891", 0), Eq(Core::ERROR_NONE));
        ASSERT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "123456", "1", 0), Eq(Core::ERROR_NONE));
        // Namespace size is 50
    }
};

TEST_F(AStoreThatReachedLimit, DoesNotSetValue)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_INVALID_INPUT_LENGTH));
}

TEST_F(AStoreThatReachedLimit, SetsValueInAnotherNamespaceWithoutError)
{
    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns2", "key1", "value1", 0), Eq(Core::ERROR_NONE));
}

TEST_F(AStoreThatReachedLimit, SetsValueAfterDeletesValue)
{
    EXPECT_THAT(store->DeleteKey(Exchange::IStore2::ScopeType::DEVICE, "ns1", "1234567890"), Eq(Core::ERROR_NONE));
    // Namespace size is 30

    EXPECT_THAT(store->SetValue(Exchange::IStore2::ScopeType::DEVICE, "ns1", "key1", "value1", 0), Eq(Core::ERROR_NONE));
}

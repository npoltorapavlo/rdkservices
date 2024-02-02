#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "secure_storage.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using distp::gateway::secure_storage::v1::GetValueRequest;
using distp::gateway::secure_storage::v1::GetValueResponse;
using distp::gateway::secure_storage::v1::Key;
using distp::gateway::secure_storage::v1::SecureStorageService;
using distp::gateway::secure_storage::v1::UpdateValueRequest;
using distp::gateway::secure_storage::v1::UpdateValueResponse;
using distp::gateway::secure_storage::v1::Value;
using distp::gateway::secure_storage::v1::Scope;

const std::string SAT = "eyJraWQiOiI3MGQzNjk1MS05YzRiLTQ5NzctYWIxMS1hMWY3YTgzOTQxMzkiLCJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJwcmluY2lwYWwiOnsiZGdkIjpbImJiYTBjNDY0LTE1ODktNGY5NC1hOTJkLWYwOGQ4MmY5MTI5OCJdLCJ6aXAiOlsiIFRXNyAgNVFEIl0sInRteiI6WyJFdXJvcGUvTG9uZG9uIl0sImFzdCI6WyJBY3RpdmUiXSwibGloIjpbInRydWUiXSwiZW50IjpbInNlcnZpY2U6eGNhbDpDRFZSIiwic2VydmljZTp4Y2FsOnZpZGVvOnN0YXRlIl0sInBpZCI6WyJza3ktdWsiXSwibG9wIjpbInRydWUiXSwic2lkIjpbIkJFVEFURVNUNDUyMDc2NTgxNjAiXSwibGlwIjpbIjE5NS4xMzguODIuOTQiXSwiYmlkIjpbIkJFVEFURVNUNDUyMDc2NTgxNjAiXSwiYWlkIjpbIjM4MzI3OTIzMzg1MzY0MTUzMzAiXSwiZGlkIjpbIjU4NzcwOTUzMDY1ODIxODIwMTAiXX0sInN1YiI6IngxOnhiby1ib3dzOmZhNTRlZiIsIm5iZiI6MTcwNjgwMDAyMiwiY2FwYWJpbGl0aWVzIjpbIngxOnNhdHM6eGJvOnRva2VuOmNyZWF0ZSIsIngxOnZpZGVvLWNsaWVudCJdLCJpc3MiOiJzYXRzLWV1LXByb2R1Y3Rpb24iLCJhbGxvd2VkUmVzb3VyY2VzIjp7ImFsbG93ZWREZXZpY2VJZHMiOlsiNTg3NzA5NTMwNjU4MjE4MjAxMCJdLCJhbGxvd2VkUGFydG5lcnMiOlsic2t5LXVrIl0sImFsbG93ZWRTZXJ2aWNlQWNjb3VudElkcyI6WyIzODMyNzkyMzM4NTM2NDE1MzMwIl19LCJleHAiOjE3MDcwNTkyMjUsImlhdCI6MTcwNjgwMDAyMiwidmVyc2lvbiI6IjIuMCIsImp0aSI6IjAxOGQ2NTM1LTkxNTktNzgyMC04OTk2LTY4NzdkYWI3NGI1ZSJ9.lzrUljcsO5YLyLa-WElrvJKAzIR8tlsnPWo8UB7C_Dsvr4zoQT4pAVtS7xs42MBBpPelLKsA6Z9daWdrDFY2zwlrNWy8nYxKzUxe5q85UIggkWcGn0lknY4oIYSe_YAXHRaOW8uuRkFpyQYaNFZyjYn4_Y14Lc1Z56nQox2ve7XmT4ORw5OdLKfaQ6vUR7yZIHJyLDR3_-G3vloWL3F6uVH_f7SAnw1Yatn4RHkxTx0VLOdxws43Ld7eQHKh8rcWlB6XLZOruV1ovOXi5nm7CymqSsj9Xs_5ks62kwMDmrhy4zmUsD099h2ahflPMKgd1QiPnt_GIUVxw78b7lzIXg";

class Client {
public:
    Client(std::shared_ptr<Channel> channel)
        : stub_(SecureStorageService::NewStub(channel))
    {
    }

    bool GetValue(const std::string& ns, const std::string& key, std::string& value) const
    {
        ClientContext context;
        GetValueRequest request;
        Key* k = new Key();
        k->set_app_id(ns);
        k->set_key(key);
        k->set_scope(Scope::SCOPE_ACCOUNT);
        request.set_allocated_key(k);
        GetValueResponse response;
        Status status = stub_->GetValue(&context, request, &response);
        if (!status.ok()) {
            printf("GetValue rpc failed. %d %s %s\n",
                (int)status.error_code(),
                status.error_message().c_str(),
                status.error_details().c_str());
            return false;
        }
        if (!response.has_value()) {
            printf("Server returns incomplete value.\n");
            return false;
        }
        value = response.value().value();
        return true;
    }

    bool UpdateValue(const std::string& ns, const std::string& key, const std::string& value)
    {
        ClientContext context;
        UpdateValueRequest request;
        Value* v = new Value();
        v->set_value(value);
        Key* k = new Key();
        k->set_app_id(ns);
        k->set_key(key);
        k->set_scope(Scope::SCOPE_ACCOUNT);
        v->set_allocated_key(k);
        request.set_allocated_value(v);
        UpdateValueResponse response;
        Status status = stub_->UpdateValue(&context, request, &response);
        if (!status.ok()) {
            printf("UpdateValue rpc failed. %d %s %s\n",
                (int)status.error_code(),
                status.error_message().c_str(),
                status.error_details().c_str());
            return false;
        }
        return true;
    }

private:
    std::unique_ptr<SecureStorageService::Stub> stub_;
};

TEST(A, B)
{
    auto creds = grpc::CompositeChannelCredentials(
        grpc::SslCredentials(grpc::SslCredentialsOptions()),
        grpc::AccessTokenCredentials(SAT));
    Client client(grpc::CreateChannel(
        "ss.eu.prod.developer.comcast.com:443",
        creds));

    printf("-------------- UpdateValue --------------\n");
    client.UpdateValue("test_123", "key_123", "value_123");
    printf("-------------- GetValue --------------\n");
    std::string value;
    client.GetValue("test_123", "key_123", value);
    printf("value=%s\n", value.c_str());
}

/*
 * SAT is base64 token with capabilities, ids, expiration, signature, etc.
 * EU prod secure_storage service requires SAT capabilities "x1:video-client".
 * Device topken configparamgen jx /opt/lxy/nivgmjmtpvcd
 * {"status":0,"token":"...","expires":86400,"received":1706712642,"received_usec":293227}
 */



/*
 * ubuntu23
sudo apt  install golang-go
go install github.com/fullstorydev/grpcui/cmd/grpcui@latest
 * ~/go/bin/grpcui -vvv -insecure ss.eu.prod.developer.comcast.com:443
 *
 * git clone git@github.com:googleapis/googleapis.git
npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ cd googleapis/
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ make LANGUAGE=cpp all
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ sudo apt install make
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ make LANGUAGE=cpp all
find: ‘/usr/local/include/google/protobuf’: No such file or directory
mkdir -p ./gens
protoc --proto_path=.:/usr/local/include --cpp_out=./gens --grpc_out=./gens --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin google/genomics/v1/operations.proto
make: protoc: No such file or directory
make: *** [Makefile:45: google/genomics/v1/operations.pb.cc] Error 127
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ protoc
Command 'protoc' not found, but can be installed with:
See 'snap info protobuf' for additional versions.
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ sudo apt  install protobuf-compiler
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ make LANGUAGE=cpp all
find: ‘/usr/local/include/google/protobuf’: No such file or directory
mkdir -p ./gens
protoc --proto_path=.:/usr/local/include --cpp_out=./gens --grpc_out=./gens --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin google/genomics/v1/operations.proto
google/genomics/v1/operations.proto:19:1: warning: Import google/api/annotations.proto is unused.
/usr/local/bin/grpc_cpp_plugin: program not found or is not executable
Please specify a program using absolute path or make sure the program is available in your PATH system variable
--grpc_out: protoc-gen-grpc: Plugin failed with status code 1.
make: *** [Makefile:45: google/genomics/v1/operations.pb.cc] Error 1
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/googleapis$ sudo apt -y install protobuf-compiler-grpc
make LANGUAGE=cpp GRPCPLUGIN=/usr/bin/grpc_cpp_plugin all
.....
protoc --proto_path=.:/usr/local/include --cpp_out=./gens --grpc_out=./gens --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin google/maps/routes/v1/route_service.proto
[libprotobuf FATAL google/protobuf/io/printer.cc:145]  Unclosed variable name.
terminate called after throwing an instance of 'google::protobuf::FatalException'
  what():   Unclosed variable name.
--grpc_out: protoc-gen-grpc: Plugin killed by signal 6.
make: *** [Makefile:45: google/maps/routes/v1/route_service.pb.cc] Error 1
?????????????????????????????
sed -i 's/\$fields/fields/' google/maps/routing/v2/routes_service.proto
!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ cmake -S rdkservices/PersistentStore/grpc/l2test -B build/persistentstoregrpcl2test -DCMAKE_INSTALL_PREFIX="install/usr" -DCMAKE_CXX_FLAGS="-Wall -Werror"
None of the required 'grpc' found
npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ sudo apt -y install libgrpc++-dev
-- Could NOT find c-ares (missing: c-ares_DIR)
-- Found RE2 via pkg-config.
CMake Error at CMakeLists.txt:36 (find_package):
  Found package configuration file:
/usr/lib/x86_64-linux-gnu/cmake/grpc/gRPCConfig.cmake
but it set gRPC_FOUND to FALSE so package "gRPC" is considered to be NOT FOUND.
??????????????
thiiiiis
 find_package(PkgConfig REQUIRED)
pkg_search_module(gRPC REQUIRED grpc)
npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ cmake --build build/persistentstoregrpcl2test --target install
/home/npoltorapavlo/dev/protoc-gen-validate: warning: directory does not exist.
validate/validate.proto: File not found.
npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ git clone git@github.com:bufbuild/protoc-gen-validate.git
npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ cd protoc-gen-validate/
npoltorapavlo@npoltorapavlo-ubuntu:~/dev/protoc-gen-validate$ make build
 npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ cmake --build build/persistentstoregrpcl2test --target install
In file included from /home/npoltorapavlo/dev/build/persistentstoregrpcl2test/secure_storage.pb.cc:4:
/home/npoltorapavlo/dev/build/persistentstoregrpcl2test/secure_storage.pb.h:35:10: fatal error: google/api/field_behavior.pb.h: No such file or directory
  35 | #include "google/api/field_behavior.pb.h"
fiiiix
 target_include_directories(${PROJECT_NAME} PRIVATE /home/npoltorapavlo/dev/googleapis/gens)
/home/npoltorapavlo/dev/build/persistentstoregrpcl2test/secure_storage.pb.h:38:10: fatal error: validate/validate.pb.h: No such file or directory
   38 | #include "validate/validate.pb.h"
????????????????????????????
run manuallyyyy ~/dev/protoc-gen-validate$ protoc --cpp_out=./ validate/validate.proto
 AAANDD
 target_include_directories(${PROJECT_NAME} PRIVATE /home/npoltorapavlo/dev/protoc-gen-validate)
grpc.pb.cc.o: undefined reference to symbol 'gpr_log'
 ?????????????????????????????????????????????????????????????????
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--copy-dt-needed-entries")
 many like this
 /usr/bin/ld: secure_storage.pb.cc:(.text+0x2f21): undefined reference to `google::protobuf............
 ???????????????????????????????????????????????????????????????????????????????????????????????????//
target_link_libraries(${PROJECT_NAME}.. ${Protobuf_LIBRARIES}
 /usr/bin/ld: CMakeFiles/grpcl2test.dir/secure_storage.pb.cc.o:(.data.rel.ro+0x0): undefined reference to `descriptor_table_google_2fapi_2ffield_5fbehavior_2eproto'
add_executable(${PROJECT_NAME}.../home/npoltorapavlo/dev/protoc-gen-validate/validate/validate.pb.cc
 /usr/bin/ld: CMakeFiles/grpcl2test.dir/secure_storage.pb.cc.o:(.data.rel.ro+0x0): undefined reference to `descriptor_table_google_2fapi_2ffield_5fbehavior_2eproto'
add_executable(${PROJECT_NAME}
...
        /home/npoltorapavlo/dev/googleapis/gens/google/api/field_behavior.pb.cc
        /home/npoltorapavlo/dev/googleapis/gens/google/api/field_behavior.grpc.pb.cc

 OK
https://grpc.io/docs/languages/cpp/basics/
.pb.h, the header which declares your generated message classes
.pb.cc, which contains the implementation of your message classes
.grpc.pb.h, the header which declares your generated service classes
.grpc.pb.cc, which contains the implementation of your service classes


 RUN
auto creds = grpc::CompositeChannelCredentials(
     grpc::InsecureChannelCredentials(),
     grpc::AccessTokenCredentials(SAT));

 GetValue rpc failed. 3 Invalid credentials.
 doc
 Warning
Only use these credentials when connecting to a Google endpoint.  >>>>>>>>>>>??????????????????????????????
 no examples
rpc failed. 14 failed to connect to all addresses; last error: UNKNOWN: ipv6:%5B2a0c:93c0:8000:3903:43:1781:9b88:4ec2%5D:443: Network is unreachable
why is this SHIT using ipv6????????????????????
sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.default.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.lo.disable_ipv6=1
14 failed to connect to all addresses; last error: INTERNAL: ipv4:184.120.72.214:443: Trying to connect an http1.x server
with GRPC_TRACE=all GRPC_VERBOSITY=debug
 grpc HTTP/1.1 400 Bad Request Failed parsing HTTP/2 Expected SETTINGS frame as the first frame, got frame type 80

auto creds = grpc::CompositeChannelCredentials(
    grpc::SslCredentials(grpc::SslCredentialsOptions()),
    grpc::AccessTokenCredentials(SAT));
Client client(grpc::CreateChannel(
    "ss.eu.prod.developer.comcast.com:443",
    creds));
OK!!!!!!!!!!!!!
 */

/*
 * ubuntu23
 * pip does not wortk
 * npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ sudo apt install python3-venv
 * npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ python3 -m venv env
 * npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ source env/bin/activate
 * npoltorapavlo@npoltorapavlo-ubuntu:~/dev$ sh +x rdkservices/.github/workflows/BuildThunder.sh
 */
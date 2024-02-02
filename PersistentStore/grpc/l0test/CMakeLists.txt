# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(grpcl0test)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
FetchContent_MakeAvailable(googletest)

find_package(WPEFramework)
find_package(${NAMESPACE}Plugins REQUIRED)

add_executable(${PROJECT_NAME}
        ../../Module.cpp
        ../Stub.cpp
        StubTest.cpp
        Store2Test.cpp
)

find_package(PkgConfig REQUIRED)
pkg_search_module(gRPC REQUIRED grpc)
find_package(Protobuf REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

FetchContent_Declare(
        validate
        GIT_REPOSITORY https://github.com/bufbuild/protoc-gen-validate.git
        GIT_TAG v1.0.4
)
FetchContent_MakeAvailable(validate)
protobuf_generate(
        PROTOS ${validate_SOURCE_DIR}/validate/validate.proto
        PROTOC_OUT_DIR ${validate_SOURCE_DIR}
        TARGET ${PROJECT_NAME}
        IMPORT_DIRS ${validate_SOURCE_DIR}
)
FetchContent_Declare(
        googleapis
        GIT_REPOSITORY https://github.com/googleapis/googleapis.git
        GIT_TAG master
)
FetchContent_MakeAvailable(googleapis)
protobuf_generate(
        PROTOS ${googleapis_SOURCE_DIR}/google/api/field_behavior.proto
        PROTOC_OUT_DIR ${googleapis_SOURCE_DIR}
        TARGET ${PROJECT_NAME}
        IMPORT_DIRS ${googleapis_SOURCE_DIR}
)
protobuf_generate(
        PROTOS ../secure_storage.proto
        TARGET ${PROJECT_NAME}
        IMPORT_DIRS ${validate_SOURCE_DIR} ${googleapis_SOURCE_DIR}
        APPEND_PATH
)
protobuf_generate(
        PROTOS ../secure_storage.proto
        TARGET ${PROJECT_NAME}
        LANGUAGE grpc
        PLUGIN protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        IMPORT_DIRS ${validate_SOURCE_DIR} ${googleapis_SOURCE_DIR}
        APPEND_PATH
        GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
)

target_include_directories(${PROJECT_NAME} PRIVATE ${googleapis_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${validate_SOURCE_DIR})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--copy-dt-needed-entries")

target_link_libraries(${PROJECT_NAME}
        gmock_main
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        grpc++
        ${Protobuf_LIBRARIES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

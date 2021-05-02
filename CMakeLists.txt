cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS svg.proto graph.proto transport_router.proto transport_catalogue.proto)

set(router_base main.cpp domain.h domain.cpp geo.h geo.cpp graph.h graph.proto
    json.h json.cpp json_builder.h json_builder.cpp json_reader.h json_reader.cpp
    map_render.h map_render.cpp ranges.h router.h svg.h svg.cpp svg.proto
    transport_catalogue.h transport_catalogue.cpp transport_catalogue.proto
    transport_router.h transport_router.cpp transport_router.proto
    serialization.h serialization.cpp)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${router_base})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIR})
#target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)

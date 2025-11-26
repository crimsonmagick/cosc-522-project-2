cmake_minimum_required(VERSION 3.15)
project(cosc522_lodi C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

file(GLOB_RECURSE COMMON_SRC CONFIGURE_DEPENDS
    src/shared/*.c
)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(echo_client
    src/echo-example-udp/udp_echo_client.c
    ${COMMON_SRC}
    include/shared.h
)

add_executable(echo_server
    src/echo-example-udp/udp_echo_server.c
    ${COMMON_SRC}
)

add_executable(echo_client_tcp
    src/echo-example-tcp/tcp_echo_client.c
    src/echo-example-tcp/die_with_error.c
    ${COMMON_SRC}
    include/shared.h
)

add_executable(echo_server_tcp
    src/echo-example-tcp/handle_tcp_client.c
    src/echo-example-tcp/tcp_echo_server.c
    src/echo-example-tcp/die_with_error.c
    ${COMMON_SRC}
)

add_executable(lodi_client
    src/lodi-client/lodi_client.c
    ${COMMON_SRC}
    src/lodi-client/print_feed.c
    src/lodi-client/print_feed.h
)
target_link_libraries(lodi_client PRIVATE m)

add_executable(lodi_server
    src/lodi-server/lodi_server.c
    src/lodi-server/message_repository.h
    src/lodi-server/message_repository.c
    ${COMMON_SRC}
    src/lodi-server/follower_repository.h
    src/lodi-server/follower_repository.c
)
add_executable(pke_server
    src/pke-server/pke_server.c
    ${COMMON_SRC}
    src/pke-server/key_repository.c
    src/pke-server/key_repository.h
)
add_executable(tfa_client
    src/tfa-client/tfa_client.c
    ${COMMON_SRC}
)

add_executable(tfa_server
    src/tfa-server/tfa_server.c
    ${COMMON_SRC}
    src/tfa-server/registration_repository.c
    src/tfa-server/registration_repository.h
)

add_executable(rsa_generate
    src/rsa-utils/rsa_generate.c
    ${COMMON_SRC}
)
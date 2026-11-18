cmake_minimum_required(VERSION 3.20)
project(rip_host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(APP ${CMAKE_CURRENT_SOURCE_DIR}/../firmware/App)

#Warning
set(RIP_WARNINGS -Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion
    -Wfloat-conversion -Wcast-align -Wmissing-prototypes -Wstrict-prototypes -Werror)

#Bat loi bo nho va hanh vi khong xac dinh khi chay test
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)

#firmware core
add_library(rip_core STATIC
    ${APP}/drivers/encoder.c
    ${APP}/services/crc16.c
    ${APP}/services/ringbuf.c
)
target_include_directories(rip_core PUBLIC ${APP}/drivers ${APP}/control ${APP}/services)
target_compile_options(rip_core PRIVATE ${RIP_WARNINGS})
target_link_libraries(rip_core PUBLIC m)

#unit tests
enable_testing()
foreach(t test_encoder test_crc16 test_ringbuf)
    add_executable(${t} tests/${t}.c)
    target_include_directories(${t} PRIVATE tests)
    target_link_libraries(${t} PRIVATE rip_core)
    add_test(NAME ${t} COMMAND ${t})
endforeach()

cmake_minimum_required(VERSION 3.15)
project(test)

add_executable(test src/main.cpp)

find_package(libpqxx)
target_link_libraries(test libpqxx::pqxx)
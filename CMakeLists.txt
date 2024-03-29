cmake_minimum_required(VERSION 3.0)
project(lexiserver C)
set(CMAKE_C_STANDARD 99)

# Add OpenSSL package
find_package(OpenSSL REQUIRED)

# Add executable target
add_executable(lexiserver src/lexiserver.c)

# Link OpenSSL libraries
target_link_libraries(lexiserver OpenSSL::SSL OpenSSL::Crypto)

cmake_minimum_required(VERSION 3.10)
project(WMBusDecryptor)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

# Add executable
add_executable(wmbus_decryptor src/wmbus_decryptor.cpp alt/simple_aes.cpp)

# Link OpenSSL libraries
target_link_libraries(wmbus_decryptor OpenSSL::SSL OpenSSL::Crypto)

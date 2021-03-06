cmake_minimum_required(VERSION 3.1)

project(IceoryxTest)

if(CMAKE_COMPILER_IS_GNUCXX)
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.3.0)
        message(FATAL_ERROR "Compiler version is lower than required. Required >=8.3.0")
    endif()
endif()


include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag(-std=c++17 SUPPORTS_CXX17)
    if(SUPPORTS_CXX17)
        add_compile_options(-std=c++17)
    else()
        message(FATAL_ERROR "Compiler doesn't support C++17")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PubSub pubsub.cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")


target_link_libraries(PubSub
       	iceoryx_posh
        iceoryx_utils
       	iceoryx_posh_roudi 
	iceoryx_platform 
	iceoryx_introspection
	pthread
	rt
)


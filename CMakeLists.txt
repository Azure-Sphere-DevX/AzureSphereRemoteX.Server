#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)

project(AzureSphereRemoteX_Server C)

if (EXISTS "${CMAKE_SOURCE_DIR}/tools/cmake/azsphere_config.cmake")
    include(tools/cmake/azsphere_config.cmake)
    auto_generate_azsphere_config()
else()

    # For information on setting tools revision and target api set see 
    # https://docs.microsoft.com/en-us/azure-sphere/app-development/using-cmake-functions

    azsphere_configure_tools(TOOLS_REVISION "21.10")
    azsphere_configure_api(TARGET_API_SET "11")

endif()

add_executable(${PROJECT_NAME} main.c eventloop_timer_utilities.c echo_tcp_server.c peripherals.c)
target_link_libraries(${PROJECT_NAME} applibs gcc_s c)

add_subdirectory("AzureSphereDevX" out)
target_link_libraries (${PROJECT_NAME} azure_sphere_devx)
target_include_directories(${PROJECT_NAME} PUBLIC Azure-Sphere-DevX/include )


azsphere_target_add_image_package(${PROJECT_NAME})

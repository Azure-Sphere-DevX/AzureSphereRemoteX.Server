#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required(VERSION 3.10)
project(AzureSphereRemoteX_Server C)

include(azsphere_board.txt)

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

set(BOARD_COUNTER 0)

if(AVNET)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    add_definitions( -DDEVICE_PLATFORM="Avnet Azure Sphere MT3620 Starter Kit Rev 1")
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk" TARGET_DEFINITION "azure_sphere_remotex.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 1")
endif(AVNET)

if(AVNET_REV_2)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_AVNET=TRUE )
    add_definitions( -DDEVICE_PLATFORM="Avnet Azure Sphere MT3620 Starter Kit Rev 2")
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/avnet_mt3620_sk_rev2" TARGET_DEFINITION "azure_sphere_remotex.json")
    message(STATUS "Azure Sphere board selected: AVNET REV 2")
endif(AVNET_REV_2)

if (SEEED_STUDIO_RDB)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_SEEED_STUDIO=TRUE )
    add_definitions( -DDEVICE_PLATFORM="Seeed Studio Azure Sphere MT3620 Development Kit aka RDB")
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/mt3620_rdb" TARGET_DEFINITION "azure_sphere_remotex.json")
    message(STATUS "Azure Sphere board selected: SEEED STUDIO RDB")
endif()

if (SEEED_STUDIO_MINI)
    MATH(EXPR BOARD_COUNTER "${BOARD_COUNTER}+1")
    add_definitions( -DOEM_SEEED_STUDIO_MINI=TRUE )    
    add_definitions( -DDEVICE_PLATFORM="Seeed Studio Azure Sphere MT3620 Mini Dev Board")    
    azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/seeed_mt3620_mdb" TARGET_DEFINITION "azure_sphere_remotex.json")
    message(STATUS "Azure Sphere board selected: SEEED STUDIO MINI")
endif(SEEED_STUDIO_MINI)

if(BOARD_COUNTER EQUAL 0)
    message(FATAL_ERROR "No Azure Sphere boards selected. Ensure one board set")
endif()

if(BOARD_COUNTER GREATER 1)
    message(FATAL_ERROR "Multiple (${BOARD_COUNTER}) Azure Sphere boards selected. Ensure only one board set")
endif()


azsphere_target_add_image_package(${PROJECT_NAME})

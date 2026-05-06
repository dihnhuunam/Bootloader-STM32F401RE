cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Application_3)

if(NOT DEFINED APP_SLOT)
    set(APP_SLOT "A")
endif()

if(APP_SLOT STREQUAL "A")
    set(APP_IMAGE_SLOT_VALUE 0)
    set(APP_IMAGE_SLOT_NAME "SlotA")
elseif(APP_SLOT STREQUAL "B")
    set(APP_IMAGE_SLOT_VALUE 1)
    set(APP_IMAGE_SLOT_NAME "SlotB")
else()
    message(FATAL_ERROR "APP_SLOT must be A or B")
endif()

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})
set(APP_IMAGE_NAME "${CMAKE_PROJECT_NAME}_${APP_IMAGE_SLOT_NAME}")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME "${APP_IMAGE_NAME}")

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Debug/Debug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Led/Led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Image/Image.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Flash/Flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/OTA/UartOTA.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/App/App.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Debug
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Led
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Image
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Flash
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/OTA
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/App
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    APP_IMAGE_SLOT=${APP_IMAGE_SLOT_VALUE}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/Scripts/post-build.bat"
            "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${APP_IMAGE_NAME}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating CRC-patched application image"
    VERBATIM
)

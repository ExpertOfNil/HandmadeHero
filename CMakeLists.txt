cmake_minimum_required(VERSION 3.16)
project(HandmadeHero LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/
)

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-shared)

include_directories(
	include/
	/usr/include/
)

file(GLOB CLIENT_SOURCES ${CMAKE_SOURCE_DIR} "src/*.c")

add_executable(${PROJECT_NAME}
    ${CLIENT_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBS}
    SDL3::SDL3
    m
)

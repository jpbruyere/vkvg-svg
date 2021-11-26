cmake_minimum_required(VERSION 2.4)

project(svgParsing)

FILE(GLOB sources src/*.c)

add_executable(app ${sources})

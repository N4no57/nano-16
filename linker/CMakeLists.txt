cmake_minimum_required(VERSION 3.30)
project(assembler C)

set(CMAKE_C_STANDARD 11)

add_executable(nl16-as src/main.c
        src/objectFileReader.c include/objectFileReader.h
        src/linklib.c
        include/linklib.h
)
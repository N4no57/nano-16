cmake_minimum_required(VERSION 3.30)
project(assembler C)

set(CMAKE_C_STANDARD 11)

add_executable(na16-as src/main.c
        src/tokeniser.c include/tokeniser.h
        src/parser.c include/parser.h
        src/asmlib.c include/asmlib.h
        src/byte_emitter.c include/byte_emitter.h
        src/object_file_writer.c
        include/object_file_writer.h
)
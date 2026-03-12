#include "../include/objectFileReader.h"

int main(void) {
    obj_file obj = {0};

    read_obj(&obj, "test.o");

    return 0;
}
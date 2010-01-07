#include <stdio.h>

#include "../ngtembed.h"
#include "test_utils.h"

DEFINE_TEST_FUNCTION    {
    return ngt_embed(ngtembed_c_template, out, argc, argv);
}

int main(int argc, char** argv) {
    if (argc < 3)   {
        fprintf(stderr, "USAGE: ngtembed_test infile1 [... infilen] bmkfile\n");
        return -1;
    }
    
    return test_runner(0, argv[argc-1], test_function, argc-1, argv);
}
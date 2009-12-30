#include <stdio.h>

#include "ngtembed.h"

int main(int argc, char** argv) {
    return ngt_embed(ngtembed_c_template, stdout, argc, argv);
}
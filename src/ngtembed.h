#ifndef NGTEMBED_H
#define NGTEMBED_H

/**
 * ngt_embed - Turn one or more template files into C code for embedding into programs
 */
int ngt_embed(FILE* out, int argc, char** argv);

#endif
#ifndef NGTEMBED_H
#define NGTEMBED_H

const char ngtembed_c_template[] = 
    "/* Embedded Template Strings */\n"
    "@#Template@"
        "const char @TemplateName@[] =@BI_NEWLINE@    \"@TemplateBody:cstring_escape:breakup_lines@\";"
        "@#Template_separator@@BI_NEWLINE@@BI_NEWLINE@@/Template_separator@"
    "@/Template@"
    "@BI_NEWLINE@"
    "/* End Embedded Template Strings */"
    "@BI_NEWLINE@";

/**
 * ngt_embed - Turn one or more template files into C code for embedding into programs
 */
int ngt_embed(const char* code_template, FILE* out, int argc, char** argv);

#endif
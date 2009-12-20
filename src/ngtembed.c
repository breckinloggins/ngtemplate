/**
 * ngtembed - A tool for converting template files to embedded cstrings
 */

#include <stdlib.h>
#include <limits.h>
#include "include/ngtemplate.h"

static const char out_template[] = 
	"/* Embedded Template Strings */\n"
	"@#Template@"
		"const char @TemplateName@[] =\n    \"@TemplateBody:cstring_escape:breakup_lines@\";"
		"@#Template_separator@@BI_NEWLINE@@BI_NEWLINE@@/Template_separator@"
	"@/Template@"
	"@BI_NEWLINE@"
	"/* End Embedded Template Strings */"
	"@BI_NEWLINE@";

void breakup_lines_modifier_cb(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)	{
	char* p;
	p = (char*)value;
	int i;
	i = 0;
	while(*p)	{
		sb_append_ch(out_sb, *p++);
		
		// Attempt to keep everything under 80 cols by splitting up lines
		if (i++ > 70 && *p && *(p-1) != '\\')	{
			i = 0;
			sb_append_str(out_sb, "\"\n    \"");
		}
	}
	sb_append_ch(out_sb, '\0');
}

int main(int argc, char** argv)	{
	int i;
	ngt_template* tpl;
	ngt_dictionary* dict;
	char* output;
	char file[PATH_MAX];
	char name[PATH_MAX];
	
	if (argc == 1)	{
		fprintf(stderr, "USAGE: ngtembed file1[=name1] file2[=name2] ... fileN[=nameN] [>out_file]\n");
		exit(-1);
	}
	
	ngt_init();
	tpl = ngt_new();
	dict = ngt_dictionary_new();
	
	ngt_add_modifier(tpl, "breakup_lines", breakup_lines_modifier_cb);
	ngt_set_delimiters(tpl, "@", "@");
	ngt_set_dictionary(tpl, dict);
	tpl->template = (char*)out_template;
	
	for (i = 1; i < argc; i++)	{
		char* p;
		int m, has_name;

		p = argv[i];
		m = 0;
		has_name = 0;
		while (*p)	{
			if (m < PATH_MAX && *p == '=')	{
				// Setting an explicit name.  Throw out what we've done so far
				has_name = 1;
				m = 0;
				p++;
				continue;
			}
			
			if (*p == '.' || *p == '\\' || *p == '/')	{
				name[m] = '_';
			} else {
				name[m] = *p;
			}
			
			if (!has_name)	{
				file[m] = *p;
				file[m+1] = '\0';
			}
			
			p++;
			m++;
		}
		name[m] = '\0';
		
		if (m)	{
			stringbuilder* sb;
			FILE* fd;
			int ch;
			
			fd = fopen(file, "r");
			if (!fd)	{
				fprintf(stderr, "Could not open '%s' for reading\n", file);
				exit(-1);
			}
			
			ngt_dictionary* section = ngt_dictionary_new();
						
			ngt_set_string(section, "TemplateName", name);
						
			sb = sb_new();
			while ((ch = fgetc(fd)) != EOF)	{
				sb_append_ch(sb, (char)ch);
			}
			
			ngt_set_string(section, "TemplateBody", sb_cstring(sb));
			sb_destroy(sb, 1);
						
			ngt_add_dictionary(dict, "Template", section, NGT_SECTION_VISIBLE);
			fclose(fd);
		}
	}
	
	ngt_expand(tpl, &output);
	fprintf(stdout, "%s\n", output);
	
	ngt_destroy(tpl);
	ngt_dictionary_destroy(dict);
	free(output);
}
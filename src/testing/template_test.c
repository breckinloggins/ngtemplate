#include "../../lib/libuseful/src/test_utils.h"
#include "../include/template.h"

#define MAXVALUELENGTH 32767

/**
 * Reads in key/value pairs of the form Key = Value <newline>
 * Note that there is little error handling here, so make sure everything is formatted correctly
 *
 * For nested sections, note that if you have another section at the same level, it must be signified by
 * "}{", do NOT put a newline between the closing brace of the last section and the opening brace of the
 * next one.  This is due to the fragility of the parser, but since it's a test function, it's really not
 * worth fixing it.
 */
char* read_in_dictionary(template_dictionary* d, int *line, int in_dictionary)	{
	// TODO:  This is one big hack of a function.  We should clean this up some time
	
	char marker[MAXMARKERLENGTH];
	char value[MAXVALUELENGTH];
	char *ptr, *in_ptr;
	int ch, after_equals, escaped, in_comment, found_dictionary;
	
	memset(marker, 0, MAXMARKERLENGTH);
	memset(value, 0, MAXVALUELENGTH);
	
	escaped = 0;
	after_equals = 0;
	in_comment = 0;
	ptr = marker;
	in_ptr = d->template;
	if (!in_ptr)	{
		return 0;
	}
	
	found_dictionary = in_dictionary;
	while (!found_dictionary && *in_ptr)	{
		if (*in_ptr 	== '{' &&
			*(in_ptr+1)	== '{' &&
			*(in_ptr+2) == '!' &&
			*(in_ptr+3) == '#')
		{
			found_dictionary = 1;
			in_ptr+=4;	// Skip over those starting characters
			break;			
		}
		
		in_ptr++;
	}
	
	if (!found_dictionary)	{
		return 0;
	}
	
	while (*in_ptr)	{
		if (*in_ptr		== '#' &&
			*(in_ptr+1)	== '!' &&
			*(in_ptr+2) == '}' &&
			*(in_ptr+3) == '}')	{
				
			// Found the end
			return 0;
		}
		
		ch = (int)*in_ptr++;
		
		
		if (!in_comment && ch == '#')	{
			// Start comment until newline
			in_comment = 1;
			continue;
		} else if (in_comment) {
			if (ch == '\r' || ch == '\n')	{
				// Done with comment
				in_comment = 0;
			} else {
				continue;
			}
		} 
		
		if (ch == '\\')	{
			// We have an escape character coming
			ch = (int)*in_ptr++;
			if (ch == EOF)	{
				fprintf(stderr, "Unexpected End of File after escape character on line %d\n", *line);
				exit(-1);
			}
			
			if (ch != '{' && ch != '}' && ch != '!' && ch != '=' && ch != '#')	{
				fprintf(stderr, "Unrecognized escape character ('\\%c') on line %d\n", ch, *line);
				exit(-1);
			}
			
			escaped = 1;
		} else {
			escaped = 0;
		}
			
		if (!escaped && ch == '{')	{
			if (!after_equals)	{
				fprintf(stderr, "Unexpected '{' before '=' on line %d\n", *line);
				exit(-1);
			}
			
			template_dictionary* child = template_new();
			child->parent = d;
			child->template = in_ptr;
			template_add_dictionary(d, marker, child);
			in_ptr = read_in_dictionary(child, line, 1);
			child->template = 0;
			continue;
		}
		
		if (!escaped && ch == '}')	{
			if (ptr != marker && !after_equals)	{
				fprintf(stderr, "Unexpected '}' before '=' on line %d\n", *line);
				exit(-1);
			}
			
			if (!(d->parent))	{
				fprintf(stderr, "Unexpected '}' before '{' on line %d\n", *line);
				exit(-1);
			}
			
			// If we got here, it means that we were a recursive read_in_dictionary call and it's
			// time for us to exit
			return in_ptr;
		}
		
		if (ch == '\r' || ch == '\n')	{
			if (ptr != marker && ptr != value)	{
				// We have a valid pair (we hope)
				template_set_string(d, marker, value);
			}
			
			memset(marker, 0, MAXMARKERLENGTH);
			memset(value, 0, MAXVALUELENGTH);
			
			ptr = marker;
			after_equals = 0;
			(*line)++;
			continue;
		}
		
		if (!after_equals && (ch == ' ' || ch == '\t'))	{
			// Skip whitespace
			continue;
		}
		
		if (!escaped && ch == '=')	{
			if (ptr == marker)	{
				fprintf(stderr, "Null marker on line %d\n", *line);
				exit(-1);
			}
			
			ptr = value;
			after_equals = 1;
			continue;
		}
		
		*(ptr++) = ch;
		
	}
	
	return in_ptr;
	
}

char* get_template_cb(const char* name)	{
	char* template;
	if (strcmp(name, "Callback_Template"))	{
		return 0;
	}

	template = (char*)malloc(1024);
	memset(template, 0, 1024);
	sprintf(template, "%s", "This is a template section.  "
							"Key = {{Key}} and One = {{One}}"
							"\n\n\tHere's another line of the section\n");
	
	return template;
}

void cleanup_template_cb(const char* name, char* template)	{
	if (!strcmp(name, "Callback_Template"))	{
		free(template);
	}
}

void modifier_cb(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)	{
	sb_append_str(out_sb, marker);
	sb_append_str(out_sb, " MODIFIED BY ");
	sb_append_str(out_sb, name);
	sb_append_str(out_sb, ", was ");
	sb_append_str(out_sb, value);
}

DEFINE_TEST_FUNCTION	{
	char* result;
	int line;
	
	if (argc < 2)	{
		fprintf(stderr, "Invoking this test with zero arguments is not supported\n");
		return -1;
	}
	
	template_dictionary* d = template_new();
	line = 1;
	
	template_load_from_file(d, in);
	read_in_dictionary(d, &line, 0);
		
	template_add_modifier(d, "modifier", modifier_cb);
	template_set_include_cb(d, "Callback_Template", get_template_cb, cleanup_template_cb);
	
	// To test template_set_stringf(), template_set_int()
	template_set_stringf(d, "FmtString", "(%d, %f, 0x%x, %s, %s some more %d)", 
		42, 3.14159, 0xdeadbeef, "A String", "Another String", -72 );
	template_set_int(d, "IntValue", 12345);	// I have the same combination on my luggage
	
	if (argc > 3)	{
		template_set_include_filename(d, "Filename_Template", argv[3]);
	}
	
	if (template_process(d, &result) < 0)	{
		return -1;
	}
	
	fprintf(out, "%s\n", result);
	
	free(result);
	template_destroy(d);
	return 0;
}

int main(int argc, char** argv)	{
	RUN_TEST;
}
/**
 * Standard Environment routines for ngtemplate.  Includes Global Dictionary defaults and standard
 * modifiers
 */ 

#include "internal.h"

/**
 * Standard :none modifier.  Just echoes value to output
 */
void _mod_none(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)	{
	sb_append_str(out_sb, value);
}

/**
 * :cstring_escape - Turns newlines into \n, tabs into \t, quotes into \", and so forth
 */
void _mod_cstring_escape(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)	{
	char* ptr;
	
	ptr = (char*)value;
	while (ptr && *ptr)	{
		switch(*ptr)	{
			case '\a': sb_append_str(out_sb, "\\a");	break;
			case '\b': sb_append_str(out_sb, "\\b");	break;
			case '\f': sb_append_str(out_sb, "\\f");	break;
			case '\n': sb_append_str(out_sb, "\\n"); 	break;
			case '\r': sb_append_str(out_sb, "\\r"); 	break;
			case '\t': sb_append_str(out_sb, "\\t"); 	break;
			case '\v': sb_append_str(out_sb, "\\v");	break;
			case '\'':	sb_append_str(out_sb, "\\\'");	break;
			case '\"': sb_append_str(out_sb, "\\\"");	break; 
			case '\\': sb_append_str(out_sb, "\\\\");	break;
			case '\?': sb_append_str(out_sb, "\\\?");	break;
			default: sb_append_ch(out_sb, *ptr);
		}
		
		ptr++;
	}
}

/**
 * Sets up the ngtemplate global dictionary with standard values
 */
void _init_global_dictionary(ngt_dictionary* d)	{
	ngt_set_string(d, "BI_SPACE", " ");
	ngt_set_string(d, "BI_NEWLINE", "\n");
}

/**
 * Sets up the standard modifier callbacks in a template.  They can be overriden by user code later
 */ 
void _init_standard_callbacks(ngt_template* tpl)	{
	ngt_add_modifier(tpl, "none", _mod_none);
	ngt_add_modifier(tpl, "cstring_escape", _mod_cstring_escape);
}
/**
 * An implementation of Google's "CTemplate" in C
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../lib/libuseful/src/include/list.h"
#include "../lib/libuseful/src/include/stringbuilder.h"
#include "include/ngtemplate.h"
#include "internal.h"

/**
 * This is a special dictionary that contains global modifiers and dictionary
 * values
 */
static ngt_dictionary* s_global_dictionary = 0;

static int s_initialized = 0;

/**
 * Initializes the ngtemplate library.  This must be called before dictionaries
 * can be created or templates processed
 */
void ngt_init()	{
	if (s_initialized)	{
		return;
	}
	
	s_initialized = 1;
	s_global_dictionary = ngt_dictionary_new();
	_init_global_dictionary(s_global_dictionary);
}

/**
 * Creates a new ngt_template, ready to be filled with values and sections
 *
 * Returns the ngt_template created, or NULL if this could not be done.  It is up
 * to the caller to manage this dictionary
 */
ngt_template* ngt_new()	{
	ngt_template* tpl;
	
	if (!s_initialized)	{
		fprintf(stderr, "FATAL: You must call ngt_init() before creating any template dictionaries\n");
		return 0;
	}
	
	tpl = (ngt_template*)malloc(sizeof(ngt_template));
	memset(tpl, 0, sizeof(ngt_template));
	
	ht_init(&tpl->modifiers, 23, _modifier_hash, _modifier_match, _modifier_destroy);
	
	_init_standard_callbacks(tpl);
	
	return tpl;
}

/**
 * Creates a new ngt Template Dictionary, ready to be filled with values
 */
ngt_dictionary* ngt_dictionary_new()	{
	ngt_dictionary* d = (ngt_dictionary*)malloc(sizeof(ngt_dictionary));
	memset(d, 0, sizeof(ngt_dictionary));
	
	// NOTE: Adjust the buckets parameter depending on how many markers are likely to be in a template
	//		file (then adjust upward to the next prime number
	ht_init((hashtable*)d, 197, _dictionary_hash, _dictionary_match, _dictionary_destroy);
	
	return d;	
}

/** 
 * Destroys the given template
 */
void ngt_destroy(ngt_template* tpl)	{
	_destroy_template((void*)tpl);
}

/**
 * Destroys the given template dictionary and any sub-dictionaries
 */
void ngt_dictionary_destroy(ngt_dictionary* dict)	{
	ht_destroy((hashtable*)dict);
	free(dict);
}

/**
 * Sets the dictionary for the given template.  The dictionary can be NULL
 */
void ngt_set_dictionary(ngt_template* tpl, ngt_dictionary* dict)	{
	tpl->dictionary = dict;
}

/**
 * Loads the template string from the given file pointer.  Does NOT close the pointer
 *
 * Returns 0 if successful, -1 otherwise
 */
int ngt_load_from_file(ngt_template* tpl, FILE* fp)	{
	char* template;
	
	template = _get_template_from_file(fp);
	if (!template)	{
		return -1;
	}
	
	if (tpl->template)	{
		free(tpl->template);
	}
	
	tpl->template = template;
	return 0;
}

/**
 * Loads the template string from the given file name
 *
 * Returns 0 if successful, -1 otherwise
 */
int ngt_load_from_filename(ngt_template* tpl, const char* filename)	{
	char* template;
	
	template = _get_template_from_filename(filename);
	if (!template)	{
		return -1;
	}
	
	if (tpl->template)	{
		free(tpl->template);
	}
	
	tpl->template = template;
	return 0;
}

/**
 * Sets a modifier function that will be called when a modifier in the template does not
 * resolve to any known modifiers.  The function will have the opportunity to adjust the output
 * of the marker, and will be passed any arguments.
 */
void ngt_set_modifier_missing_cb(ngt_template* tpl, modifier_fn mod_fn)	{
	tpl->modifier_missing = mod_fn;
}

/**
 * Sets a callback function that will be called when no value for a variable marker can be
 * found.  The function will have the opportunity to give the value of the variable by appending
 * to the out_sb string builder.
 */
void ngt_set_variable_missing_cb(ngt_template* tpl, get_variable_fn get_fn)	{
	tpl->variable_missing = get_fn;
}

/**
 * Sets a modifier function that can be called when the given modifier name is encountered
 * in the template.  The modifier will have the opportunity to adjust the output of the 
 * marker, and will be passed in any arguments.  If another modifier is already present with
 * the same name, that modifier will be replaced.
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_add_modifier(ngt_template* tpl, const char* name, modifier_fn mod_fn)	{
	_modifier* mod, *prev_mod;
	
	mod = (_modifier*)malloc(sizeof(_modifier));
	mod->name = (char*)malloc(strlen(name) + 1);
	strcpy(mod->name, name);
	mod->modifier = mod_fn;
	
	if (ht_insert(&tpl->modifiers, mod) == 1)	{
		// Already in the table, replace
		prev_mod = mod;
		ht_remove(&tpl->modifiers, (void *)&prev_mod);
		_dictionary_destroy((void*)prev_mod);
		
		ht_insert(&tpl->modifiers, mod);
	}
	
	return 0;	
}

/**
 * Sets a string value in the template dictionary.  Any instance of "marker" in the template 
 * will be replaced by "value".  If the marker is already in the template, the value will be
 * replaced.
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_set_string(ngt_dictionary* dict, const char* marker, const char* value)	{
	char* str;
	str = (char*)malloc(strlen(value) + 1);
	if (!str)	{
		// Could not allocate enough memory for the string
		return -1;
	}
	
	strcpy(str, value);
	return _set_string(dict, marker, str);
}

/**
 * Sets a string value in the template dictionary using printf-style format specifiers.  Any
 * instance of "marker" in the template will be replaced by "value"
 *
 * Returns 0 if the operations succeeded, -1 otherwise
 */
int ngt_set_stringf(ngt_dictionary* dict, const char* marker, const char* fmt, ...)	{
	// TODO: Won't work on Windows.  Have to use _vscprintf on that platform
	char* str;
	int length;
	va_list arglist;
	
	va_start(arglist, fmt);
	length = vsnprintf(0, 0, fmt, arglist);
	va_end(arglist);
	
	if (length <= 0)	{
		// Could not determine the space needed for the string
		return -1;
	}
	
	str = (char*)malloc(length + 1);
	if (!str)	{
		// Could not allocate enough memory for the string
		return -1;
	}
	
	va_start(arglist, fmt);
	vsnprintf(str, length + 1, fmt, arglist);
	va_end(arglist);
	
	return _set_string(dict, marker, str);
}

/**
 * Sets an integer value in the template dictionary.  Any
 * instance of "marker" in the template will be replaced by "value"
 *
 * Returns 0 if the operations succeeded, -1 otherwise
 */
int ngt_set_int(ngt_dictionary* dict, const char* marker, int value)	{
	return ngt_set_stringf(dict, marker, "%d", value);
}

/**
 * On an include template dictionary, sets the callbacks to be called when the system needs the template
 * string for the given include name.  Also, prove a cleanup_template function to call when the
 * system no longer needs the template data.
 * NOTES: It is illegal to call this function on a string value marker
 *        It is legal to pass null to cleanup_template
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_set_include_cb(ngt_dictionary* dict, const char* marker, get_template_fn get_template, 
							cleanup_template_fn cleanup_template)	{
	_dictionary_item* item, *prev_item;
	
	item = _new_dictionary_item();
	item->marker = (char*)malloc(strlen(marker) + 1);
	
	strcpy(item->marker, marker);
	
	if (ht_insert((hashtable*)dict, item) == 1)	{
		// Already in the hashtable
		prev_item = item;
		ht_lookup((hashtable*)dict, (void*)&prev_item);
		free(item);
		item = prev_item;
		
		if (!(item->type & ITEM_D_LIST))	{
			// Cannot call set_include_cb on a string value
			return -1;
		}
	}
	
	item->type = ITEM_INCLUDE;
	item->val.include_value.get_template = get_template;
	item->val.include_value.cleanup_template = cleanup_template;
	
	return 0;	
}

/**
 * On an include template dictionary, sets the filename that will be loaded to obtain the template data
 * NOTE: It is illegal to call this function on a string value marker
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_set_include_filename(ngt_dictionary* dict, const char* marker, const char* filename)	{
	_dictionary_item* item;
	
	if (ngt_set_include_cb(dict, marker, _get_template_from_filename, _cleanup_template) != 0)	{
		// Something went wrong
		return -1;
	}
	
	item = _query_item(dict, marker);
	item->val.include_value.filename = (char*)malloc(strlen(filename) + 1);
	strcpy(item->val.include_value.filename, filename);
	
	return 0;
}

/**
 * Adds a child dictionary under the given marker.  If there is an existing dictionary under this marker, 
 * the new dictionary will be ADDED to the end of the list, NOT replace the old one
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_add_dictionary(ngt_dictionary* dict, const char* marker, ngt_dictionary* child)	{
	_dictionary_item* item, *prev_item;
	
	item = (_dictionary_item*)malloc(sizeof(_dictionary_item));
	item->type = ITEM_D_LIST;
	item->marker = (char*)malloc(strlen(marker) + 1);
	
	strcpy(item->marker, marker);
	
	if (ht_insert((hashtable*)dict, item) == 1)	{
		
		// Already in the table, add to it
		prev_item = item;
		ht_lookup((hashtable*)dict, (void*)&prev_item);
		free(item);
		item = prev_item;
		
		if (!(item->type & ITEM_D_LIST))	{
			// We already have a marker with this name, but it's of a different type
			return -1;
		}
		
	}
	
	if (!item->val.d_list_value)	{
		item->val.d_list_value = (list*)malloc(sizeof(list));
		list_init(item->val.d_list_value, _dictionary_destroy);
	}
	
	list_insert_next(item->val.d_list_value, list_tail(item->val.d_list_value), child);
	child->parent = dict;
	
	return 0;
}

/**
 * Expands the given template according to the dictionary, putting the result in "result" pointer.
 * Sufficient space will be allocated for the result, and it will then be 
 * up to the caller to manage this space
 *
 * Returns 0 if the template was successfully processed, -1 if there was an error
 */
int ngt_expand(ngt_template* tpl, char** result)	{
	int res;
	_parse_context context;
	char start_marker[2];
	char end_marker[2];
	
	memset(&context, 0, sizeof(_parse_context));	
	context.current_section = "";
	
	strcpy(context.start_delimiter.literal, "{{");
	context.start_delimiter.length = 2;
	strcpy(context.end_delimiter.literal, "}}");
	context.end_delimiter.length = 2;
	
	context.template = tpl;
	context.active_dictionary = tpl->dictionary;
	context.in_ptr = (char*)tpl->template;
	context.template_line = 1;
	
	context.out_sb = sb_new_with_size(1024);
	res = _process(&context) > 0;
	
	*result = sb_cstring(context.out_sb);
	sb_destroy(context.out_sb, 0);
	
	return res;
}

/**
 * Pretty-prints the dictionary key value pairs, one per line, with nested dictionaries tabbed
 */
void ngt_print_dictionary(ngt_dictionary* dict, FILE* out)	{
	
	hashtable_iter* it = ht_iter_begin(&dict->dictionary);
	_dictionary_item* item;
	while (it)	{
		item = (_dictionary_item*)ht_value(it);
		switch(item->type)	{
		case ITEM_STRING:
			fprintf(out, "%s=%s\n", item->marker, item->val.string_value);
			break;
		case ITEM_D_LIST:
		case ITEM_INCLUDE:
			// TODO: Recursively print
			fprintf(out, "%s=(section)\n", item->marker);
			break;
		default:
			// If you see this, you forgot to add a case here
			fprintf(out, "%s=(UNKNOWN TYPE)\n", item->marker);
			break;
		}
		
		it = ht_iter_next(it);
	}
}

/**
 * Returns that Global Dictionary in which the Standard Values for all templates are defined
 */
ngt_dictionary* ngt_get_global_dictionary()	{
	return s_global_dictionary;
}
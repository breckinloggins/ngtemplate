/**
 * An implementation of Google's "CTemplate" in C
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../lib/libuseful/src/include/list.h"
#include "../lib/libuseful/src/include/stringbuilder.h"
#include "include/template.h"

/** 
 * These are the items we will hold in the dictionary hash
 *
 * NOTE: This is a kind of "self-inherited struct".  By that, I mean that any item that is an
 *		ITEM_INCLUDE is ALSO an ITEM_D_LIST, and you can treat it as such by (e.g.) using 
 *      val.d_list_value instead of val.include_value.d_list even if it is an ITEM_INCLUDE.  The reason
 *      we do this instead of (say) making an "_dictionary_include_item" is because C has no RTTI, and 
 *		we're already checking on the type enum anyway.
 *
 *		To make this work, the only conventions you must follow are:
 *         1. Do NOT put a field above the d_list in include_params_tag!  It must be the first so it 
 *            is compatible with the d_list_value pointer
 *         2. When testing if something is an ITEM_D_LIST, use type & ITEM_D_LIST instead of type ==
 *
 */
typedef struct _dictionary_item_tag	{
	char* marker;
	enum { 
		ITEM_STRING = 0, 
		ITEM_D_LIST = 1, 
		ITEM_INCLUDE = 3	/* So a bitwise AND test on ITEM_D_LIST will succeed on ITEM_INCLUDE */ 
	} type;
	
	union	{
		char* string_value;
		list* d_list_value;
		
		struct	_include_params_tag {
			list* d_list;			/* NOTE: MUST be the first item in the struct.  This lets us use
											val.d_list_value instead of val.include_value.d_list     */
			
			get_template_fn 		get_template;
			cleanup_template_fn		cleanup_template;
			
			char* template;
			char* filename;			/* Only used in case of template_set_filename */
			
		} include_value;
	} val;
} _dictionary_item;

// Represents a start or stop marker delimiter
#define MAX_DELIMITER_LENGTH	8				/* I really don't know why someone would want a 
											 		delimiter this long, but just in case */
typedef struct _delimiter_tag	{
	int length;
	char literal[MAX_DELIMITER_LENGTH];			/* Keep the literal as the last record in the 
													struct so people can do the "overallocated 
													struct" trick if they need more than 
													MAX_DELIMITER_LENGTH */
} _delimiter;

// Represents a marker modifier
typedef struct _modifier_tag	{
	char* name;								// The name that will be used to call the modifier
	modifier_fn modifier;					// The function that will be called when the modifier is 
											// invoked
} _modifier;

// Represents the current state of the template parser
typedef struct _parse_context_tag	{
	struct _parse_context_tag* parent;		

	int mode;								// The current mode of the parser
	
	char* current_section;					// The current section we're in (could be null)
	int last_expansion;						// Nonzero if this is the last expansion in a series of
											// 	section expansions.  Used for separator logic
	template_dictionary* dict;				// The current data dictionary in use
	_delimiter start_delimiter;				// Characters that signify start of a marker
	_delimiter end_delimiter;				// Characters that signify end of a marker
	
	int		template_line;					// Line number in the current source template
	char* 	in_ptr;							// Where we are in the template
	stringbuilder* out_sb;					// Stringbuilder that holds our current output
											//	(may be reallocated by any call)
	int		out_pos;						// Pointer representing position in the output string
	
	int		expanding_include;
	char*	line_ws_start;					// When we include a template from another file, we want					
	int		line_ws_range;					//   every line of the expanded template to respect the 
											//	 indention level of the include.  Furthermore, we keep
											//   our contact that everything not inside the marker be copied
											//   verbatim (including whitespace), so these variables represent
											//	 the chunk of the input template representing the whitespace
											//	 right before the template include
} _parse_context;

/**
 * The hash function we will use for the template dictionary
 */
int dictionary_hash(const void* key)	{
	// Hash by the marker value
	return ht_hashpjw(((_dictionary_item*)key)->marker);
}

/**
 * The function we will use to determine if two dictionary_items match
 */
int dictionary_match(const void* key1, const void* key2)	{
	return !strcmp(((_dictionary_item*)key1)->marker, ((_dictionary_item*)key2)->marker);
}

/**
 * The function that will be called when a dictionary_item must be destroyed
 */
void dictionary_destroy(void *data)	{
	_dictionary_item* d = (_dictionary_item*)data;
	
	if (d->type == ITEM_STRING)	{
		free(d->val.string_value);
	} else if (d->type & ITEM_D_LIST && d->val.d_list_value != 0) {
		list_destroy(d->val.d_list_value);
	} 
	
	if (d->type == ITEM_INCLUDE)	{
		if (d->val.include_value.cleanup_template)	{
			d->val.include_value.cleanup_template(d->marker, d->val.include_value.template);
		}
		
		if (d->val.include_value.filename)	{
			free(d->val.include_value.filename);
		}
	}
	
	free(d->marker);
	free(d);
}

/**
 * The hash function we will use for the modifier dictionary
 */
int modifier_hash(const void* key)	{
	// Hash by the marker value
	return ht_hashpjw(((_modifier*)key)->name);
}

/**
 * The function we will use to determine if two modifiers match
 */
int modifier_match(const void* key1, const void* key2)	{
	return !strcmp(((_modifier*)key1)->name, ((_modifier*)key2)->name);
}

/**
 * The function that will be called when a modifier must be destroyed
 */
void modifier_destroy(void *data)	{
	_modifier* m = (_modifier*)data;
	
	free(m->name);
	free(m);
}


/**
 * Creates a new template_dictionary, ready to be filled with values and sections
 *
 * Returns the template_dictionary created, or NULL if this could not be done.  It is up
 * to the caller to manage this dictionary
 */
template_dictionary* template_new()	{
	template_dictionary* d;
	
	// NOTE: Adjust the buckets parameter depending on how many markers are likely to be in a template
	//		file (then adjust upward to the next prime number)
	d = (template_dictionary*)malloc(sizeof(template_dictionary));
	ht_init((hashtable*)d, 197, dictionary_hash, dictionary_match, dictionary_destroy);
	ht_init(&d->modifiers, 23, modifier_hash, modifier_match, modifier_destroy);
	return d;
}

/**
 * Destroys the given template dictionary and any sub dictionaries
 * Signature comforms to hashtable and list function pointer signature
 */
static void _destroy(void* data)	{
	template_dictionary* dict = (template_dictionary*)data;
	
	ht_destroy(&dict->ht);
	ht_destroy(&dict->modifiers);
	if (dict->template)	{
		free(dict->template);
	}
	free(dict);
}

/** 
 * Destroys the given template dictionary and any sub-dictionaries
 */
void template_destroy(template_dictionary* dict)	{
	_destroy((void*)dict);
}

/**
 * Helper function to retrieve a template string from a file
 */
static char* _get_template_from_file(FILE* fd)	{
	int length;
	int ch, i;
	char* contents;
	
	fseek(fd, 0, SEEK_END);
	length = ftell(fd);
	if (!length)	{
		close(fd);
		return 0;
	}
	rewind(fd);
	
	// TODO: Do this with block reads so it's faster
	i = 0;
	contents = (char*)malloc(length+1);
	while ((ch = fgetc(fd)) != EOF)	{
		contents[i++] = ch;
	}
	close(fd);
	
	return contents;
}

/** 
 * Callback function to retrieve a template string from a file name
 */
static char* _get_template_from_filename(const char* filename)	{
	FILE* fd;
	char* template;
	
	fd = fopen(filename, "rb");
	if (!fd)	{
		return 0;
	}
	
	template = _get_template_from_file(fd);
	close(fd);
	
	return template;
}

/**
 * Loads the template string from the given file pointer.  Does NOT close the pointer
 *
 * Returns 0 if successful, -1 otherwise
 */
int template_load_from_file(template_dictionary* dict, FILE* fp)	{
	char* template;
	
	template = _get_template_from_file(fp);
	if (!template)	{
		return -1;
	}
	
	if (dict->template)	{
		free(dict->template);
	}
	
	dict->template = template;
	return 0;
}

/**
 * Loads the template string from the given file name
 *
 * Returns 0 if successful, -1 otherwise
 */
int template_load_from_filename(template_dictionary* dict, const char* filename)	{
	char* template;
	
	template = _get_template_from_filename(filename);
	if (!template)	{
		return -1;
	}
	
	if (dict->template)	{
		free(dict->template);
	}
	
	dict->template = template;
	return 0;
}

/**
 * Helper function - allocates and initializes a new _dictionary_item
 */
_dictionary_item* _new_dictionary_item()	{
	_dictionary_item* item;
	
	item = (_dictionary_item*)malloc(sizeof(_dictionary_item));
	memset(item, 0, sizeof(_dictionary_item));
	
	return item;
}

/**
 * Sets a modifier function that will be called when a modifier in the template does not
 * resolve to any known modifiers.  The function will have the opportunity to adjust the output
 * of the marker, and will be passed any arguments.
 */
void template_set_modifier_missing_cb(template_dictionary* dict, modifier_fn mod_fn)	{
	dict->modifier_missing = mod_fn;
}

/**
 * Sets a callback function that will be called when no value for a variable marker can be
 * found.  The function will have the opportunity to give the value of the variable by appending
 * to the out_sb string builder.
 */
void template_set_variable_missing_cb(template_dictionary* dict, get_variable_fn get_fn)	{
	dict->variable_missing = get_fn;
}

/**
 * Sets a modifier function that can be called when the given modifier name is encountered
 * in the template.  The modifier will have the opportunity to adjust the output of the 
 * marker, and will be passed in any arguments.  If another modifier is already present with
 * the same name, that modifier will be replaced.
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_add_modifier(template_dictionary* dict, const char* name, modifier_fn mod_fn)	{
	_modifier* mod, *prev_mod;
	
	mod = (_modifier*)malloc(sizeof(_modifier));
	mod->name = (char*)malloc(strlen(name) + 1);
	strcpy(mod->name, name);
	mod->modifier = mod_fn;
	
	if (ht_insert((hashtable*)dict, mod) == 1)	{
		// Already in the table, replace
		prev_mod = mod;
		ht_remove((hashtable*)dict, (void *)&prev_mod);
		dictionary_destroy((void*)prev_mod);
		
		ht_insert((hashtable*)dict, mod);
	}
	
	return 0;	
}

/**
 * Helper function for the template_set_* functions.  Does NOT make a copy of the given value
 * string, but uses the pointer directly.
 */
int _set_string(template_dictionary* dict, const char* marker, char* value)	{
	_dictionary_item* item, *prev_item;
	
	item = _new_dictionary_item();
	item->type = ITEM_STRING;
	item->marker = (char*)malloc(strlen(marker) + 1);
	item->val.string_value = value;
	
	strcpy(item->marker, marker);
		
	if (ht_insert((hashtable*)dict, item) == 1)	{
		// Already in the table, replace
		prev_item = item;
		ht_remove((hashtable*)dict, (void *)&prev_item);
		dictionary_destroy((void*)prev_item);
		
		ht_insert((hashtable*)dict, item);
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
int template_set_string(template_dictionary* dict, const char* marker, const char* value)	{
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
int template_set_stringf(template_dictionary* dict, const char* marker, const char* fmt, ...)	{
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
int template_set_int(template_dictionary* dict, const char* marker, int value)	{
	return template_set_stringf(dict, marker, "%d", value);
}

/**
 * On an include template, sets the callbacks to be called when the system needs the template
 * string for the given include name.  Also, prove a cleanup_template function to call when the
 * system no longer needs the template data.
 * NOTES: It is illegal to call this function on a string value marker
 *        It is legal to pass null to cleanup_template
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_set_include_cb(template_dictionary* dict, const char* marker, get_template_fn get_template, 
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
 * Callback function for cleanup of default file template-includes
 */
void _cleanup_template(const char* filename, char* template)	{
	free(template);
}

/** 
 * Helper Function - Queries the dictionary for the item under the given marker and returns the 
 * item if it exists
 * 
 * Returns a pointer to the item if it exists, zero if not
 */
static _dictionary_item* _query_item(template_dictionary* dict, const char* marker)	{
	_dictionary_item* query_item, *item;
	query_item = (_dictionary_item*)malloc(sizeof(_dictionary_item));
	query_item->marker = (char*)marker;
	query_item->val.string_value = 0;
	
	item = query_item;
	
	if (ht_lookup((hashtable*)dict, (void*)&item) != 0)	{
		// No value for this key
		free(query_item);
		return 0;
	}
	
	free(query_item);
	
	return item;
}

/**
 * Helper function - Queries the dictionary for the modifier for the given name and returns the
 * modifier if it exists
 */
static _modifier* _query_modifier(template_dictionary* dict, const char* name)	{
	_modifier* query_mod, *mod;
	query_mod = (_modifier*)malloc(sizeof(_modifier));
	query_mod->name = (char*)name;
	
	mod = query_mod;
	
	if (ht_lookup((hashtable*)dict, (void*)&mod) != 0)	{
		// No value for this key
		free(query_mod);
		return 0;
	}
	
	free(query_mod);
	
	return mod;	
}

/**
 * On an include template, sets the filename that will be loaded to obtain the template data
 * NOTE: It is illegal to call this function on a string value marker
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_set_include_filename(template_dictionary* dict, const char* marker, const char* filename)	{
	_dictionary_item* item;
	
	if (template_set_include_cb(dict, marker, _get_template_from_filename, _cleanup_template) != 0)	{
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
int template_add_dictionary(template_dictionary* dict, const char* marker, template_dictionary* child)	{
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
		list_init(item->val.d_list_value, _destroy);
	}
	
	list_insert_next(item->val.d_list_value, list_tail(item->val.d_list_value), child);
	child->parent = dict;
	
	return 0;
}

/**
 * Gets the string value in the dictionary for the given marker
 * NOTE: The string returned is managed by the dictionary.  Do NOT retain a reference to
 *		it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found
 */
static const char* _get_string_value_ref(template_dictionary* dict, const char* marker)	{
	_dictionary_item* item;
	if (!dict)	{
		return 0;
	}
	
	item = _query_item(dict, marker);
	if (!item)	{
		// Look in the parent dictionary
		return _get_string_value_ref(dict->parent, marker);
	}
	
	if (item->type != ITEM_STRING)	{
		// Getting a non-string item as a string is not defined
		return 0;
	}
	
	return item->val.string_value;
}

/**
 * Gets the dictionary list value in the dictionary for the given marker
 * NOTE: The dictionary list returned is managed by the dictionary.  Do NOT retain a reference to
 *		it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found or if the given node is not
 * a D_LIST
 */
static const list* _get_dictionary_list_ref(template_dictionary* dict, const char* marker)	{
	_dictionary_item* item;
	if (!dict)	{
		return 0;
	}
	
	item = _query_item(dict, marker);
	if (!item)	{
		// Look in the parent dictionary
		return _get_dictionary_list_ref(dict->parent, marker);
	}
	
	if (!(item->type & ITEM_D_LIST))	{
		// Getting a non-d-list item as a d-list is not defined
		return 0;
	}
	
	return item->val.d_list_value;
}

/**
 * Gets the include params struct in the dictionary for the given marker
 * NOTE: The include params struct returned is managed by the dictionary.  Do NOT retain a reference
 *       to it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found or if the given node is not
 * an INCLUDE
 */
static struct _include_params_tag* _get_include_params_ref(template_dictionary* dict, const char* marker)	{
	_dictionary_item* item;
	if (!dict)	{
		return 0;
	}
	
	item = _query_item(dict, marker);
	if (!item)	{
		// Look in the parent dictionary
		return _get_include_params_ref(dict->parent, marker);
	}
	
	if (item->type != ITEM_INCLUDE)	{
		// Getting a non-include item as an include is not defined
		return 0;
	}
	
	return &item->val.include_value;
}

/* Parse modes */
#define MODE_NORMAL					0
#define MODE_MARKER					1			
#define MODE_MARKER_COMMENT			2
#define MODE_MARKER_VARIABLE		4
#define MODE_MARKER_SECTION			8
#define MODE_MARKER_ENDSECTION		16	
#define MODE_MARKER_DELIMITER		32
#define MODE_MARKER_INCLUDE			64
#define MODE_MARKER_MODIFIER		128

/**
 * Helper function - returns nonzero if the portion of the input string starting at p matches the given
 * marker, 0 otherwise
 */
static int _match_marker(const char* p, _delimiter* delim)	{
	int i;
	
	i = 0;
	while (i < delim->length && *p && *p == delim->literal[i])	{
		p++, i++;
	}
	
	return i == delim->length;
}

/**
 * Helper function. Copies the data from the source delimiter into dest
 */
static void _copy_delimiter(_delimiter* dest, const _delimiter* src)	{
	if (!dest || !src)	{
		return;
	}
	
	strncpy(dest->literal, src->literal, src->length);
	dest->length = src->length;
}

/**
 * Helper function.  Returns a freshly allocated copy of the given delimiter
 */
static _delimiter* _duplicate_delimiter(const _delimiter* delim)	{
	_delimiter* new_delim;
	
	new_delim = (_delimiter*)malloc(sizeof(_delimiter));
	_copy_delimiter(new_delim, delim);
	
	return new_delim;
}

/**
 * Helper function - assuming that p points to the character after '<start delim>=', reads the string up 
 * 					until a space or '=<end delim>', then modifies the delimiter to be the new delimiter
 *
 * Returns the adjusted character pointer, pointing to one after the last position processed
 */
static char* _extract_delimiter(const char* p, _delimiter* delim)	{
	int length;
	
	length = 0;
	while (*p && *p != '=' && *p != ' ' && *p != '\t')	{
		delim->literal[length++] = *p++;
		
		if (length >= MAX_DELIMITER_LENGTH)	{
			fprintf(stderr, "FATAL: Delimiter cannot be greater than %d characters\n", MAX_DELIMITER_LENGTH);
			exit(-1);
		}
	}
	
	delim->length = length;
		
	return (char*)p;
}

/**
 * Helper function - creates a new context and duplicates the contents of the given context
 */ 
static _parse_context* _duplicate_context(_parse_context* ctx)	{
	_parse_context* new_ctx;
	
	new_ctx = (_parse_context*)malloc(sizeof(_parse_context));
	memcpy(new_ctx, ctx, sizeof(_parse_context));
	
	return new_ctx;
}

#define EAT_SPACES(p)		while(*(p) == ' ' || *(p) == '\t') { (p)++; }
#define EAT_WHITESPACE(p)	while(*(p) == ' ' || *(p) == '\t' || *(p) == '\r' || *(p) == '\n') { (p)++; }

/**
 * Helper function - appends the marked whitespace at the current position in the output
 */
static void _append_line_whitespace(_parse_context* line_ctx, _parse_context* out_ctx)	{
	char* ptr;
	
	if (line_ctx->parent)	{
		_append_line_whitespace(line_ctx->parent, out_ctx);
	} else {
		return;
	}
	
	if (!line_ctx->expanding_include)	{
		return;
	}
	
	if (line_ctx->parent->line_ws_range == 0)	{
		return;
	}
	
	ptr = line_ctx->parent->line_ws_start;
	while(ptr)	{
		
		sb_append_ch(out_ctx->out_sb, *ptr++);
		
		if ((ptr - line_ctx->parent->line_ws_start) == line_ctx->parent->line_ws_range)	{
			// We're done
			ptr = 0;
		}
	}
}

/**
 * Helper function - Processes a set delimiter {{= =}} sequence in the template
 */
static void _process_set_delimiter(_parse_context* ctx)	{
	ctx->in_ptr = _extract_delimiter(ctx->in_ptr, &(ctx->start_delimiter));
	EAT_SPACES(ctx->in_ptr);
	
	// OK now we should either be at the new end delimiter or an =
	if (*ctx->in_ptr == '=')	{
		ctx->in_ptr++;
		if (!_match_marker(ctx->in_ptr, &(ctx->end_delimiter)))	{
			fprintf(stderr, "Unexpected '=' in middle of Set Delimiter\n");
			exit(-1);
		}
		
		ctx->in_ptr += ctx->end_delimiter.length;
		
		// In these cases, the start and end delimiters are the same
		_copy_delimiter(&(ctx->end_delimiter), &(ctx->start_delimiter));
	} else {
		_delimiter* old_end_delimiter = _duplicate_delimiter(&(ctx->end_delimiter));
		ctx->in_ptr = _extract_delimiter(ctx->in_ptr, &(ctx->end_delimiter));
		EAT_SPACES(ctx->in_ptr);
		
		if (*ctx->in_ptr != '=')	{
			fprintf(stderr, "Unexpected '%c' after end delimiter in Set Delimiter\n", *ctx->in_ptr);
			exit(-1);
		}
		ctx->in_ptr++;
		
		if (!_match_marker(ctx->in_ptr, old_end_delimiter))	{
			fprintf(stderr, "Unexpected characters after '=' in Set Delimiter\n");
			exit(-1);
		}
		
		ctx->in_ptr += old_end_delimiter->length;
		free(old_end_delimiter);
	}
}

/**
 * Helper function - Expands a variable marker in the template
 */
static void _process_variable(const char* marker, const char* modifiers, _parse_context* ctx)	{
	char* value;
	int applied_modifier;
	
	value = (char*)_get_string_value_ref(ctx->dict, marker);
	if (!value) {
		
		// Find the first parse context up the chain with a valid
		// template dictionary and variable missing cb
		_parse_context* this_ctx = ctx;
		template_dictionary* dict = 0;
		while (this_ctx)	{
			if (this_ctx && this_ctx->dict && this_ctx->dict->variable_missing)	{
				dict = this_ctx->dict;
				break;
			}
			
			this_ctx = this_ctx->parent;
		}
		
		if (dict && dict->variable_missing)	{
			// Give user code a chance to fill in this value
			value = dict->variable_missing(marker);
		}
		
		if (!value)	{
			return;
		}
	}
	
	applied_modifier = 0;	
	if (ctx->mode & MODE_MARKER_MODIFIER)	{
		char *p;
		p = (char*)modifiers;
		
		while (1)	{
			char modifier[MAXMODIFIERLENGTH];
			int m;
			_modifier* mod;
			
			m = 0;
			while (*p && *p != ':')	{
				modifier[m++] = *p++;
			}
			modifier[m] = '\0';
			
			mod = _query_modifier(ctx->dict, modifier);
			if (mod)	{
				mod->modifier(modifier, "", marker, value, ctx->out_sb);
				applied_modifier = 1;
			} else {
				// Find the first parse context up the chain with a valid
				// template dictionary and modifier missing cb
				_parse_context* this_ctx = ctx;
				template_dictionary* dict = 0;
				while (this_ctx)	{
					if (this_ctx && this_ctx->dict && this_ctx->dict->modifier_missing)	{
						dict = this_ctx->dict;
						break;
					}

					this_ctx = this_ctx->parent;
				}

				if (dict && dict->modifier_missing)	{
					// Give user code a chance to fill in this value
					dict->modifier_missing(modifier, "", marker, value, ctx->out_sb);
					applied_modifier = 1;
				} 
			}
			
			if (*p++ != ':')	{
				break;
			}
		}
	}
	
	if (!applied_modifier)	{
		// Output the value ourselves
		sb_append_str(ctx->out_sb, value);
	}
		
}

static char* _process(_parse_context *ctx);	// Prototype for forward declaration

/**
 * Helper function - Determines if the marker is a special separator section and, if so, 
 *					processes it
 *
 * Returns nonzero (true) if it was a separator section, zero otherwise
 */
static int _process_separator_section(const char* marker, _parse_context* ctx)	{
	char* separator_name;
	char* saved_section;
	char* resume;
	int saved_output_pos;
	
	// Before doing normal expansion logic, check to see if this is a separator
	separator_name = (char*)malloc(strlen(ctx->current_section) + strlen("_separator") + 1);
	sprintf(separator_name, "%s_separator", ctx->current_section);
	if (strcmp(marker, separator_name))	{
		// Not a separator
		free(separator_name);
		return 0;
	}
	
	free(separator_name);
	saved_output_pos = ctx->out_sb->pos;
						
	// We leave it up to the implementor to put the separator in the right place.  We expand it
	// here and now
	saved_section = ctx->current_section;
	ctx->current_section = (char*)marker;
		
	resume = _process(ctx);
		
	if (ctx->last_expansion)	{
		// We don't expand separators unless we're not the last or only one
		ctx->out_sb->pos = saved_output_pos;
	}
		
	ctx->in_ptr = resume;
	ctx->current_section = saved_section;
	
	return 1;
}

/**
 * Helper function - Expands a section in the template
 */
static void _process_section(const char* marker, _parse_context* ctx, int is_include)	{
	list *d_list_value;
	list_element* child;
	char* resume;
	_parse_context* section_ctx;
	int saved_out_pos;
	
	if (_process_separator_section(marker, ctx))	{
		// Section was a separator, which has to be handled differently
		return;
	}
	
	// We loop through each dictionary in the dictionary list for this marker 
	// (0 or more), and for each one we recursively process the template there
	d_list_value = (list*)_get_dictionary_list_ref(ctx->dict, marker);
	
	if (is_include)	{
		section_ctx = ctx;
	} else {
		section_ctx = _duplicate_context(ctx);
		section_ctx->parent = ctx;
		section_ctx->expanding_include = 0;
	}
	
	section_ctx->current_section = (char*)marker;
	resume = section_ctx->in_ptr;
	
	child = 0;
	if (d_list_value)	{
		child = list_head(d_list_value);
	}
	
	do {
		section_ctx->last_expansion = (child && list_next(child) == 0) ? 1: 0;
		section_ctx->dict = child? (template_dictionary*)list_data(child) : 0;
		section_ctx->in_ptr = ctx->in_ptr;
		saved_out_pos = ctx->out_sb->pos;
			
		resume = _process(section_ctx);
						
		if (resume < 0)	{
			// There was an error in the inner section
			fprintf(stderr, "Error expanding %s section\n", marker);
			exit(-1);
		}
			
		if (!child)	{
			// We only use the output if we had an actual dictionary, else we have to throw it 
			// out because we didn't ultimately expand anything
			ctx->out_sb->pos = saved_out_pos;
		}
	
	} while (child && (child = list_next(child)) != 0);
	
	if (!is_include)	{
		free(section_ctx);
	}
	ctx->in_ptr = resume;	// Now we can skip over the read template section
}

/**
 * Helper function - Processes template include markers
 */
static void _process_include(const char* marker, _parse_context* ctx)	{
	struct _include_params_tag* params;
	_parse_context* include_ctx;
	
	params = _get_include_params_ref(ctx->dict, marker);
	if (!params || !params->get_template)	{
		// Can't do anything with this one
		return;
	}
		
	// The way this works is if someone has set a filename for us, we'll pass that to 
	// the get_template callback, else we'll just pass the marker name and hope they know
	// what to do with it
	if (!params->template)	{
		if (params->filename)	{
			params->template = params->get_template(params->filename);
		} else {
			params->template = params->get_template(marker);
		}
	}

	if (!params->template)	{
		// We got nothin'
		return;
	}	
	
	include_ctx = _duplicate_context(ctx);
	include_ctx->parent = ctx;
	include_ctx->in_ptr = params->template;
	include_ctx->template_line = 1;
	include_ctx->expanding_include = 1;
	
	// Now we can treat it just like a normal section, then restore the original template
	_process_section(marker, include_ctx, 1);
	
	free(include_ctx);
}

/**
 * Internal implementation of the template process function
 *
 * Returns -1 if there was an error, otherwise records a pointer to the position in the template when the function ended
 */ 
static char* _process(_parse_context *ctx)	{
	int m, mod;
	char marker[MAXMARKERLENGTH];
	char modifiers[MAXMODIFIERLENGTH];

	ctx->mode = MODE_NORMAL;
	m = mod = 0;
	
	while (*ctx->in_ptr)	{
		if (ctx->mode & MODE_MARKER)	{
			EAT_WHITESPACE(ctx->in_ptr);
			
			if (ctx->mode & MODE_MARKER_DELIMITER)	{
				_process_set_delimiter(ctx);
				ctx->mode = MODE_NORMAL;
				continue;

			} else if (_match_marker(ctx->in_ptr, &(ctx->end_delimiter)))	{
				// At the end of the marker
				marker[m] = '\0';
				modifiers[mod] = '\0';
				ctx->in_ptr += ctx->end_delimiter.length;
				
				if (ctx->mode & MODE_MARKER_VARIABLE)	{
					_process_variable(marker, modifiers, ctx);
					
				} else if (ctx->mode & MODE_MARKER_SECTION)	{
					_process_section(marker, ctx, 0);
					 
				} else if (ctx->mode & MODE_MARKER_ENDSECTION)	{
					
					if (strcmp(ctx->current_section, marker) != 0)	{
						fprintf(stderr, "End section '%s' does not match start section '%s'\n", marker, ctx->current_section);
						return (char*)-1;
					} else {					
						// We're done expanding this section
						return ctx->in_ptr;
					}
				} else if (ctx->mode & MODE_MARKER_INCLUDE)	{
					_process_include(marker, ctx);
					
				}
				
				ctx->mode = MODE_NORMAL;
				continue;
			} else if (ctx->mode == MODE_MARKER)	{
				switch(*ctx->in_ptr)	{
					case '!':	ctx->mode |= MODE_MARKER_COMMENT;		ctx->in_ptr++;	break;
					case '#':	ctx->mode |= MODE_MARKER_SECTION;		ctx->in_ptr++;	break;
					case '/':	ctx->mode |= MODE_MARKER_ENDSECTION;	ctx->in_ptr++;	break;
					case '=': 	ctx->mode |= MODE_MARKER_DELIMITER; 	ctx->in_ptr++;	break;
					case '>':	ctx->mode |= MODE_MARKER_INCLUDE;		ctx->in_ptr++;	break;
					default: 	ctx->mode |= MODE_MARKER_VARIABLE;		break;
				}
				
				continue;
			} else if (ctx->mode & MODE_MARKER_VARIABLE)	{
				if (*ctx->in_ptr == ':' && !(ctx->mode & MODE_MARKER_MODIFIER))	{
					ctx->mode |= MODE_MARKER_MODIFIER;
					ctx->in_ptr++;
					continue;
				} 
				
			} else if (!isalnum(*ctx->in_ptr) && *ctx->in_ptr != '_' && !(ctx->mode & MODE_MARKER_COMMENT))	{
				// Can't have embedded funky characters inside
				fprintf(stderr, "Illegal character (%c) inside template marker\n", *ctx->in_ptr);
				return (char*)-1;
			}
									
			if (ctx->mode & MODE_MARKER_COMMENT)	{
				// Ignore the actual comment contents
			} else {
				if (ctx->mode & MODE_MARKER_MODIFIER)	{
					if (!(ctx->mode & MODE_MARKER_VARIABLE))	{
						fprintf(stderr, "Illegal modifier on line %d\n", ctx->template_line);
						return (char*)-1;
					}
					
					modifiers[mod++] = *ctx->in_ptr;
					if (mod == MAXMODIFIERLENGTH)	{
						modifiers[mod-1] = 0;
						fprintf(stderr, "Template modifier \"%s\" exceeds maximum modifier length of %d characters\n", modifiers, MAXMODIFIERLENGTH);
						return (char*)-1;
					}
					
				} else {
					marker[m++] = *ctx->in_ptr;

					if (m == MAXMARKERLENGTH)	{
						marker[m-1] = 0;
						fprintf(stderr, "Template marker \"%s\" exceeds maximum marker length of %d characters\n", marker, MAXMARKERLENGTH);
						return (char*)-1;
					}
				}	
			}
			
			ctx->in_ptr++;			
			continue;
		}
		
		if (_match_marker(ctx->in_ptr, &(ctx->start_delimiter)))	{
			ctx->in_ptr += ctx->start_delimiter.length;	// Skip over those characters
			ctx->mode = MODE_MARKER;
			memset(marker, 0, MAXMARKERLENGTH);
			memset(modifiers, 0, MAXMODIFIERLENGTH);
			m = 0;
			mod = 0;
		} else {
			if (*ctx->in_ptr == '\n')	{
				// Newline, reset the line whitespace pointer
				ctx->line_ws_start = ctx->in_ptr+1;
				ctx->line_ws_range = 0;
				ctx->template_line++;
				
			} else if ((ctx->in_ptr - ctx->line_ws_range == ctx->line_ws_start) &&
				(*ctx->in_ptr == ' ' || *ctx->in_ptr == '\t'))	{
				// We're still in a chunk of whitespace before any real content, 
				// keep track of that so we can expand template includes correctly
				ctx->line_ws_range++;				
			}
			
			sb_append_ch(ctx->out_sb, *ctx->in_ptr++);
			
			if (ctx->template_line > 1 && *(ctx->in_ptr-1) == '\n')	{
				// We're currenlty expanding an include section, which means we need
				// to copy the accumulated whitespace at the beginning of each line
				// other than the first one
				_append_line_whitespace(ctx, ctx);
			}
			
		}
	}
	
	return ctx->in_ptr;
}

/**
 * Processes the given template according to the dictionary, putting the result in "result" pointer.
 * Sufficient space will be allocated for the result, and it will then be 
 * up to the caller to manage this space
 *
 * Returns 0 if the template was successfully processed, -1 if there was an error
 */
int template_process(template_dictionary* dict, char** result)	{
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
	
	context.dict = dict;
	context.in_ptr = (char*)dict->template;
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
void template_print_dictionary(template_dictionary* dict, FILE* out)	{
	
	hashtable_iter* it = ht_iter_begin((hashtable*)dict);
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
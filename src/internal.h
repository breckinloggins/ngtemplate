/**
 * Internal functions for the ngtemplate engine
 */
#ifndef INTERNAL_H
#define INTERNAL_H

#include "include/ngtemplate.h"

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


#define EAT_SPACES(p)		while(*(p) == ' ' || *(p) == '\t') { (p)++; }
#define EAT_WHITESPACE(p)	while(*(p) == ' ' || *(p) == '\t' || *(p) == '\r' || *(p) == '\n') { (p)++; }


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
int _dictionary_hash(const void* key);

/**
 * The function we will use to determine if two dictionary_items match
 */
int _dictionary_match(const void* key1, const void* key2);	

/**
 * The function that will be called when a dictionary_item must be destroyed
 */
void _dictionary_destroy(void *data);

/**
 * The hash function we will use for the modifier dictionary
 */
int _modifier_hash(const void* key);

/**
 * The function we will use to determine if two modifiers match
 */
int _modifier_match(const void* key1, const void* key2);

/**
 * The function that will be called when a modifier must be destroyed
 */
void _modifier_destroy(void *data);

/**
 * Destroys the given template dictionary and any sub dictionaries
 * Signature comforms to hashtable and list function pointer signature
 */
void _destroy(void* data);

/**
 * Helper function to retrieve a template string from a file
 */
char* _get_template_from_file(FILE* fd);

/** 
 * Callback function to retrieve a template string from a file name
 */
char* _get_template_from_filename(const char* filename);

/**
 * Helper function - allocates and initializes a new _dictionary_item
 */
_dictionary_item* _new_dictionary_item();

/**
 * Helper function for the template_set_* functions.  Does NOT make a copy of the given value
 * string, but uses the pointer directly.
 */
int _set_string(template_dictionary* dict, const char* marker, char* value);

/**
 * Callback function for cleanup of default file template-includes
 */
void _cleanup_template(const char* filename, char* template);

/** 
 * Helper Function - Queries the dictionary for the item under the given marker and returns the 
 * item if it exists
 * 
 * Returns a pointer to the item if it exists, zero if not
 */
_dictionary_item* _query_item(template_dictionary* dict, const char* marker);

/**
 * Helper function - Queries the dictionary for the modifier for the given name and returns the
 * modifier if it exists
 */
_modifier* _query_modifier(template_dictionary* dict, const char* name);

/**
 * Gets the string value in the dictionary for the given marker
 * NOTE: The string returned is managed by the dictionary.  Do NOT retain a reference to
 *		it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found
 */
const char* _get_string_value_ref(template_dictionary* dict, const char* marker);

/**
 * Gets the dictionary list value in the dictionary for the given marker
 * NOTE: The dictionary list returned is managed by the dictionary.  Do NOT retain a reference to
 *		it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found or if the given node is not
 * a D_LIST
 */
const list* _get_dictionary_list_ref(template_dictionary* dict, const char* marker);

/**
 * Gets the include params struct in the dictionary for the given marker
 * NOTE: The include params struct returned is managed by the dictionary.  Do NOT retain a reference
 *       to it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found or if the given node is not
 * an INCLUDE
 */
struct _include_params_tag* _get_include_params_ref(template_dictionary* dict, const char* marker);

/**
 * Helper function - returns nonzero if the portion of the input string starting at p matches the given
 * marker, 0 otherwise
 */
int _match_marker(const char* p, _delimiter* delim);

/**
 * Helper function. Copies the data from the source delimiter into dest
 */
void _copy_delimiter(_delimiter* dest, const _delimiter* src);
	
/**
 * Helper function.  Returns a freshly allocated copy of the given delimiter
 */
_delimiter* _duplicate_delimiter(const _delimiter* delim);	

/**
 * Helper function - assuming that p points to the character after '<start delim>=', reads the string up 
 * 					until a space or '=<end delim>', then modifies the delimiter to be the new delimiter
 *
 * Returns the adjusted character pointer, pointing to one after the last position processed
 */
char* _extract_delimiter(const char* p, _delimiter* delim);

/**
 * Helper function - creates a new context and duplicates the contents of the given context
 */ 
_parse_context* _duplicate_context(_parse_context* ctx);

/**
 * Helper function - appends the marked whitespace at the current position in the output
 */
void _append_line_whitespace(_parse_context* line_ctx, _parse_context* out_ctx);

/**
 * Helper function - Processes a set delimiter {{= =}} sequence in the template
 */
void _process_set_delimiter(_parse_context* ctx);

/**
 * Helper function - Expands a variable marker in the template
 */
void _process_variable(const char* marker, const char* modifiers, _parse_context* ctx);

/**
 * Helper function - Determines if the marker is a special separator section and, if so, 
 *					processes it
 *
 * Returns nonzero (true) if it was a separator section, zero otherwise
 */
int _process_separator_section(const char* marker, _parse_context* ctx);

/**
 * Helper function - Expands a section in the template
 */
void _process_section(const char* marker, _parse_context* ctx, int is_include);

/**
 * Helper function - Processes template include markers
 */
void _process_include(const char* marker, _parse_context* ctx);

/**
 * Internal implementation of the template process function
 *
 * Returns -1 if there was an error, otherwise records a pointer to the position in the template when the function ended
 */ 
char* _process(_parse_context *ctx);

#endif // INTERNAL_H
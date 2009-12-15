/**
 * Public interface to the ngtemplate engine
 */
#ifndef NGTEMPLATE_H
#define NGTEMPLATE_H

#include <stdio.h>
#include "../../lib/libuseful/src/include/hashtable.h"
#include "../../lib/libuseful/src/include/stringbuilder.h"

#define MAXMARKERLENGTH		64
#define MAXMODIFIERLENGTH	128

/* Pointer to a function that will return a pointer to a template string given its name */ 
typedef char* (*get_template_fn)(const char* name);

/* Pointer to a function that will clean up the template data given the data and its name */ 
typedef void (*cleanup_template_fn)(const char* name, char* template);

/** 
 * Pointer to a function that will be called to modify the output of a template process for a marker
 *  name - The name of the modifier being called
 *	marker - The name of the marker being evaluated
 *  value - The value that the template processor will be using if nothing is done
 *	out_sb - The stringbuilder to which to append changes
 *  args - The arguments, if any
 */
typedef void (*modifier_fn)(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb);

/**
 * Pointer to a function that will be called to get the value of the given variable marker
 *   marker - The marker name
 *
 * Return an allocated string containing the value, or 0
 */
typedef char* (*get_variable_fn)(const char* marker);

typedef struct template_dictionary_tag	{
	hashtable ht;
	hashtable modifiers;
	
	char*	template;
	
	modifier_fn			modifier_missing;
	get_variable_fn		variable_missing;
	
	struct template_dictionary_tag* parent;
} template_dictionary;

/**
 * Creates a new template_dictionary, ready to be filled with values and sections
 *
 * Returns the template_dictionary created, or NULL if this could not be done.  It is up
 * to the caller to manage this dictionary
 */
template_dictionary* template_new();

/** 
 * Destroys the given template dictionary and any sub-dictionaries
 */
void template_destroy(template_dictionary* dict);

/**
 * Loads the template string from the given file pointer.  Does NOT close the pointer
 *
 * Returns 0 if successful, -1 otherwise
 */
int template_load_from_file(template_dictionary* dict, FILE* fp);

/**
 * Loads the template string from the given file name
 *
 * Returns 0 if successful, -1 otherwise
 */
int template_load_from_filename(template_dictionary* dict, const char* filename);

/**
 * Sets a modifier function that can be called when the given modifier name is encountered
 * in the template.  The modifier will have the opportunity to adjust the output of the 
 * marker, and will be passed in any arguments.  If another modifier is already present with
 * the same name, that modifier will be replaced.
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_add_modifier(template_dictionary* dict, const char* name, modifier_fn mod_fn);

/**
 * Sets a modifier function that will be called when a modifier in the template does not
 * resolve to any known modifiers.  The function will have the opportunity to adjust the output
 * of the marker, and will be passed any arguments.
 */
void template_set_modifier_missing_cb(template_dictionary* dict, modifier_fn mod_fn);

/**
 * Sets a callback function that will be called when no value for a variable marker can be
 * found.  The function will have the opportunity to give the value of the variable by appending
 * to the out_sb string builder.
 */
void template_set_variable_missing_cb(template_dictionary* dict, get_variable_fn get_fn);

/**
 * Sets a string value in the template dictionary.  Any instance of "marker" in the template 
 * will be replaced by "value"
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_set_string(template_dictionary* dict, const char* marker, const char* value);

/**
 * Sets a string value in the template dictionary using printf-style format specifiers.  Any
 * instance of "marker" in the template will be replaced by "value"
 *
 * Returns 0 if the operations succeeded, -1 otherwise
 */
int template_set_stringf(template_dictionary* dict, const char* marker, const char* fmt, ...);

/**
 * Sets an integer value in the template dictionary.  Any
 * instance of "marker" in the template will be replaced by "value"
 *
 * Returns 0 if the operations succeeded, -1 otherwise
 */
int template_set_int(template_dictionary* dict, const char* marker, int value);

/**
 * On an include template, sets the filename that will be loaded to obtain the template data
 * NOTES: - It is illegal to call this function on a string value marker
 *        - Calling template_set_include_cb is not necessary if you set a filename, as the file will
 *		    be looked up for you.  However, if you need to perform custom logic (such as searching
 *          through a list of pre-defined directories for the file), you can do the load logic yourself
 *          by registering a callback function with template_set_include_cb.  When the function is called, 
 *          the "name" argument will be the filename you set instead of the include marker name
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_set_include_filename(template_dictionary* dict, const char* marker, const char* filename);

/**
 * On an include template, sets the callbacks to be called when the system needs the template
 * string for the given include name.  Also, prove a cleanup_template function to call when the
 * system no longer needs the template data.
 * NOTES: - It is illegal to call this function on a string value marker
 *        - It is legal to pass null to cleanup_template
 *		  - The first call to the callback for every unique name will be memoized.  The function will 
 *          NOT be called for every expansion of the marker
 * 
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_set_include_cb(template_dictionary* dict, const char* marker, get_template_fn get_template, 
							cleanup_template_fn cleanup_template);

/**
 * Adds a child dictionary under the given marker.  If there is an existing dictionary under this marker, 
 * the new dictionary will be ADDED to the end of the list, NOT replace the old one
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int template_add_dictionary(template_dictionary* dict, const char* marker, template_dictionary* child);

/**
 * Processes the given template according to the dictionary, putting the result in "result" pointer.
 * Sufficient space will be allocated for the result, and it will then be 
 * up to the caller to manage this memory
 *
 * Returns 0 if the template was successfully processed, -1 if there was an error
 */
int template_process(template_dictionary* dict, char** result);

/**
 * Pretty-prints the dictionary key value pairs, one per line, with nested dictionaries tabbed
 */
void template_print_dictionary(template_dictionary* dict, FILE* out);

#endif // NGTEMPLATE_H
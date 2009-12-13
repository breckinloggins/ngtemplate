TEMPLATE is a small, fast template engine written in C.  It aims to be fully syntax-compatible with Google's CTemplate.  In addition, the TEMPLATE API is designed to closely mirror the CTemplate API.

Differences from CTemplate:
	- Unlike in CTemplate, you can have as many FOO_separator sections as you like inside a template and they will all be expanded in the order in which the separators are placed
	- Dictionaries have block scope, meaning that names in the parent dictionary are available in inner dictionaries.  In particular, this means sections can access markers from their parent sections or the root template

Known Issues and Limitations for Version 0.1:
	- You cannot currently specify search directories for templates.  Calls to template_set_filename() need to have absolute or resolvable relative paths to the template file
	- Pragmas (including AUTOESCAPE) are not currently supported
	- Modifier support is not yet as capable as CTemplate:
		- Modifiers are supported, but none are currently built-in
		- Modifiers are supported on values only, not sections or includes
		- Modifiers do not currently support arguments
	- Per-expand-data and custom emitters are not currently supported
	- Custom delimiters are supported (via {{= =}}), but they cannot be more than 8 characters long
	- Repeatedly generating output from the same template is slower than it needs to be, because we do not parse the source template into an internal data structure before processing
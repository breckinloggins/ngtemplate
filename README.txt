ngtemplate - A template engine written in C designed to be (mostly) syntax-compatible with Google CTemplate

Version 0.1-ish, so there's plenty more work to do. 

Introduction
------------

ngtemplate is a small, fast template engine written in C.  Like Google's CTemplate, it
strives to be as simple as possible to shield developers from the temptation of embedding
logic inside their templates.  An example of an ngtemplate would be:

Hello, {{Name}}.  You have read {{NumPosts}} posts on our blog today.  Thank you for visiting!

When combined with the following dictionary:

Name=John
NumPosts=7

The engine will emit the following string as output:

Hello, John.  You have read 7 posts on our blog today.  Thank you for visiting!

Installing
----------

To use ngtemplate:
	1) Install CMake if you don't have it
	2) Clone the git repository
	3) cd ngtemplate
	4) mkdir build
	5) cd build
	6) cmake ../src
	7) make
	8) make test
	
If all went well you should have a libngtemplate.a in your build directory

NOTE: CMake-less compilation and binary distributions will come eventually

Differences from CTemplate
--------------------------

	- Unlike in CTemplate, you can have as many FOO_separator sections as you like inside a template and they will all be expanded in the order in which the separators are placed
	- Dictionaries have block scope, meaning that names in the parent dictionary are available in inner dictionaries.  In particular, this means sections can access markers from their parent sections or the root template
	- ngtemplate is a little more dynamic than CTemplate.  You are allowed to set variable_missing and 
		modifier_missing callbacks to populate templates dynamically rather than stuffing variable
		values in a data dictionary ahead of time (or some combination of the two approaches).  This
		allows neat tricks similar to the magic you can pull off with Ruby's method_missing

Known Issues and Limitations for Version 0.1
--------------------------------------------

	- You cannot currently specify search directories for templates.  Calls to template_set_filename() need to have absolute or resolvable relative paths to the template file
	- Pragmas (including AUTOESCAPE) are not currently supported
	- Modifier support is not yet as capable as CTemplate:
		- Modifiers are supported, but none are currently built-in
		- Modifiers are supported on values only, not sections or includes
		- Modifiers do not currently support arguments
	- Per-expand-data and custom emitters are not currently supported
	- Custom delimiters are supported (via {{= =}}), but they cannot be more than 8 characters long
	- Repeatedly generating output from the same template is slower than it needs to be, because we do not parse the source template into an internal data structure before processing

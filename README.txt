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

ngtemplate is designed to follow Google's CTemplate syntax and design (to a 
point), so you can get a good feel for the capabilities of the library by 
visiting:

    http://google-ctemplate.googlecode.com/svn/trunk/doc/howto.html

Note that not all features have been implemented yet.    

Anatomy of a Template
---------------------

A template is a string with literal text and markers.  All literal text is output verbatim
by the template engine and is not modified in any way.  Markers are evaluated and replaced
depending on their type and the contents of the template dictionary.

Variable Markers
----------------

Markers that look like {{This}} are variable markers.  When ngtemplate encounters a variable
marker, it will look up the marker in the data dictionary.  If it finds a match, the marker
will be replaced by the text in the dictionary.  If the marker cannot be found in the dictionary, 
the marker will be erased from the output.

Section Markers
---------------

A Section looks like:

{{#Person}}
	Name: {{Name}}
	Age: {{Age}}
	Favorite Color: {{Color}}
	
{{/Person}}

Sections correspond to nested template dictionaries inside the parent template dictionary.  A 
section is expanded as many times as there are entires.  So, given:

Persion={
	Name=John
	Age=32
	Color=Blue
} {
	Name=Sarah
	Age=19
	Color=Purple
}

You would expect the template above to expand to:

	Name: John
	Age: 32
	Color: Blue
	
	Name: Sarah
	Age: 19
	Color: Purple 

If there is no entry in the template dictionary for a section, that section is ignored and the markers
erased from the template output.

Section Separators
------------------

Take the following template and template dictionary:

My favorite colors are {{#Colors}}{{Color}}{{/Colors}}

Colors={
	Color=Red
} {
	Color=Yellow
} {
	Color=Blue
} {
	Color=Black
} {
	Color=Gray
}

The output of the template would be:

RedYellowBlueBlackGray

But that's not what you want!  You'd like a comma and a space after all but the last color.
Fortunately, ngtemplate understands separators:

{{#Colors}}{{Color}}{{#Colors_separator}}, {{/Colors_separator}}{{/Colors}}

Will output:

Red, Yellow, Blue, Black, Gray

Just like you want.

[Work in progress]

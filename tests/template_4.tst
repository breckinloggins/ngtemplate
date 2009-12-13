{{!#
ClassName=Dog
Section1={
	Foo=Bar
}
#!}}
{{=@ @=}}	@! Change delimiter since that's not a valid character in C @

typedef struct @ClassName@_tag	{
	int TYPE;
} @ClassName@;

@={{ }}=@	{{! Changing delimiter back }}

For the generated C code above, ClassName was {{ClassName}}

{{=#=}}		#! Trying it with just one character, no spaces #

Now we should see #ClassName#.

#=///\\\ \\\0///=#	///\\\! Funky Delimiters \\\0///

///\\\ ClassName \\\0///

///\\\={{ }}=\\\0///

And, back to normal: {{ClassName}}

Here's a trick, we should see {{ClassName}} here, but now here comes a section:

{{#Section1}}
	{{=@=}}		@! This should work like above @
	
	I can get to @Foo@

@! Note the weirdness here, that we end the section with different delimiters than we started with @
@/Section1@

But, now that I'm out of the section, I have to use the old markers, like this: {{ClassName}}




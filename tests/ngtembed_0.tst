{{! Started out as a copy of template_0 test input }}
{{!#
# The above is a special sequence recognized by the test engine to specify a dictionary

# Template Test #1
# Test The data dictionary and all the escapes, comments, etc.
Foo=Bar
One=Two
BigText=Oh man is this a bunch of big text that keeps going and going and going and going and going on an on and on and on ad infinitum, well probably not "ad infinitum", but close.  Say, what happens if we do \{\{ THIS \}\} ?  Probably nothing, but we should check anyway huh?

# This is a comment, it should be ignored
Escapes=This should work: \= \{ \} \!			# This is a comment, it should also be ignored

# End of dictionary
#!}}
// This is a test of a template

This should be copied verbatim.  But Foo = {{ Foo }}

Also, One = {{ One }}, but that's kind weird, huh?

What happens if we insert a {{ key }}that doesn't exist?

Even better, what happens we insert a bunch of crap like "{{BigText}}".  Hopefully that works!

This formatted string: {{FmtString}} comes directly from the test code using the 
template_string_stringf() function.

This should be an integer: {{IntValue}}
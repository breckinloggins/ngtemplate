# Template Test #1
# Test The data dictionary and all the escapes, comments, etc.
Foo=Bar
One=Two
BigText=Oh man is this a bunch of big text that keeps going and going and going and going and going on an on and on and on ad infinitum, well probably not "ad infinitum", but close.  Say, what happens if we do \{\{ THIS \}\} ?  Probably nothing, but we should check anyway huh?

# This is a comment, it should be ignored
Escapes=This should work: \= \{ \} \!			# This is a comment, it should also be ignored

!

// This is a test of a template

This should be copied verbatim.  But Foo = {{ Foo }}

Also, One = {{ One }}, but that's kind weird, huh?

What happens if we insert a {{ key }} that doesn't exist?

Even better, what happens we insert a bunch of crap like "{{BigText}}".  Hopefully that works!




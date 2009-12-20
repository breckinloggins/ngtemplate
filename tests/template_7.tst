{{!#
One=1
Two=2
Three=3
Dummy=Dummy
#!}}
{{One}},{{Two}},{{Three:modifier}}
{{Dummy:modifier_with_args=1,2,3,4,5}}
{{Dummy:modifier_with_args:cstring_escape}}
{{Dummy:modifier_with_args=FooBar:cstring_escape}}
{{Dummy:x-modifier_no_args}}
{{Dummy:x-modifier}}
{{Dummy:x-modifier=5,4,3,2,1}}
{{Dummy:x-modifier:a,b,c,d }}
{{Dummy:x-modifier:a,b=something}}
{{Dummy:x-modifier:A,B,C,D:cstring_escape}}
Just seeing if we still work after modification! {{One}}

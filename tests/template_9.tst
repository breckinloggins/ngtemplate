{{! Tests the ngtemplate standard environment }}
{{!#
NoneModifier=This sentence should come out just like this
CStringModifier=This should have	escapes for various things like "quotes" and tabs
#!}}
NoneModifier={{NoneModifier:none}}
CStringModifier={{CStringModifier:cstring_escape}}

There should be spaces Between{{BI_SPACE}}These{{BI_SPACE}}Words, and a newline{{BI_NEWLINE}}here

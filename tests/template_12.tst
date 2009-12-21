{{! Test all the escapes }}
{{!#
CString=This is an example of a "CString" and should be 'escaped'
HTML=There are "many" things in HTML that should be escaped, like < & >
#!}}
{{CString:cstring_escape}}
{{HTML:html_escape}}
{{HTML:h}}

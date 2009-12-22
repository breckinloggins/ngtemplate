{{! Test all the escapes }}
{{!#
CString=This is an example of a "CString" and should be 'escaped'
HTML=There are "many" things in HTML that should be escaped, like < & >
URL=http://www.somewhere.com/~somebody/this will need to be escaped!
CSS=This_Is_(NOT)_A/Safe"Value"?!
#!}}
{{CString:cstring_escape}}
{{HTML:html_escape}}
{{HTML:h}}
{{HTML:pre_escape}}
{{HTML:p}}
{{URL:url_query_escape}}
{{URL:u}}
{{CSS:css_cleanse}}
{{CSS:c}}

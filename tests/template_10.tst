<%! Need to switch back to old delimiters temporarily because read_dictionary isn't delimiter-aware %>
<%! None of this stuff should show up in the output if ngt_set_delimiters() is working %>
<%={{ }}=%>
{{!# 
DelimiterTest=True
#!}}
{{! OK now back to the originals }}
{{=<% %>=}}
This should say "True": <% DelimiterTest %>

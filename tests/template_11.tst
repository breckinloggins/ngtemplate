{{!# 
VisibleSection={
	Foo="Foo"
	Bar="Bar"
}

HiddenSection={
	One="One"
	Two="Two"
}{
	One=1
	Two=2
}
#!}}

{{#VisibleSection}}Foo = {{Foo}}, Bar = {{Bar}}{{/VisibleSection}}
{{#HiddenSection}}We shouldn't see this: One={{One}}, Two={{Two}}{{#HiddenSection_separator}}{{BI_NEWLINE}}{{/HiddenSection_separator}}{{/HiddenSection}}

And some more stuff down here.
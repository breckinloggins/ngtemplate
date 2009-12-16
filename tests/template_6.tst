{{!#
Foo=Bar
Callback_Template={
	Key=This is a key
	One=One
}
Filename_Template={
	Name=John
	Age=21
	Quote="Yay, I can finally drink beer\!"
}
Person_Favorites={
	Color=Black
	Band=Pearl Jam
	Sport=Football
}
Extra=If you can't see this there is a bug
#!}}
This template tests include sections

{{! An Include template that doesn't exist.  It should expand to nothing}}
Nothing should be between these arrows --->{{>SOME_TEMPLATE}}<---

{{! A Template that is defined in the code and retrieved via a callback function }}
{{>Callback_Template}}

Just to make sure: Foo = {{Foo}} <--- should be "Foo = Bar"

{{! A Template that needs to be loaded from some filename set in code }}
A person:
{{>Filename_Template}}

Again, but should be indented:
	{{>Filename_Template}}

{{! TODO: Test includes inside sections }}

Just to make sure again: Foo = {{Foo}} <--- should be "Foo = Bar"
Done.

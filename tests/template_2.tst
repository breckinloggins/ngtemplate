One=1
Two=2
Section1={
	One="One"
	Two="Two"
	KeyThree=This is KeyThree
}
Variable=decl
MultiSection={
	One=1.0
	Two=2.0
	KeyThree=3.0
	AdditionalThing=That other thing
}{
	One="one dot oh"
	Two="two dot oh"
	AdditionalThing=That other thing again
	Last=Bar
}
KeyThree=LastThree

!
This is a test of sections.
{{One}}, {{Two}} 

{{#Section1}}
	{{! Should expand once }}
	Welcome to my section!
{{/Section1}}

Some stuff outside of the section.  Here's a variable name: {{Variable}}

{{#MultiSection}}
	Here is a section that will be repeated multiple times
	{{One}}
	{{Two}}
	{{KeyThree}}
	{{AdditionalThing}}
{{/MultiSection}}

Now let's test scoping:

{{#Section1}}
	We should be able to see variable: {{ Variable }} because it's in the parent
	template.  Also:
	
	{{One}}
	{{Two}}
	{{KeyThree}} <-- Should be "This is KeyThree", not "LastThree"
	
	But we should not be able to see {{AdditionalThing}}, because it's in a child section
{{/Section1}}

Lastly, {{KeyThree}} should now be "LastThree"

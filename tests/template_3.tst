{{! More section testing.  This time testing nested sections and empty sections !}}
{{!#
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
#!}}
This is a test of sections.
{{One}}, {{Two}}, 

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
	{{#Section1}}
		One = {{One}},
		Two = {{Two}}
	{{/Section1}}
{{/MultiSection}}

Now let's test scoping:

{{#Section1}}
	We should be able to see variable: {{ Variable }} because it's in the parent
	template.  Also:
	
	{{One}}
	{{Two}}
	{{KeyThree}} <-- Should be "3.0" in the first section, but "LastThree" in the second 
	
	But we should not be able to see {{AdditionalThing}}, because it's in a child section
{{/Section1}}

How about an empty section:

{{#EMPTY_SECTION}}
{{/EMPTY_SECTION}}

Or a non-empty section with no corresponding entry in the dictionary
{{#Unref_Section}}
	This stuff should be completely skipped over
{{/Unref_Section}}

{{#Unref_Section}}
	This stuff should be completely skipped over
	Even {{One}}, {{Two}}
	
	And certainly nested sections:
	{{#Section1}}
		{{One}} {{Two}}
	{{/Section1}}
	
	And invalid nested sections:
	{{#Unref_Section}}
	{{/Unref_Section}}
	
	And invalid non-empty nested sections"
	{{#EMPTY_SECTION}}
		This is not empty, {{One}}
	{{/EMPTY_SECTION}}
{{/Unref_Section}}


Lastly, {{KeyThree}} should now be "LastThree"

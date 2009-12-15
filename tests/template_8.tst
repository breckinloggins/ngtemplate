{{! Tests to see if the dynamic variable_missing and modifier_missing callbacks work }}
This should work:
	{{DynOne}}      <-- "One"
	{{DynTwo}}      <-- "Two"
	{{DynThree}}    <-- "Three"
	
	{{DynOne:DynModifier}}      <-- "One + C"
	{{DynTwo:DynModifier}}      <-- "Two + C"
	{{DynThree:DynModifier}}    <-- "Three + C"

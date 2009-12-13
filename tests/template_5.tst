{{!#
People={
	Person=Bob
}{
	Person=Mary
}{
	Person=Sue
}{
	Person=Terry
}
Animals={
	Animal=Dog
	Number=1
}{
	Animal=Cat
	Number=2
}{
	Animal=Bird
	Number=3
}{
	Animal=Lion
	Number=4
}{
	Animal=Tiger
	Number=5
}
MuscleCars={
	Make=Ford
	Model=Mustang
}{
	Make=Chevy
	Model=Camaro
}{
	Make=Dodge
	Model=Charger
}
#!}}
People: {{#People}}{{Person}}{{#People_separator}}, {{/People_separator}}{{/People}}

Animals: {{#Animals}}({{Number}}) {{Animal}}{{#Animals_separator}} {{Number}}=> {{/Animals_separator}}{{/Animals}}

{{!Note, this tests using more than one separator, which we support but CTemplate does NOT }}
Muscle Cars: {{#MuscleCars}}{{#MuscleCars_separator}}/{{/MuscleCars_separator}}{{Make}} {{Model}}{{#MuscleCars_separator}}/{{/MuscleCars_separator}}{{/MuscleCars}}

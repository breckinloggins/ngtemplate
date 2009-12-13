CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(cxtemplate)

SUBDIRS(../lib/libuseful/src)

SET(cxtemplate_LIB_SRCS
	template.c
)

ADD_LIBRARY(cxtemplate STATIC ${cxtemplate_LIB_SRCS})
TARGET_LINK_LIBRARIES(cxtemplate useful)

ENABLE_TESTING()

ADD_EXECUTABLE(template_test testing/template_test.c)
TARGET_LINK_LIBRARIES(template_test useful cxtemplate)

MACRO(ADD_TEMPLATE_TEST NUMBER)
	ADD_TEST(
		template_${NUMBER} 
		${EXECUTABLE_OUTPUT_PATH}/template_test
		../tests/template_${NUMBER}.tst
		../tests/template_${NUMBER}.bmk
	)
ENDMACRO(ADD_TEMPLATE_TEST)

MACRO(ADD_TEMPLATE_TEST_1 NUMBER EXTRA_ARG)
	ADD_TEST(
		template_${NUMBER} 
		${EXECUTABLE_OUTPUT_PATH}/template_test
		../tests/template_${NUMBER}.tst
		../tests/template_${NUMBER}.bmk
		${EXTRA_ARG}
	)
ENDMACRO(ADD_TEMPLATE_TEST_1)

ADD_TEMPLATE_TEST(0)
ADD_TEMPLATE_TEST(1)
ADD_TEMPLATE_TEST(2)
ADD_TEMPLATE_TEST(3)
ADD_TEMPLATE_TEST(4)
ADD_TEMPLATE_TEST(5)
ADD_TEMPLATE_TEST_1(6 ../tests/template_6.subtemplate)
ADD_TEMPLATE_TEST(7)




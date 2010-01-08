CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ngtemplate)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(NOT LIBUSEFUL_DIR)
	SET(LIBUSEFUL_DIR ../lib/libuseful/src)
	ADD_SUBDIRECTORY(${LIBUSEFUL_DIR} "${CMAKE_CURRENT_BINARY_DIR}/libuseful")
ENDIF(NOT LIBUSEFUL_DIR)

INCLUDE_DIRECTORIES(${LIBUSEFUL_INCLUDES})

FIND_PATH(NGTEMPLATE_INCLUDES 
			ngtemplate.h
			${CMAKE_CURRENT_SOURCE_DIR}/include
)

INCLUDE_DIRECTORIES(${NGTEMPLATE_INCLUDES})


SET(ngtemplate_LIB_SRCS
	internal.h
	internal.c
	stdenv.c
	ngtemplate.c
	include/ngtemplate.h
)

ADD_LIBRARY(ngtemplate STATIC ${ngtemplate_LIB_SRCS})
TARGET_LINK_LIBRARIES(ngtemplate useful)

ADD_EXECUTABLE(ngtembed ngtembed_tool.c ngtembed.c)
TARGET_LINK_LIBRARIES(ngtembed useful ngtemplate)

ENABLE_TESTING()

ADD_EXECUTABLE(template_test testing/template_test.c)
TARGET_LINK_LIBRARIES(template_test useful ngtemplate)

ADD_EXECUTABLE(ngtembed_test testing/ngtembed_test.c ngtembed.c)
TARGET_LINK_LIBRARIES(ngtembed_test useful ngtemplate)

SET(NGT_TESTDIR ${CMAKE_CURRENT_SOURCE_DIR}/../tests)

MACRO(ADD_TEMPLATE_TEST NUMBER)
	ADD_TEST(
		template_${NUMBER} 
		${EXECUTABLE_OUTPUT_PATH}/template_test
		${NGT_TESTDIR}/template_${NUMBER}.tst
		${NGT_TESTDIR}/template_${NUMBER}.bmk
	)
ENDMACRO(ADD_TEMPLATE_TEST)

MACRO(ADD_TEMPLATE_TEST_1 NUMBER EXTRA_ARG)
	ADD_TEST(
		template_${NUMBER} 
		${EXECUTABLE_OUTPUT_PATH}/template_test
		${NGT_TESTDIR}/template_${NUMBER}.tst
		${NGT_TESTDIR}/template_${NUMBER}.bmk
		${EXTRA_ARG}
	)
ENDMACRO(ADD_TEMPLATE_TEST_1)

ADD_TEMPLATE_TEST(0)
ADD_TEMPLATE_TEST(1)
ADD_TEMPLATE_TEST(2)
ADD_TEMPLATE_TEST(3)
ADD_TEMPLATE_TEST(4)
ADD_TEMPLATE_TEST(5)
ADD_TEMPLATE_TEST_1(6 ${NGT_TESTDIR}/template_6.subtemplate)
ADD_TEMPLATE_TEST(7)
ADD_TEMPLATE_TEST(8)
ADD_TEMPLATE_TEST(9)
ADD_TEMPLATE_TEST(10)
ADD_TEMPLATE_TEST(11)
ADD_TEMPLATE_TEST(12)
ADD_TEST(ngtembed ${EXECUTABLE_OUTPUT_PATH}/ngtembed_test ${NGT_TESTDIR}/ngtembed_0.tst=test0 ${NGT_TESTDIR}/ngtembed_1.tst=test1 ${NGT_TESTDIR}/ngtembed_2.tst=test2 ${NGT_TESTDIR}/ngtembed.bmk)

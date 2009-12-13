CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libuseful)

FIND_PROGRAM(DiffFound diff)
IF(DiffFound STREQUAL "DiffFound-NOTFOUND")
	MESSAGE(FATAL_ERROR "The diff tool is not found on your system.  This is required for testing!")
ENDIF(DiffFound STREQUAL "DiffFound-NOTFOUND")

SET(useful_LIB_SRCS
	hashtable.c
	list.c
	stringbuilder.c
)

ADD_LIBRARY(useful ${useful_LIB_SRCS})

ENABLE_TESTING()

ADD_EXECUTABLE(stringbuilder_test stringbuilder.c testing/stringbuilder_test.c)
ADD_TEST(stringbuilder_0 ${EXECUTABLE_OUTPUT_PATH}/stringbuilder_test)

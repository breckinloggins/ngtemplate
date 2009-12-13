#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef int (*test_fn)(FILE* in, FILE* out, int argc, char** argv);

#define DEFINE_TEST_FUNCTION int test_function(FILE* in, FILE* out, int argc, char** argv)
#define RUN_TEST return test_handler(test_function, argc, argv)

int test_handler(test_fn fn, int argc, char** argv)	{
	char cmd[512];
	char tempname[255];
	FILE* in;
	FILE* out;
	int res;
	
	if (argc > 4)	{
		fprintf(stderr, "USAGE: <testname>_test <input file> <benchmark file> <optional arg>\n");
		fprintf(stderr, "\t<testname>_test <input_file>\n");
		return 1;
	}
	
	if (argc > 1)	{
		in = fopen(argv[1], "r");

		if (argc > 2)	{
			memset(tempname, 0, 255);
			tmpnam(tempname);
			out = fopen(tempname, "w");
		} else {
			out = stdout;
		}
	} else {
		in = stdin;
		out = stdout;
	}
	
	res = fn(in, out, argc, argv);
	
	if (argc > 1)	{
		fclose(in);

		if (res == 0)	{
		// We didn't receive an error from the actual test handler, so let's test the 
		// output against the benchmark
			if (argc > 2)	{
				fclose(out);

				memset(cmd, 0, 512);
				sprintf(cmd, "diff %s %s", argv[2], tempname);
				res = system(cmd);
				res = (res != 0);
				remove(tempname);
			} else {
				res = 0;
			}
		}
	}
	
	return res;
}

int get_file_length(const char* filename)	{
	int length;
	FILE* fd;
	
	fd = fopen(filename, "rb");
	if (!fd)	{
		return 0;
	}
	
	fseek(fd, 0, SEEK_END);
	length = ftell(fd);
	close(fd);
	
	return length;
}

#endif // TEST_UTILS_H
#include <iostream>
#include <cstdio>

void logger(double runtime) {
	FILE *fp = fopen("log.txt", "w");
	if (fp == NULL) {
		fputs("File error\n", stderr);
		exit(1);
	}
	fprintf(fp, "%f\n", runtime);
	fclose(fp);
}

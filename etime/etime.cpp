#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <windows.h>

int main(int argc,char **argv) {
	const char *cl=GetCommandLineA();

	while(*cl && !isspace(*cl))
		cl++;
	while(*cl && isspace(*cl))
		cl++;

	clock_t start=clock();

	int ec=system(cl);

	clock_t end=clock();

	fprintf(stderr,"time taken for [[%s]] is %f with errcode %d\n",
		cl,
		(end-start)/(double)CLOCKS_PER_SEC,
		ec
		);
	return ec;
}

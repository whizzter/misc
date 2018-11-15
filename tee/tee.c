#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>

#define BZ 1
int main(int argc,char **argv) {
	char buf[BZ+1];
	int fin=fileno(stdin);
	int fout=fileno(stdout);
	int fil;
	int r;
	if (argc<2) {
		fprintf(stderr,"Error, no output file given\n");
		return -1;
	}
	{
		char *fn=argv[1];
		int mod;
		if (fn[0]=='+') {
			fn++;
			mod=_O_APPEND|_O_BINARY|_O_CREAT|_O_WRONLY;
		} else if (fn[0]=='!') {
			fn++;
			mod=_O_APPEND|_O_BINARY|_O_CREAT|_O_TRUNC|_O_WRONLY;
		} else {
			fprintf(stderr,"Error, no file mode was given, prefix the name by + for append or ! for overwrite\n");
			return -1;
		}
		fil=open(fn,mod,_S_IREAD|_S_IWRITE);
		if (fil==-1) {
			fprintf(stderr,"Error, could not open %s\n",fn);
			return -1;
		}
	}
	setmode(fin,_O_BINARY);
	setmode(fout,_O_BINARY);

	while(0<(r=read(fin,buf,BZ))) {
		int fi;
		int max=r;
		//fprintf(stderr,"Read:%d\n",r);
		//Sleep(1);
		for (fi=0;fi<2;fi++) {
			int p=0;
			int fh=fi==0?fil:fout;
			while(p<max) {
				if (0>=(r=write(fh,buf+p,max-p))) {
					//fprintf(stderr,"Error on write\n");
					goto err;
				}
				p+=r;
			}
//			if (fi==0)
//				commit(fil);
		}
	}
	//fprintf(stderr,"Err-Read:%d\n",r);
	err:

	
	close(fil);

	return 0;
}

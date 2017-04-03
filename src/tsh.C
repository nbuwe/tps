#include <stdio.h>
#include <fcntl.h>
#include "tps.H"

#if VERBOSE
#include "debug.H"
#endif

#if !defined(__osf__) && !defined(solaris2)
#  ifndef __GNUC__
EXTERNC int open(char*,int);
#  endif
#endif

main(int argc, char** argv)
{
    Tps_Status status;
    Tps_Interp* pi;
    int execprint = 5;

#if DEBUGMALLOC
    (void)malloc_debug(2);
#endif
    status = Tps_initialize(TRUE);
    if(status != TPSSTAT_OK) goto done;

    pi = Tps_interp_create();
    if(!pi) goto done;

    if(argc == 3 && strcmp(argv[1],"-restore")==0) {
	int fd = ::open(argv[2],O_RDONLY);
	if(!fd) {
	    fprintf(stderr,"%s: cannot read %s\n",argv[2]);
	    return (1);
	}
	if(pi->restore(fd) != TPSSTAT_OK) {
	    fprintf(stderr,"state restore failed\n");
	    return (1);
	}
	close(fd);
    } else {
	/* begin by executing the procedure "start" */
	status = pi->load("start");
	if(status != TPSSTAT_OK) goto done;
    }

#if VERBOSE > 1
    printf("dict:\n%s\n\n",debugdstacks(pi));
#endif

#if VERBOSE
    printf("initial exec:\n%s\n",debugexec(pi));
    printf("initial stack: %s\n",debugstack(pi));
    printf("initial dict stack: %s\n",debugdstacks(pi));
    pi->step(TRUE);
    for(;;) {
	printf("stack before: %s\n",debugstack(pi));
	printf("dictstack before: %s\n",debugdstacks(pi));
	printf("exec before:\n%s\n",debugexec0(pi,execprint));
#endif
	switch (status = pi->run()) {
	    case TPSSTAT_OK:
		break;
	    case TPSSTAT_INTERRUPT:
		if(argc == 3 && strcmp(argv[1],"-save")==0) {
		    FILE* f = fopen(argv[2],"w");
		    char* s;
		    if(!f) {
			fprintf(stderr,"cannot write state file %s\n",argv[2]);
			return (1);
		    }
		    if(pi->save(s) != TPSSTAT_OK) {
			fprintf(stderr,"state save failed\n");
			return (1);
		    }
		    fputs(s,f);
		    fclose(f);
		} else {
		    printf("interpreter interrupt occurred\n");
		}
		break;
	    default:
		goto done;
	}
#if VERBOSE
	printf("step%s%s: %s\n",pi->tracing()?"+":"",pi->safe()?"*":"",debugobject(pi->_object));
	printf("stack after: %s\n",debugstack(pi));
	printf("dictstack after: %s\n",debugdstacks(pi));
	printf("exec after:\n%s\n",debugexec0(pi,execprint));
    }
#endif
done:
    if(status != TPSSTAT_OK) {
	printf("return status: %s\n",TPS_ENM((int)status));
	if(status == TPSSTAT_QUIT) status = TPSSTAT_OK;
    }
    return ( status );
}

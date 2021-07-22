#define NTRBUF 10000 /* tamano del buffer de inst. shade */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <varargs.h>
#include <shade.h>
#include <shade_sparcv9.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "cabecera.h"

extern void fincpu(), inicpu(), inichivato(), sim();	
extern void traduce();

#define caso(a, b) case ('a' << 8) + 'b'

struct shade_trace_s {SHADE_SPARCV9_TRACE};

char fsalida[20];
FILE *fpout;

char copia_aargv0[100];

void initialize(), terminate();

int
shade_main(int argc, char *argv[], char **envp)
{
    int	aargc, pid = getpid();
    char	**aargv;
    shade_iset_t *piset;
    shade_tset_t *ptset;

    shade_trsize(sizeof(shade_trace_t));

    piset = shade_iset_newclass(SHADE_ICLASS_ANY, -1);
    ptset = shade_tset_new(SHADE_SPARCV9_TRCTL_I, SHADE_TRCTL_ANNULLED, SHADE_TRCTL_PC, SHADE_TRCTL_EA, SHADE_TRCTL_TAKEN, -1);
    shade_trctl(piset, SHADE_TRI_ISSUED, ptset);

    argc = shade_splitargs (argv, &aargv, &aargc);

    strcpy(copia_aargv0, *aargv);
    initialize (argc, argv);
    shade_loadp (*aargv, aargv, envp);

    sim();
	
    if (pid == getpid())
	terminate();
}


char
get_instr(IREG *ir)
{
    static unsigned long i=0,ntr=0;	
    static shade_trace_t tr[NTRBUF];		

    if (i == ntr)
    {
	ntr = shade_run(tr,NTRBUF);
	if (ntr == 0) return 0;
	i = 0;
    }
    traduce(&tr[i],ir);
    i++;
    return 1;
}

void
initialize(int parc 	/* command-line parameters to */,
	   char *parv[]	/*  analysis program */)
{
    int i;

    fsalida[0] = '\0';

    for (i = 1; i <= parc; i++)
    {
	if (parv[i] != NULL)
	{
	    switch(parv[i][0])
	    {
		case 'f':
		    strcpy(fsalida,&parv[i][1]);
		    break;
		default:
		    ;
	    }
	}
    }

    inichivato();
    inicpu();
}


void
terminate()
{
    char maquina[30];
    struct rusage *te;
    long espaciote[25];
    long umin, usec, smin, ssec;

    if (fsalida[0] != '\0')
	fpout = fopen(fsalida,"w");
    else
	fpout = fopen("/dev/tty","w");

    fincpu();

    gethostname(maquina, sizeof(maquina));
    maquina[sizeof(maquina)]='\0';
    fprintf(fpout,"Ejecutado por %s ",maquina);

    te = (struct rusage *)espaciote;
    if (getrusage(RUSAGE_SELF,te) == -1)
	fprintf(fpout,"ERROR en getrusage\n");
    else
    {
	umin = te->ru_utime.tv_sec /60;
	usec = te->ru_utime.tv_sec %60;
	smin = te->ru_stime.tv_sec /60;
	ssec = te->ru_stime.tv_sec %60;
	fprintf(fpout, "en %d' %d'' de usr y %d' %d'' de sys\n\n",
		umin, usec, smin, ssec);
    }
    fflush(fpout);
}

void suspend()
{
    terminate();
}

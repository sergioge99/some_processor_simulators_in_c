#define NOSTEP

/* para eliminar chivato definir NOSTEP */
/* para activar chivato definir STEP */
/* no pueden estar definidos los dos al mismo tiempo */

#include "cabecera.h"

extern IREG etapa_Bin, etapa_Din;

extern unsigned long instrucciones;

typedef struct {
	unsigned long pc, iw;
	long inm;
	char rd, rs1, rs2, *co, ops;
} INST_DEC;

/*extern void decodifica_ins(), imprime_ins();*/
extern void escribirRob();

FILE *fpch, *fpchin;

void verir(), printir();

char *textco[8] = { "NOP  ", "LOAD ", "STORE", "ARITM", "BRCON", "BRINC", "FLOAT", "OTROS"};


void chivato()
{
	char dosreg = ' ';
	char linea[40];
	static unsigned long int salta = 0, saltai;
	static char termina = 0, busca = 0, quebusco[20];
	int i;
	unsigned long iw, ea;
	long rs1, rs2, Inm;
	INST_DEC instruccion;

#ifdef NOSTEP
	return;
#endif

	if (termina) return;
	if (salta) {salta--; return; }
	if (saltai > instrucciones) return;
	if (busca) {
		if (strcmp(textco[etapa_Bin.co], quebusco) != 0) return;
	}
	busca = 0;

	if (etapa_Din.co == STORE && etapa_Din.nreg == 2) dosreg = '2';

	fprintf(fpch, "ciclo %lu \tinstruccion %lu\n", tiempo, instrucciones);
	printir(&etapa_Bin, fpch);
	printir(&etapa_Din, fpch);
	fprintf(fpch,"\n");

	escribirRob(tiempo);

/******************************************/

	while(1)
	{
		if (fgets(linea, 40, fpchin) == NULL) return;
		switch(linea[0]) {
			case '\0': return;
			case 't': termina = 1; return;
			case 'q': exit(0);
			case 's': salta  = atol(&linea[1]); return;
			case 'g': saltai = atol(&linea[1]); return;
			case 'i': verir(atoi(&linea[1]), fpch); break;
			case 'b':
				strcpy(quebusco, &linea[1]); /* busca codigos de operacion */
				quebusco[strlen(quebusco)-1] = '\0';
				busca = 1;
				return;
			default: return;
     	}
	}
}


void inichivato()
{
#ifdef STEP
	fpch = fopen("/dev/tty", "w");
	fpchin = fopen("/dev/tty", "r");
#endif

}


void verir(int ir,FILE *fp)
{
  IREG *p;

  switch(ir)
  {
     case 1: p = &etapa_Bin; break;
     default: p = &etapa_Din; break;
  }
  fprintf(fp,"%s r%d, r%d, r%d -- %x(%lu) %x(%lu)\n",
	textco[p->co], p->rs1, p->rs2, p->rd, p->pc, p->pc, p->ea, p->ea);
}

void printir(IREG *ir,FILE *fp)
{
  fprintf(fp, "%5s", textco[ir->co]);
  switch(ir->co)
  {
     case ARITM:
     case FLOAT:
			if (ir->rs1 >= 0)
		    fprintf(fp, " %2d,", ir->rs1);
		else
		    fprintf(fp," --,");
	
		if (ir->rs2 >= 0)
		    fprintf(fp, "%2d,", ir->rs2);
		else
		    fprintf(fp, "--,");

		if (ir->rd >= 0)
		    fprintf(fp, "%2d  ", ir->rd);
		else
		    fprintf(fp,"--  ");
	break;

     case LOAD:
		fprintf(fp, "(%2d,", ir->rs1);
		if (ir->rs2 >= 0)
		    fprintf(fp, "%2d) ", ir->rs2);
		else
		    fprintf(fp, "--) ");
		fprintf(fp, "%2d ", ir->rd);
	break;

     case STORE:
		fprintf(fp, "%2d ", ir->rd);
		fprintf(fp, "(%2d,", ir->rs1);
		if (ir->rs2 >= 0)
		    fprintf(fp,"%2d) ", ir->rs2);
		else fprintf(fp,"--) ");
	break;

     /* case BRINC: 
	case BRCON:
		if (ir->rd >= 0) fprintf(fp, " %2d,", ir->rd);
		else fprintf(fp, " --,");
		if (ir->rs1 >= 0) fprintf(fp, "%2d," ,ir->rs1);
		else fprintf(fp,"--,");
		if (ir->rs2 >= 0) fprintf(fp, "%2d  ", ir->rs2);
		else fprintf(fp, "--  ");
	break;
     */
     case BRINC: 
     case BRCON: 
     default:
	fprintf(fp,"           ");
	break;
  }
  fprintf(fp,"     ");
}

#include "cabecera.h"
#include "registros.h"

#define MAX_ROB 10

extern int latenciasWR[8];
extern void escribirInstruccion();

typedef struct ROBentry_s {
	IREG instr; /* solo necesario para chivato */
	char estado;
	unsigned long terminada;
	char rd, old_rd;
} ROBentry_t;

ROBentry_t ROB[MAX_ROB];

int ROBocu = 0, ROBin = 0, ROBout = 0;

static IREG jubilada;

/**********************************************************/
int ROBhay()
{
	/* TODO: sustituir MAX_ROB por parámetro tamaño ROB */
	if (ROBocu == MAX_ROB) return 0;
	return 1;
}   

/**********************************************************/
int ROBvacio()
{
	if (ROBocu == 0) return 1;
	return 0;
}   

/**********************************************************/
int ROBadd(IREG instr, int rd, int old)
{
	int i;
	if (ROBocu == MAX_ROB) {
		printf ("ERROR en funcion ROBadd *****************************\n");
		exit(1);
	}
	ROBocu++;
	i = ROBin;
	ROB[i].rd = rd;
	ROB[i].old_rd = old;
	ROB[i].terminada = 0;
	ROB[i].instr = instr;
	ROB[i].instr.ROBentry = i;
	ROB[i].estado = 0;
	ROBin++;
	if (ROBin == MAX_ROB) ROBin = 0;
	return i;
}

/**********************************************************/
void ROBejecuta(int entrada, unsigned long int ciclo)
{
	ROB[entrada].terminada = ciclo;
	ROB[entrada].estado = 1;
}

/**********************************************************/
int ROBjubila()
{
	jubilada.co = NOP;
	if ( (ROBocu == 0)
			|| (ROB[ROBout].terminada > tiempo)
			|| (ROB[ROBout].estado == 0) )
		return 0;
	jubilada = ROB[ROBout].instr;
	if ((ROB[ROBout].rd > 0) && (ROB[ROBout].old_rd > 0))
		libera_RF(ROB[ROBout].old_rd);
	ROBocu--;
	ROBout++;
	if (ROBout == MAX_ROB) ROBout = 0;
	return 1;
}


/**********************************************************/
unsigned long printROBfirst()
{
	if (ROBocu == 0) return 0;
	return ROB[ROBout].terminada;
}


/**********************************************************/
void escribirRob()
{
	ROBentry_t var;
	unsigned long int ciclo_issue;
	int i, cont;
	char hay_wb;

	printf("%s---------------------------------------%s\n", AMAR , RESET);
	printf("%sROB: %2d   %sNOlanzada %sENejecucion %sTerminada%s\n",
				AMARN, ROBocu, AMAR, RESET, ROJO, RESET);
	printf("%sPOS  Etapa  CO    r1  r2  rd%s\n", AMAR , RESET);

	// Muestra por pantalla cada campo del ROB.
	if (ROBin == 0) i = MAX_ROB - 1;
	else i = ROBin - 1;

	for (cont = 0; cont < ROBocu; cont++) {
		var = ROB[i];
		if (var.instr.rd > 0 && var.instr.co != STORE)  hay_wb = 1;
		else hay_wb = 0;

		if (var.estado == 0) 	printf("%s%2d:        ", AMAR, i);
		else {
			if (var.terminada >= tiempo) {
				ciclo_issue = var.terminada - latenciasWR[var.instr.co] - 1 - hay_wb;
				if (tiempo == ciclo_issue)
               		printf("%2d:     I  ", i); /* ciclo issue */
				else {
					if (tiempo == ciclo_issue + 1)
						printf("%2d:     R  ", i); /* ciclo register read */
					else {
						if ((var.terminada == tiempo) && hay_wb)
							printf("%2d:     W  ", i); /* ciclo write back */
						else
						    printf(  "%2d:    E%1lu  ", i, tiempo - ciclo_issue - 1);
					}
				}
			}
			else 	printf("%s%2d:        ", ROJO, i);
		}
																			
		escribirInstruccion(var.instr);
		printf("%s", RESET);

		if (i > 0) i--;
		else i = MAX_ROB - 1;
	}
	if (jubilada.co != NOP)	{
		printf("%s        C  ", ROJO); 
		escribirInstruccion(jubilada);
		printf("%s", RESET);
	}
}



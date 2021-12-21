#include "cabecera.h"

/* para eliminar chivato definir NOSTEP */
/* para activar chivato definir STEP */
/* no pueden estar definidos los dos al mismo tiempo */
#define NOSTEP

#define LINE_WIDTH 70

extern IREG etapa_Bin, etapa_Bout, etapa_Din;
extern char carga_B, carga_D;

extern void imprimirIssue();			// Imprime Issue.
extern void escribirRegistros();		// Imprime estado registros.
extern void escribirRob();				// Imprime el ROB.
extern char consultar();

typedef struct {
	unsigned long pc, iw;
	long inm;
	char rd, rs1, rs2, *co, ops;
} INST_DEC;

FILE *fpch, *fpchin;

char *textco[8] = { "NOP", "LOAD", "STORE", "ARITM", "BRCON", "BRINC", "FLOAT", "OTROS"};


void escribirInstruccion();

void chivato(IREG sacaISSUE, unsigned long instrucciones)
{
	unsigned long aux;
	int longitud = 0;
	char linea[40];
	static unsigned long int salta = 0, saltai;
	static char termina = 0, busca = 0, quebusco[20];


#ifdef NOSTEP
	return;
#endif

	if (termina) return;
	if (salta) { salta--; return; }
	if (saltai > instrucciones) return;
	if (busca) { if (strcmp(textco[etapa_Bin.co], quebusco) != 0) return; }
	busca = 0;

	/******************************************/
	/*    escribe informacion ciclo actual    */
	/******************************************/

	/* imprime el tiempo alineado a la derecha */	
	aux = tiempo;
	longitud = 6; /* "TIME: " -> 6 caracteres */
	while (aux > 0) { longitud++; aux = aux/10; }
	// \033[%dC: mueve el cursor a la dcha
	printf("\033[%dC", (LINE_WIDTH - 1) - longitud);
	printf("%sTIME: %lu%s", AMARN, tiempo, RESET);
	// \033[%dD: mueve el cursor a la izquierda
	printf("\033[%dD", LINE_WIDTH - 1);
	printf("%s        B   CO    r1  r2  rd \n%s", AMAR, AMAR, RESET);

	/* imprime el numero de instrucciones alineado a la derecha */	
	aux = instrucciones;
	longitud = 9; /* "FETCHED: " -> 6 caracteres */
	while (aux > 0) { longitud++; aux = aux/10; }
	printf("\033[%dC", (LINE_WIDTH - 1) - longitud);
	printf("%sFETCHED: %lu%s", AMARN, instrucciones, RESET);
	// \033[%dD: mueve el cursor a la izquierda
	printf("\033[%dD", LINE_WIDTH - 1);

	if (carga_B) {
		printf("%sFETCH: %s    ", AMARN, RESET);
		escribirInstruccion(etapa_Bin);
	}
	else {
		printf("%sFETCH:%s  x  ", AMARN, AMAR);
		escribirInstruccion(etapa_Bin);
		printf("%s", RESET);
	}
		
	if (carga_D) {
		printf("%sDECOD: %s    ", AMARN, RESET);
		escribirInstruccion(etapa_Din);
	}
	else {
		printf("%sDECOD:%s  x  ", AMARN, AMAR);
		escribirInstruccion(etapa_Din);
		printf("%s", RESET);
	}

	imprimirIssue(sacaISSUE);			// Imprime Issue
	//escribirRegistros();				// Imprime estado registros
	escribirRob(tiempo);				// Imprime el ROB
											
	printf(	"======================================================="
										"==============");	

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
			//case 'i': verir(atoi(&linea[1]), fpch); break;
			case 'b':
				strcpy(quebusco, &linea[1]); /* busca codigos de operacion */
				quebusco[strlen(quebusco)-1] = '\0';
				busca = 1;
				return;
			case 'p':
				switch(linea[1]) {
					case 'f': escribirRegistros(); break;
					case 'l': // escribirMapeoReg(); break;
					default: return;
				}
				break;
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


/**
 * Escribe por pantalla la instucción manteniendo color que recibe
 */
void escribirInstruccion(IREG I)
{
	printf("%-5s", textco[I.co]);
	if (I.co == NOP)
		printf("\n");
	else
		printf("  %2d  %2d  %2d\n", I.rs1, I.rs2, I.rd);
}


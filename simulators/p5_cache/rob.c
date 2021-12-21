#include "cabecera.h"
#include "registros.h"

extern unsigned long int tiempo;

extern escribirInstruccion();

#define MAX_ROB 10

typedef struct ROBentry {
	IREG instr; /* solo necesario para chivato */
	char estado;
	unsigned long terminada;
	char rd, old_rd;
};

struct ROBentry ROB[MAX_ROB];

int ROBocu=0, ROBin=0, ROBout=0;

/**********************************************************/
char ROBhay(){
	    if (ROBocu==MAX_ROB) return 0;
		    return 1;
}   

/**********************************************************/
int ROBadd(IREG instr, int rd, int old){
	int i;
	if (ROBocu==MAX_ROB) {
		printf ("ERROR en funcion ROBadd *****************************\n");
		exit();
	}
	ROBocu++;
	i=ROBin;
	ROB[i].rd=rd;
	ROB[i].old_rd=old;
	ROB[i].terminada=0;
	ROB[i].instr=instr;
	ROB[i].instr.ROBentry=i;
	ROB[i].estado=0;
	ROBin++;
	if (ROBin==MAX_ROB) ROBin = 0;
	return i;
}

/**********************************************************/
void ROBejecuta(int entrada, unsigned long int ciclo){
	ROB[entrada].terminada=ciclo;
	ROB[entrada].estado=1;
}

/**********************************************************/
char ROBjubila(){
	if (ROBocu==0 || ROB[ROBout].terminada>=tiempo || ROB[ROBout].estado==0) return 0;
	if (ROB[ROBout].rd>0 && ROB[ROBout].old_rd>0)libera_RF(ROB[ROBout].old_rd);
	ROBocu--;
	ROBout++;
	if (ROBout==MAX_ROB) ROBout = 0;
	return 1;
}


/**********************************************************/
unsigned long printROBfirst(){
	if (ROBocu==0) return 0;
	return ROB[ROBout].terminada;
}

/**********************************************************/
void escribirRob() {
	struct ROBentry var;
	int i, cont;
		
	printf("%s---------------------------%s\n", AMAR , RESET);
	printf("%sROB: %d\t       %sNOlanzada %sENejecucion %sTerminada%s\n", AMARN, ROBocu, AMAR, RESET, ROJO, RESET);
	printf("%sPOS Tiempo INSTRUCCION%s\n", AMAR , RESET);

			// Muestra por pantalla cada campo del ROB.
	if (ROBin==0) i=MAX_ROB-1;
	else i= ROBin-1;

	for (cont=0; cont<ROBocu; cont++){
		var = ROB[i];
		if (i>0) i--;
		else i=MAX_ROB-1;
							
		if (var.estado==0) 	printf("%s%2d:        ", AMAR,i);
		else if (var.estado==1 && var.terminada>=tiempo) 	
							printf(  "%2d: %6lu ", i , var.terminada-tiempo);
				else 		printf("%s%2d:        ", ROJO,i); 
																			
		escribirInstruccion(var.instr);
		printf("%s",RESET);
	} 
}


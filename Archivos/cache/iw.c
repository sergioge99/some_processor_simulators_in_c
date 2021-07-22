#include "cabecera.h"
#include "registros.h"

/* Esta implementacion de IW mantiene siempre la instr mas vieja en su entrada cero y no guarda huecos.
si tiene n instrucciones, las almacena en las posiciones 0..n-1 en orden de programa
cuando saca una instruccion, siempre compacta
*/

#define MAX_IW 6

extern IREG inula;
extern int latenciasWR[8];

typedef struct IWentry {
	IREG instr;
	int f1,f2,f3;
};

struct IWentry IW[MAX_IW];

int IWocu=0;

/*******************************************************************/
char IWhay(){
	if (IWocu==MAX_IW) return 0;
	return 1;
}


/*******************************************************************/
int IWadd(IREG instr){

	if (IWocu==MAX_IW) {
		printf("ERROR en funcion IWadd *********************************\n");
		exit();
	}

	IW[IWocu].instr=instr;
	IW[IWocu].f1=instr.rs1;
	IW[IWocu].f2=instr.rs2;
    if (instr.rs1==0) IW[IWocu].f1=-1;
    if (instr.rs2==0) IW[IWocu].f2=-1;
	if (instr.co == STORE) IW[IWocu].f3=instr.rd;
	else IW[IWocu].f3=-1;
    if (IW[IWocu].f3==0) IW[IWocu].f3=-1;

	IWocu++;
	return IWocu;
}

/*******************************************************************/
IREG IWsaca(int wBR, char aluf){    
/* el parametro aluf indica si el correspondiente camino esta disponible */

	int i, aux;
	char puedo;
	IREG instr;

	if (IWocu==0) return inula;
	for (i=0; i<IWocu;i++){
		if (Rdisponible(IW[i].f1) && Rdisponible(IW[i].f2) && Rdisponible(IW[i].f3)){
			 puedo=1; /* usar BR en escritura */
			 if (IW[i].instr.rd > 0 && IW[i].instr.co != STORE){
			      aux=1<<(latenciasWR[IW[i].instr.co]+1);
				  puedo = !(wBR & aux);
			 }
			 puedo=puedo && !(IW[i].instr.co == FLOAT && aluf==0);
	    	 if (puedo){
			  	instr=IW[i].instr;
			  	IWocu--;
			  	for (;i<IWocu;i++) IW[i]=IW[i+1];
			  	return instr;
		     }
		}
	}
	return inula;
}	

/*******************************************************************/
int print_iwocu(){
	return IWocu;
}




/**
* Escribe por pantalla la instucció cambiando color registros no disponibles
*/
/*******************************************************************/
void escribirInstruccionIW(IREG I) {
	printf("%s", RESET);
	printf("%s", textco[I.co]);
	if (I.co == NOP  ) printf("\n");
	else {
	    if (!Rdisponible(I.rs1))       printf("%s", PURPU);
        printf(" r1: %2d", I.rs1);
        printf("%s", RESET);

   		if (!Rdisponible(I.rs2))       printf("%s", PURPU);
        printf(" r2: %2d", I.rs2);
        printf("%s", RESET);

		if (I.co==STORE && !Rdisponible(I.rd)) printf("%s", PURPU);
        printf(" rd: %2d\n", I.rd);
        printf("%s", RESET);
    }
}





/**
* Imprime la ISSUE.
*/
/*******************************************************************/
void imprimirIssue(IREG sacaISSUE) {

	int i;
	char preparada;
	  	
	printf("--------------------------\n");
	printf("%sISSUE:  %2d instrucciones\n", AMARN, IWocu);
	printf("%sPOS      V INSTRUCCION\n%s", AMAR, RESET);


	for (int i=IWocu-1;i>=0; i--) {
		preparada= (Rdisponible(IW[i].f1) && Rdisponible(IW[i].f2) && Rdisponible(IW[i].f3));
		printf("%2d:      %d ",	i,preparada);
		escribirInstruccionIW(IW[i].instr);
	}	
	printf("%sISSUED:    %s", ROJO, RESET);
	escribirInstruccionIW(sacaISSUE);
}

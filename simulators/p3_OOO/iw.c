#include "cabecera.h"
#include "registros.h"

/* Esta implementacion de IW mantiene siempre la instruccion mas vieja
   en su entrada cero y no guarda huecos.
   Si tiene n instrucciones, las almacena en las posiciones 0..n-1
   en orden de programa.
   Cuando saca una instruccion, siempre compacta.
*/

#define MAX_IW 6

extern IREG inula;
extern int latenciasWR[8];

typedef struct IWentry {
	IREG instr;
	int f1,f2,f3;
} IWentry_t;

IWentry_t IW[MAX_IW];

int IWocu = 0;

/*******************************************************************/
int IWhay()
{
	/* TODO: sustituir MAX_IW por parámetro tamaño IW */
	if (IWocu == MAX_IW) return 0;
	return 1;
}


/*******************************************************************/
int IWadd(IREG instr)
{
	if (IWocu == MAX_IW) {
		printf("ERROR en funcion IWadd *********************************\n");
		exit(1);
	}

	IW[IWocu].instr = instr;
	IW[IWocu].f1 = instr.rs1;
	IW[IWocu].f2 = instr.rs2;
	if (instr.rs1 == 0) IW[IWocu].f1 = -1;
	if (instr.rs2 == 0) IW[IWocu].f2 = -1;
	if (instr.co == STORE) IW[IWocu].f3 = instr.rd;
	else IW[IWocu].f3 = -1;
	if (IW[IWocu].f3 == 0) IW[IWocu].f3 = -1;

	IWocu++;
	return IWocu;
}

/*******************************************************************/
IREG IWsaca(int wBR, unsigned int aluf)
/* aluf: booleano, camino float libre */
{
	if(IWocu == 0) return inula;
	IREG wakeup = inula;
	char encontrada = 0;
	char ok;
	int wBRaux;
	int i = 0;
	while(encontrada != 1 && i < IWocu){
		//comprobamos si están disponibles los registros fuente
		ok = Rdisponible(IW[i].f1) && Rdisponible(IW[i].f2) && Rdisponible(IW[i].f3);
		//comprobamos riesgo estructural banco registros
		if(IW[i].instr.rd > 0 && IW[i].instr.co != STORE){
			wBRaux = 1 << (latenciasWR[IW[i].instr.co] + 2);//corregido +2 en cpu.c
			ok = ok && !(wBR & wBRaux);
		}
		//comprobamos riesgo estructural UF float
		if(IW[i].instr.co == FLOAT) ok = ok && aluf;
		//si no hay problema, lanzamos la instrucción comprobada
		if(ok){
			wakeup = IW[i].instr;
			encontrada = 1;
		}
		i++;
	}
	if(encontrada == 1){
		//eliminamos la instrucción lanzada de la lista
		for(i = i - 1 ; i < (IWocu - 1); i++){
			IW[i].instr = IW[i + 1].instr;
			IW[i].f1 = IW[i + 1].f1;	
			IW[i].f2 = IW[i + 1].f2;
			IW[i].f3 = IW[i + 1].f3;		
		}
		IWocu--;
	}

	return wakeup;
}	

/*******************************************************************/
int print_iwocu()
{
	return IWocu;
}


/**
 * Escribe por pantalla la instucció cambiando color registros no disponibles
 */
/*******************************************************************/
void escribirInstruccionIW(IREG I)
{
	printf("%s", RESET);
	printf("%-5s", textco[I.co]);
	if (I.co == NOP) printf("\n");
	else {
		if (!Rdisponible(I.rs1)) printf("%s", PURPU);
		printf("  %2d", I.rs1);
		printf("%s", RESET);

   		if (!Rdisponible(I.rs2)) printf("%s", PURPU);
		printf("  %2d", I.rs2);
		printf("%s", RESET);

		if (I.co == STORE && !Rdisponible(I.rd)) printf("%s", PURPU);
		printf("  %2d\n", I.rd);
		printf("%s", RESET);
	}
}



/**
 * Imprime la ISSUE.
 */
/*******************************************************************/
void imprimirIssue(IREG sacaISSUE)
{
	int i;
	char preparada;
	  	
	printf("--------------------------------------\n");
	printf("%sISSUE: %2d instrucciones\n", AMARN, IWocu);
	printf("%sPOS     V   CO    r1  r2  rd\n%s", AMAR, RESET);

	for (i = IWocu-1; i >= 0; i--) {
		preparada = (Rdisponible(IW[i].f1) &&
					 Rdisponible(IW[i].f2) &&
					 Rdisponible(IW[i].f3));
		printf("%2d:     %d  ",	i, preparada);
		escribirInstruccionIW(IW[i].instr);
	}	
	printf("%sISSUED:    %s", ROJO, RESET);
	escribirInstruccionIW(sacaISSUE);
}

void finiw(){}


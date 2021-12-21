#define NOSTEP

/* para eliminar chivato definir NOSTEP */
/* para activar chivato definir STEP */
/* no pueden estar definidos los dos al mismo tiempo */

#include "cabecera.h"

extern IREG etapa_Bin, etapa_Din;

extern char carga_B, carga_D;

void escribirInstruccion();

extern unsigned long instrucciones;

extern void imprimirIssue();				// Imprime Issue.
extern void escribirRegistros();			// Imprime estado registros.
extern void escribirRob();					// Imprime el ROB.

extern char consultar();

typedef struct {
	unsigned long pc, iw;
	long inm;
	char rd, rs1, rs2, *co, ops;
} INST_DEC;

/*extern void decodifica_ins(), imprime_ins();*/

FILE *fpch, *fpchin;


char *textco[8] = { "NOP  ", "LOAD ", "STORE", "ARITM", "BRCON", "BRINC", "FLOAT", "OTROS"};


void chivato(IREG sacaISSUE)
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

/******************************************/
/*    escribe informacion ciclo actual    */
/******************************************/
{

//	int longitud = 107, t = tiempo;
//	while	(t>0)	{ longitud++; t=t/10; }
//	printf("\033[%dC",100); // Mover el cursor a la dcha 50 columnas.
//	printf("%sTIME: %lu%s", AMARN, tiempo, RESET);
//	printf("\033[%dD",longitud); // Mover el cursor a la dcha 50 columnas.
	
	int longitud = 77, t = tiempo;
	while	(t>0)	{ longitud++; t=t/10; }
	printf("\033[%dC",70); // Mover el cursor a la dcha 50 columnas.
	printf("%sTIME: %lu%s", AMARN, tiempo, RESET);
	printf("\033[%dD",longitud); // Mover el cursor a la dcha 50 columnas.

	printf("%sFETCH: %s", AMARN, RESET);
	if (carga_B) {
		printf("%d: ",instrucciones);
		escribirInstruccion(etapa_Bin);
	}
	else printf("\n");
	printf("%sDECOD: %s    ", AMARN, RESET);
	if (carga_D) {
		escribirInstruccion(etapa_Din);
	}
	else printf("\n");
	imprimirIssue(sacaISSUE);				// Imprime Issue.
	//escribirRegistros();							// Imprime estado registros.
	escribirRob();						// Imprime el ROB.
												
	printf(	"======================================================="
																	"========================");	

}

/******************************************/

  while(1)
  {
     if (fgets(linea, 40, fpchin) == NULL) return;
     switch(linea[0])
     {
	case '\0': return;
	case 't': termina = 1;
		  return;
	case 'q': exit();
	case 's': salta = atol(&linea[1]);
		  return;
	case 'g': saltai = atol(&linea[1]);
		  return;
	//case 'i': verir(atoi(&linea[1]), fpch);
	//	  break;
	case 'b': strcpy(quebusco, &linea[1]); /* busca codigos de operacion */
		  quebusco[strlen(quebusco)-1] = '\0';
		  busca = 1;
		  return;
    case 'p': switch(linea[1]){
		  		case 'f': escribirRegistros(); break;
				case 'l': // escribirMapeoReg(); break;
				default: return;
	 		  }
			  break;
	default:  return;
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
void escribirInstruccion(IREG I) {
	printf("%s", textco[I.co]);
	if (I.co == NOP  ) printf("\n");
	else {
			printf(" r1: %2d", I.rs1);
			printf(" r2: %2d", I.rs2);
			printf(" rd: %2d\n", I.rd);
		}
  }

/*	      saltos.c	        */
/* Procesadores Comerciales */
/*       Práctica 4         */
/*  Sergio García Esteban   */
/*         755844           */

#include "cabecera.h"

#define logBTBsize 10
#define logBHTsize 15


static unsigned int historia=0;
static char predictor = 'h';

typedef struct BTBentry {
	unsigned long long tag;
	unsigned long long direccion;
} BTBentry_t;

BTBentry_t BTB[1 << logBTBsize];
char BHT[1 << logBHTsize];

void ini_saltos(){
   int i;
   int BHTsize = 1 << logBHTsize;

   for (i=0; i < BHTsize; i++) BHT[i] = 2;
}

/****************************************************************/
/******    Funciones BTB                    *********************/
/****************************************************************/
char leerBTB (unsigned long long pc, unsigned long long *destino) {
	int mascara = (1 << logBTBsize) - 1;
	int index = (pc >> 2) & mascara;
	int tag = pc >> (logBTBsize + 2);
	if(BTB[index].tag == tag){
		*destino = BTB[index].direccion;
		return 1;	
	}
	return 0;
}

void actualizarBTB (unsigned long long pc, unsigned long long destino) {
	int mascara = (1 << logBTBsize) - 1;
	int index = (pc >> 2) & mascara;
	int tag = pc >> (logBTBsize + 2);
	BTB[index].tag = tag;
	BTB[index].direccion = destino;
}


/****************************************************************/
/******    Funciones especificas BHT local  *********************/
/****************************************************************/
char leerBHTl (unsigned long long pc){
	int mascara = (1 << logBHTsize) - 1;
	int index = (pc >> 2) & mascara;
	char prediccion = BHT[index];
	switch (prediccion){
		case 0 : return 0;
	  	case 1 : return 0;
	  	case 2 : return 1;
		case 3 : return 1;
      	default: printf("ERROR en la variable predictor %d \n",prediccion);
				 exit(1);
  	}
}

void actualizarBHTl (unsigned long long pc, char tomado){
	int mascara = (1 << logBHTsize) - 1;
	int index = (pc >> 2) & mascara;
	char estado = BHT[index];
	switch (estado){
		case 0 : if (tomado) BHT[index] = 1;
	  			 break;
	  	case 1 : if (tomado) BHT[index] = 2;
				 else BHT[index] = 0;
	  			 break;
	  	case 2 : if (tomado) BHT[index] = 3;
	  			 else BHT[index] = 1;
				 break;
		case 3 : if (!tomado) BHT[index] = 2;
				 break;
      	default: printf("ERROR en la variable predictor %d \n",estado);
				 exit(1);
  	}
}


/****************************************************************/
/******    Funciones especificas BHT global  ********************/
/****************************************************************/
char leerBHTg (unsigned long long pc){
	char prediccion = BHT[historia];
	switch (prediccion){
		case 0 : return 0;
	  	case 1 : return 0;
	  	case 2 : return 1;
		case 3 : return 1;
      	default: printf("ERROR en la variable predictor %d \n",prediccion);
				 exit(1);
  	}
}

void actualizarBHTg (unsigned long long pc, char tomado){
	char estado = BHT[historia];
	switch (estado){
		case 0 : if (tomado) BHT[historia] = 1;
	  			 break;
	  	case 1 : if (tomado) BHT[historia] = 2;
				 else BHT[historia] = 0;
	  			 break;
	  	case 2 : if (tomado) BHT[historia] = 3;
	  			 else BHT[historia] = 1;
				 break;
		case 3 : if (!tomado) BHT[historia] = 2;
				 break;
      	default: printf("ERROR en la variable predictor %d \n",estado);
				 exit(1);
  	}
	historia = historia >> 1;
	historia = historia | (tomado << (logBHTsize - 1));
}

/****************************************************************/
/******    Funciones especificas BHT hibrido  *******************/
/****************************************************************/
char leerBHTh (unsigned long long pc){
	int mascara = (1 << logBHTsize) - 1;
	int index = (pc >> 2) & mascara;
	index = index ^ historia; // XOR
	char prediccion = BHT[index];
	switch (prediccion){
		case 0 : return 0;
	  	case 1 : return 0;
	  	case 2 : return 1;
		case 3 : return 1;
      	default: printf("ERROR en la variable predictor %d \n",prediccion);
				 exit(1);
  	}
}

void actualizarBHTh (unsigned long long pc, char tomado){
	int mascara = (1 << logBHTsize) - 1;
	int index = (pc >> 2) & mascara;
	index = index ^ historia; // XOR
	char estado = BHT[index];
	switch (estado){
		case 0 : if (tomado) BHT[index] = 1;
	  			 break;
	  	case 1 : if (tomado) BHT[index] = 2;
				 else BHT[index] = 0;
	  			 break;
	  	case 2 : if (tomado) BHT[index] = 3;
	  			 else BHT[index] = 1;
				 break;
		case 3 : if (!tomado) BHT[index] = 2;
				 break;
      	default: printf("ERROR en la variable predictor %d \n",estado);
				 exit(1);
  	}
	historia = historia >> 1;
	historia = historia | (tomado << (logBHTsize - 1));
}

/****************************************************************/
/******    Funciones genericas BHT     **************************/
/****************************************************************/
char leerBHT (unsigned long long pc){
  switch (predictor){
	  case 'l': return(leerBHTl(pc));
	  case 'g': return(leerBHTg(pc));
	  case 'h': return(leerBHTh(pc));
      default:  printf("ERROR en la variable predictor\n");
				exit(1);
  }
}
void actualizarBHT (unsigned long long pc, char tomado){
  switch (predictor){
	  case 'l': actualizarBHTl (pc, tomado);
	  			break;
	  case 'g': actualizarBHTg (pc, tomado);
	  			break;
	  case 'h': actualizarBHTh (pc, tomado);
	  			break;
      default:  printf("ERROR en la variable predictor\n");
				exit(1);
  }
}

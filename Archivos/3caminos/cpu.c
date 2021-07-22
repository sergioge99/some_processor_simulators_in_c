/*	     cpu_p22.c	        */
/* Procesadores Comerciales */
/*       Práctica 2         */
/*  Sergio García Esteban   */
/*         755844           */

#include "cabecera.h"
#include "rob.h"
#define max_reg 64

extern void chivato();
extern char get_instr();

int latenciasWR[8]={1,2,2,1,1,1,5,1};
/* NOP LOAD STORE ARITM BRCON BRINC FLOAT OTROS */

unsigned long int disp_reg[max_reg];   
/* guarda el ciclo en que se hizo la ultima actualizacion de cada reg o el ciclo en el que se hará
si hay alguna instrucción en vuelo que lo va a modificar */
							/* si es menos que tiempo, esta pendiente de escribir */


IREG inula = {0, 0, 0, 0, -1, -1, -1, NOP, 0, 0};
IREG etapa_Bin, etapa_Din;
IREG etapa_Bout, etapa_Dout;

char carga_B = 1, carga_D = 1;

unsigned long int tiempo = 0;
unsigned long int instrucciones = 0;
static unsigned long int nulas = 0, anuladas = 0, validas = 0, jubiladas = 0;

static unsigned long ccpu = 0, craw = 0, cfloat = 0, cwBR = 0, cwaw = 0, crob = 0;
static unsigned long loads = 0, stores = 0, floats = 0, saltos = 0;

static void reloj();

static char quedan_inst;

unsigned int indice = 0;

void sim()
{
	int ciclos_parada_AF=0;
	int wBR=0;
	int aux;

    /* cargamos la primera instruccion en etapa_Bin  */
    quedan_inst = get_instr(&etapa_Bin);

    while (quedan_inst)
    {
		/* etapa Decode, implementa SCOREBOARD */
		/**********************************************************************************************/
		/* deteccion de riesgos y lectura en BR, 
			si no hay problemas la instruccion pasa a A */
		/* Tres caminos: 
		I: nunca para, alu de un ciclo
		M: nunca para, @ y M de un ciclo (se puede hacer multiciclo segmentada facilmente)
		F: 5 ciclos no segmentada */
		
		wBR=wBR>>1;   /* tras este desplazamiento el bit de menor peso de wBR representa ciclo actual */
		
		jubiladas += ROBjubila(tiempo); /* Jubila (o no) la instrucción más antigua del ROB */

		if (ciclos_parada_AF>0) ciclos_parada_AF--; /* Gestión ciclos de parada float */

		carga_D = 1;

		/* RIESGO RAW */
		if ((etapa_Din.rs1 > 0 && disp_reg[etapa_Din.rs1] > tiempo) ||
			(etapa_Din.rs2 > 0 && disp_reg[etapa_Din.rs2] > tiempo) ||
			(etapa_Din.co == STORE && etapa_Din.rd > 0 && disp_reg[etapa_Din.rd] > tiempo))
		{ 
			craw++;
			carga_D = 0;
		}
		/* RIESGO ESTRUCTURAL EN FLOAT */
		else if (etapa_Din.co == FLOAT && ciclos_parada_AF > 0) 
		{
			cfloat++;
			carga_D = 0;
		}
		else if (etapa_Din.rd > 0 && etapa_Din.co != STORE) 
		{
			aux = 1 << (latenciasWR[etapa_Din.co] + 1);
			if(wBR & aux){
				/* RIESGO ESTRUCTURAL EN BR */
				cwBR++;
				carga_D = 0;
			
			}else if(disp_reg[etapa_Din.rd] > tiempo + latenciasWR[etapa_Din.co]){
				/* RIESGO WAW */
				cwaw++;
				carga_D = 0;
			}else{
				/* NO HAY RIESGO */
				indice = ROBadd(etapa_Din);
				if (indice == -1){
					crob++;
					carga_D = 0;
				}
			}
		}
		/* NO HAY RIESGO */
		else{
			indice = ROBadd(etapa_Din);
			if (indice == -1){
				crob++;
				carga_D = 0;
			}
		}


		if(carga_D){   
			/* lanzo la inst. por el camino correspondiente */
			if (etapa_Din.co == FLOAT) ciclos_parada_AF=5; /* para una latencia de 5 */

			/* si escribe un registro ocupo wBR y marco ciclo en que se produce rd */
			if (etapa_Din.rd > 0 && etapa_Din.co != STORE) {
				wBR=wBR | aux;
				disp_reg[etapa_Din.rd] = tiempo + latenciasWR[etapa_Din.co];
			}
			
			/* asignamos ciclo de terminación de la instrucción actualmente en decode */
			if(etapa_Din.co != STORE && etapa_Din.rd > 0){
				ROBejecuta(indice, tiempo + latenciasWR[etapa_Din.co] + 1);
			}else{
				ROBejecuta(indice, tiempo + latenciasWR[etapa_Din.co]);		
			}
		}

		/* etapa Busqueda */
		/**********************************************************************************************/
		/* Si no hay problemas la instruccion pasa a D */
		/* nada que simular en esta etapa */	

		etapa_Bout = etapa_Bin;
		carga_B = carga_D;

		if (carga_B){
            ccpu++;
            quedan_inst = get_instr(&etapa_Bin);
            if (quedan_inst)
            {
                instrucciones++;
                if (etapa_Bin.co == NOP) nulas++;
                else if (etapa_Bin.an) 
				{
                    anuladas++;
                    etapa_Bin = inula;
                }
                else{
                    validas++;
                    if (etapa_Bin.rd == 0) etapa_Bin.rd = -1;
                    if (etapa_Bin.co == LOAD) loads++;
                    if (etapa_Bin.co == STORE) stores++;
                    if (etapa_Bin.co == FLOAT) floats++;
                    if (etapa_Bin.co == BRCON || etapa_Bin.co == BRINC) saltos++;
                }
            }
        }


		reloj();
    }

}

void reloj()
{
    if (carga_D) etapa_Din = etapa_Bout;
    tiempo++;
    chivato();
}

void inicpu()
{
    int i;

    etapa_Din = inula;
    etapa_Dout = inula;
    etapa_Bout = inula;

	for (i=0;i<max_reg;i++)disp_reg[i]=0;
}

void fincpu()
{
    unsigned long todocpu;
    int i;

    fprintf(fpout, "CPI: %lu inst. %lu jubiladas %lu ciclos %2.2f ciclos/inst.\n", 
	    instrucciones, jubiladas, tiempo, tiempo/(float)instrucciones);
    fprintf(fpout, "%lu NOPs %lu anuladas %lu validas %2.4f c/inst.val.\n",
	    nulas, anuladas, validas, tiempo/(float)validas);

    todocpu =  ccpu + craw + cfloat + cwBR + cwaw + crob;
    fprintf(fpout, "ciclos CPU: %lu cpu %lu RAW %lu WAW %lu FLOAT %lu wBR %lu ROB\t(Total: %lu)\n",
	    ccpu, craw, cwaw, cfloat, cwBR, crob, todocpu);

    fprintf(fpout, "%lu loads %lu stores %lu floats %lu saltos\n", loads, stores, floats, saltos);
}

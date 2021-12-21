/*	cpu.c	*/

#include "cabecera.h"

extern void chivato();
extern char get_instr();

IREG inula = {0, 0, 0, 0, -1, -1, -1, NOP, 0, 0};
IREG etapa_Bin, etapa_Din, etapa_Ain, etapa_Min, etapa_Ein, etapa_Fin;
IREG etapa_Bout, etapa_Dout, etapa_Aout, etapa_Mout, etapa_Eout, etapa_Fout;

char carga_B = 1, carga_D = 1, carga_A = 1, carga_M = 1, carga_E = 1, carga_F = 1;

unsigned long int tiempo = 0;
unsigned long int instrucciones = 0;
static unsigned long int nulas = 0, anuladas = 0, validas = 0;

static unsigned long ccpu = 0;
static unsigned long loads = 0, stores = 0, saltos = 0, floats = 0;

static void reloj();

static char quedan_inst;

static unsigned long int br=0, c1=0, c2=0, c3=0;

static unsigned long int nfloat = 0, lat_float =0;

static unsigned long int camino_A = 0, camino_E = 0;

static unsigned long int cfloat = 0, cbr = 0, craw = 0;

static unsigned long int riesgo_escritura[5] = {0,0,0,0,0};

static unsigned long int wBR = 0;

void sim() 
{
    /* cargamos la primera instruccion en etapa_Bin  */
    quedan_inst = get_instr(&etapa_Bin);

    while (quedan_inst){
		/* etapa Escritura en BR */
		/* nada que simular en esta etapa */	
		etapa_Eout = etapa_Ein;
		carga_E = 1;
		
		/* etapa Memoria: si no hay problemas la instruccion pasa a E */
		if (etapa_Min.co == LOAD || etapa_Min.co == STORE) camino_E = 1;
		etapa_Mout = etapa_Min;
		carga_M = 1;
		
		/* etapa Float: si no hay problemas la instruccion pasa a E */

		if (etapa_Fin.co == FLOAT){
			lat_float++;
			if (lat_float < 5){
				etapa_Fout = inula;
				carga_F = 0;
			}else{
				nfloat++;
				lat_float = 0;
				etapa_Fout = etapa_Fin;
				carga_F = 1;
				camino_E = 3;
			}
		}else{
			etapa_Fout = etapa_Fin;
			carga_F = 1;
		}
		
		/* etapa Alu: si no hay problemas la instruccion pasa a M/E */
		if (etapa_Ain.co != NOP && etapa_Ain.co != LOAD && etapa_Ain.co != STORE){
			camino_E = 2;
		}
		etapa_Aout = etapa_Ain;
		carga_A = 1;

		/* etapa Decode: deteccion de riesgos y lectura en BR, 
			si no hay problemas la instruccion pasa a A */

		if (etapa_Ain.co == LOAD && (etapa_Ain.rd == etapa_Din.rs1 || etapa_Ain.rd == etapa_Din.rs2)){
			etapa_Dout = inula;
			craw++;
			carga_D = 0;
		}else if (etapa_Din.co == FLOAT && etapa_Fin.co == FLOAT && lat_float > 0){
			etapa_Dout = inula;
			cfloat++;
			carga_D = 0;
		}else if (etapa_Din.co == LOAD && riesgo_escritura[1] == 1){
			etapa_Dout = inula;
			cbr++;
			carga_D = 0;
		}else if (etapa_Din.co == ARITM && etapa_Din.rd >= 0 && riesgo_escritura[0] == 1){
			etapa_Dout = inula;
			cbr++;
			carga_D = 0;
		}else{
			if(etapa_Din.co == LOAD){
				riesgo_escritura[1] = 1;
				camino_A = 1;
			}else if (etapa_Din.co == FLOAT){
				riesgo_escritura[4] = 1;
				camino_A = 3;
			}else{
				camino_A = 2;
			}
			etapa_Dout = etapa_Din;
			carga_D = 1;
		}

		/* etapa Busqueda: si no hay problemas la instruccion pasa a D */
		/* nada que simular en esta etapa */	
		etapa_Bout = etapa_Bin;
		carga_B = carga_D;

		if (carga_B){
			ccpu++;
			quedan_inst = get_instr(&etapa_Bin);
			if (quedan_inst){
				instrucciones++;
				if (etapa_Bin.co == NOP) nulas++;
				else if (etapa_Bin.an){
					anuladas++;
					etapa_Bin = inula;
				}else{
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
	int i;
    for(i = 0; i < 4; i++){
		riesgo_escritura[i] = riesgo_escritura[i + 1];
	}
	riesgo_escritura[4] = 0;
	
    if (carga_D) etapa_Din = etapa_Bout;
	
    if (camino_A==1 || camino_A==2){
		etapa_Ain = etapa_Dout;
	}else{
		etapa_Ain = inula;
	}
	
    if (carga_F){
		if(camino_A==3){
			etapa_Fin = etapa_Dout;
		}else{
			etapa_Fin = inula;
		}
	}
    
	etapa_Min = etapa_Aout;
	
    if (camino_E==1){ 
		etapa_Ein = etapa_Mout;
	}else if (camino_E==2){
		etapa_Ein = etapa_Aout;
	}else if (camino_E==3){
		etapa_Ein = etapa_Fout;
	}else{
		etapa_Ein = inula;
	}
	
    tiempo++;
	camino_E = 0;
	camino_A = 0;
    chivato();
}

void inicpu()
{
    int i;

    etapa_Ein = inula;
    etapa_Eout = inula;
    etapa_Min = inula;
    etapa_Mout = inula;
    etapa_Fin = inula;
    etapa_Fout = inula;
    etapa_Ain = inula;
    etapa_Aout = inula;
    etapa_Din = inula;
    etapa_Dout = inula;
    etapa_Bout = inula;
}

void fincpu()
{
    unsigned long todocpu;
    int i;

    fprintf(fpout, "CPI: %lu inst. %lu ciclos %2.2f ciclos/inst.\n", 
	    instrucciones, tiempo, tiempo/(float)instrucciones);
    fprintf(fpout, "%lu NOPs %lu anuladas %lu validas %2.4f c/inst.val.\n",
	    nulas, anuladas, validas, tiempo/(float)validas);

    todocpu =  ccpu + craw + cfloat + cbr;
    fprintf(fpout, "ciclos CPU: %lu cpu %lu RAW %lu FLOAT %lu wBR\t(Total: %lu)\n",
	    ccpu, craw, cfloat, cbr, todocpu);

	fprintf(fpout, "%lu loads %lu stores %lu floats %lu saltos\n", loads, stores, nfloat, saltos);
}

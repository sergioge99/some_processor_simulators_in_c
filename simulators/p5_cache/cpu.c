/*	cpu.c	*/
/* version p5. instrucciones multiciclo float, 5 ciclos latencia sin segmentar
varios caminos de ejecucion
estructural al escribir en banco de registros

ROB   
ejecucion fuera de orden (OOO)
Renombre de registros

*/


#include "cabecera.h"
#include "registros.h"

extern int ROBadd();
extern int ROBhay();
extern void ROBejecuta();
extern char ROBjubila();
extern char IWhay();
extern int IWadd();
extern IREG IWsaca();

extern void chivato();
extern char get_instr();

extern void leer();
extern void escribir();
extern void inicache();
extern void print_cache();


int latenciasWR[8]={1,2,2,1,1,1,5,1};
/* NOP LOAD STORE ARITM BRCON BRINC FLOAT OTROS */

int TRen[64];

IREG inula = {0, 0, 0, 0, 0, -1, -1, -1, NOP, 0, 0};
/* pc, ea, iw, ROBentry, an, rd, rs1, rs2, co, nreg, taken */

IREG etapa_Bin, etapa_Din, etapa_Ain, etapa_Min, etapa_Ein;
IREG etapa_Bout, etapa_Dout, etapa_Aout, etapa_Mout, etapa_Eout;

/* control independiente para cada camino de ejecucion */

IREG etapa_AIin, etapa_AMin, etapa_AFin;
IREG etapa_AIout, etapa_AMout, etapa_AFout;
IREG etapa_IFout, etapa_IMout, etapa_IIout;

char carga_B = 1, carga_D = 1, carga_A = 1, carga_M = 1, carga_E = 1;

char carga_AI = 1, carga_AM = 1, carga_AF = 1;

unsigned long int tiempo = 0;
unsigned long int instrucciones = 0;
static unsigned long int nulas = 0, validas = 0, jubiladas = 0;

static unsigned long ccpu = 0, craw = 0, cfloat = 0, cwBR = 0, cROB=0, creg=0;
static unsigned long loads = 0, stores = 0, floats = 0, saltos = 0;

static void reloj();

static char quedan_inst;

int wBR=0;

void sim()
{
	int ciclos_parada_AF=0;
	int aux, auxrob;
	IREG instr;
	char alum, alui, aluf;

    /* cargamos la primera instruccion en etapa_Bin  */
    quedan_inst = get_instr(&etapa_Bin);

    while (quedan_inst)
    {
	/* etapa consolidacion */

	if(ROBjubila()) jubiladas++;

    /* etapa Escritura en BR */
	/**********************************************************************************************/
	
	wBR=wBR>>1;   /* tras este desplazamiento el bit de menor peso de wBR representa ciclo actual */
	
    /* etapa Alu */
	/**********************************************************************************************/
    /* Tres caminos:
    I: nunca para, alu de un ciclo
    M: nunca para, @ y M de un ciclo (se puede hacer multiciclo segmentada facilmente)
    F: 5 ciclos no segmentada */
	
	/* parada en FLOAT */	
	
	if (ciclos_parada_AF>0) {
		ciclos_parada_AF--;
	}
	else if (etapa_AFin.co == FLOAT){
			ciclos_parada_AF=4; /* para una latencia de 5 */
			carga_AF = 0;
	}

	if (ciclos_parada_AF==0) {
		carga_AF = 1;
	}



	/* etapa ISSUE: deteccion de riesgos RAW, estructurales */
	/**********************************************************************************************/

	/*etapa_IIout = inula;*/
	/*etapa_IMout = inula;*/
	etapa_IFout = inula;

 	instr= IWsaca(wBR, carga_AF);

	if(instr.co!=NOP){
			ROBejecuta(instr.ROBentry, tiempo + latenciasWR[instr.co]);
			if (instr.co == FLOAT) etapa_IFout = instr;
			else if (instr.co == LOAD || instr.co == STORE) etapa_IMout = instr;
				 else etapa_IIout = instr;
			if (instr.rd > 0 && instr.co != STORE) {
					aux=1<<(latenciasWR[instr.co]+1);
					 wBR=wBR | aux;
					 produce_reg(instr.rd, tiempo+latenciasWR[instr.co]);
			}
	}

	/* etapa Decode: Renombre e insercion en ROB e IW */ 
	/**********************************************************************************************/

	carga_D=0;
	if (etapa_Din.co==NOP) carga_D=1;
		else if (ROBhay() && IWhay()){
			int rf1=-1, rf2=-1, rf3=-1, rd=0, old_rd=-1;
			if (etapa_Din.rd==0) etapa_Din.rd=-1;

			if (etapa_Din.rs1>0) rf1=TRen[etapa_Din.rs1];
			if (etapa_Din.rs2>0) rf2=TRen[etapa_Din.rs2];
			if (etapa_Din.rd>0 && etapa_Din.co == STORE) rf3=TRen[etapa_Din.rd];

			if (etapa_Din.rd>0 && etapa_Din.co != STORE) {
				old_rd=TRen[etapa_Din.rd];
				rd=asigna_RF();
			}
			if (rd==-1) {
				creg++;
			}
			else{
				etapa_Din.rs1=rf1;
				etapa_Din.rs2=rf2;
				if (etapa_Din.rd>0 && etapa_Din.co == STORE) etapa_Din.rd=rf3;
				if (etapa_Din.rd>0 && etapa_Din.co != STORE) {
					TRen[etapa_Din.rd]=rd;
					etapa_Din.rd=rd;
				}

				etapa_Din.ROBentry=ROBadd(etapa_Din, etapa_Din.rd, old_rd);
				IWadd(etapa_Din);
				if (etapa_Din.rd > 0 && etapa_Din.co != STORE) pendiente_reg(etapa_Din.rd);
				carga_D=1;
			}
		}


	/* etapa Busqueda D */
	/**********************************************************************************************/
	/* nada temporal que simular en esta etapa, predictor ideal, nunca para */	

	etapa_Bout = etapa_Bin;
	carga_B = carga_D;

	if (carga_B)
	{
	    ccpu++;
	    quedan_inst = get_instr(&etapa_Bin);
	    if (quedan_inst)
	    {
		instrucciones++;
		if (etapa_Bin.co == NOP) nulas++;
		else
		{
		    validas++;
		    if (etapa_Bin.co == LOAD && !etapa_Bin.an) {     
/*          elimina las anuladas para que sea comparable con cachesim5, tambien en stores  */
				loads++;
				leer(etapa_Bin.ea);
//				printf("%x ",(unsigned long) etapa_Bin.ea);
			}
		    if (etapa_Bin.co == STORE && !etapa_Bin.an) {
				stores++;
				escribir(etapa_Bin.ea);
//				printf("%x \n",(unsigned long) etapa_Bin.ea);
			}
		    if (etapa_Bin.co == FLOAT) floats++;
		    if (etapa_Bin.co == BRCON || etapa_Bin.co == BRINC) saltos++;
		}
	    }
	}
	reloj();
    chivato(instr);
    }	

}

void reloj()
{
    if (carga_D) etapa_Din = etapa_Bout;
    if (carga_AF) etapa_AFin = etapa_IFout;
	/* etapa_AIin = etapa_IIout; */
	/* etapa_AMin = etapa_IMout; */
    tiempo++;
}

void inicpu()
{
    int i;

    etapa_Ain = inula;
    etapa_Aout = inula;
    etapa_Din = inula;
    etapa_Dout = inula;
    etapa_Bout = inula;
	for(i=0;i<64;i++)TRen[i]=i;
	ini_reg();
	inicache();
}

void fincpu()
{
    unsigned long todocpu;
    int i;

    fprintf(fpout, "CPI: %lu inst. %lu jubiladas %lu ciclos %2.2f ciclos/inst.\n", 
	    instrucciones, jubiladas, tiempo, tiempo/(float)instrucciones);
    fprintf(fpout, "%lu NOPs %lu validas %2.4f c/inst.val.\n",
	    nulas, validas, tiempo/(float)validas);

    todocpu =  ccpu + craw + cfloat + cwBR + cROB + creg;
    fprintf(fpout, "ciclos CPU: %lu cpu %lu RAW %lu FLOAT %lu wBR %lu ROB %lu Renombre \t(Total: %lu)\n",
	    ccpu, craw, cfloat, cwBR, cROB, creg, todocpu);

    fprintf(fpout, "%lu loads %lu stores %lu floats %lu saltos\n", loads, stores, floats, saltos);
	print_cache();
}



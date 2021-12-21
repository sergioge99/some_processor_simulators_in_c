/*	cpu.c	*/
/* version p5.
instrucciones multiciclo float, 5 ciclos latencia sin segmentar
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
extern int ROBvacio();
extern void ROBejecuta();
extern int ROBjubila();
extern int IWhay();
extern int IWadd();
extern IREG IWsaca();
extern void finiw();

extern void chivato();
extern char get_instr();

int latenciasWR[8] = {1, 2, 2, 1, 1, 1, 5, 1};
/* NOP LOAD STORE ARITM BRCON BRINC FLOAT OTROS */

int TRen[64];

IREG inula = {0, 0, 0, 0, 0, -1, -1, -1, NOP, 0, 0};
/* pc, ea, iw, ROBentry, an, rd, rs1, rs2, co, nreg, taken */

IREG etapa_Bin, etapa_Bout, etapa_Din;
char carga_B = 1, carga_D = 1, carga_AF = 1;

unsigned long int tiempo = 0;
unsigned long int instrucciones = 0;
static unsigned long int nulas = 0, anuladas = 0, validas = 0, jubiladas = 0;

static unsigned long ccpu = 0, cIW = 0, cROB = 0, creg = 0;
static unsigned long loads = 0, stores = 0, floats = 0, saltos = 0;

static void reloj();

static char quedan_inst;

int wBR = 0;

static unsigned int IW_no_tiene_disponibles = 0, ROB_no_jubila = 0, parada_decode = 0;

void sim()
{
	unsigned long int ciclo_commit;
	unsigned int ciclos_parada_AF = 0;
	int aux, hay_wb;
	IREG instr;

	/* cargamos la primera instruccion en etapa_Bin  */
	quedan_inst = 1;
	// quedan_inst = get_instr(&etapa_Bin);
	// while (quedan_inst)
	while (quedan_inst || (!ROBvacio()))
	{
		/* etapa consolidacion */
		/**********************************************************************/
		if (ROBjubila()){
			jubiladas++;
		}else{
			ROB_no_jubila++;
		}

		/* etapa Escritura en BR */
		/**********************************************************************/

		/* tras este desplazamiento
		   el bit de menor peso de wBR representa ciclo actual */
		wBR = wBR >> 1;
	
		/* etapa Alu */
		/**********************************************************************/
		/* Tres caminos:
			I: nunca para, alu de un ciclo
			M: nunca para, @ y M de un ciclo
				(se puede hacer multiciclo segmentada facilmente)
			F: 5 ciclos no segmentada */
		/* nada que simular en esta etapa */

		/* etapa ISSUE: deteccion de riesgos RAW, estructurales */
		/**********************************************************************/

		/* actualizar contador parada en FLOAT */	
		if (ciclos_parada_AF > 0) ciclos_parada_AF--;
		if (ciclos_parada_AF == 0) carga_AF = 1;
		else carga_AF = 0;
		instr = IWsaca(wBR, carga_AF);
		if(instr.co==inula.co){
			IW_no_tiene_disponibles++;
		}

		/* iniciar contador parada en FLOAT */	
		if (instr.co == FLOAT)
			ciclos_parada_AF = latenciasWR[FLOAT];

		if (instr.co != NOP) {
			if (instr.rd > 0 && instr.co != STORE)	hay_wb = 1;
			else									hay_wb = 0;
			ciclo_commit = tiempo + latenciasWR[instr.co] + 2 + hay_wb;
			ROBejecuta(instr.ROBentry, ciclo_commit);
			if (instr.rd > 0 && instr.co != STORE) {
				aux = 1 << (latenciasWR[instr.co] + 2);//corregido +2
				wBR = wBR | aux;
				produce_reg(instr.rd, tiempo + latenciasWR[instr.co]);
			}
		}

		/* etapa Decode: Renombre e insercion en ROB e IW */ 
		/**********************************************************************/

		carga_D = 0;
		if (etapa_Din.co == NOP) {
			carga_D = 1;
			ccpu++;
		}
		else {
			if (ROBhay() && IWhay()) {
				int rf1 = -1, rf2 = -1, rf3 = -1, rd = 0, old_rd = -1;
				if (etapa_Din.rd == 0) etapa_Din.rd = -1;
				if (etapa_Din.rs1 > 0) rf1 = TRen[etapa_Din.rs1];
				if (etapa_Din.rs2 > 0) rf2 = TRen[etapa_Din.rs2];
				if (etapa_Din.rd > 0 && etapa_Din.co == STORE)
					rf3 = TRen[etapa_Din.rd];
				if (etapa_Din.rd > 0 && etapa_Din.co != STORE) {
					old_rd = TRen[etapa_Din.rd];
					rd = asigna_RF();
				}

				if (rd == -1) {
					creg++;
				}
				else {
					etapa_Din.rs1 = rf1;
					etapa_Din.rs2 = rf2;
					if (etapa_Din.rd > 0 && etapa_Din.co == STORE)
						etapa_Din.rd = rf3;
					if (etapa_Din.rd > 0 && etapa_Din.co != STORE) {
						TRen[etapa_Din.rd] = rd;
						etapa_Din.rd = rd;
					}

					etapa_Din.ROBentry = ROBadd(etapa_Din, etapa_Din.rd, old_rd);
					IWadd(etapa_Din);
					if (etapa_Din.rd > 0 && etapa_Din.co != STORE)
						pendiente_reg(etapa_Din.rd);
					carga_D = 1;
					ccpu++;
				}
			}
			else {
				if (ROBhay()) cIW++;
					else cROB++;
			}
		}

		if(carga_D == 0){
			parada_decode++;
		}

		/* etapa Busqueda D */
		/**********************************************************************/
		/* nada temporal que simular en esta etapa, predictor ideal, nunca para */	

		etapa_Bout = etapa_Bin;
		carga_B = carga_D & quedan_inst;

		/* no hay instrucciones: hay que drenar el ROB */
		if (carga_B) {
			quedan_inst = get_instr(&etapa_Bin);
			if (quedan_inst) {
				instrucciones++;
				if (etapa_Bin.co == NOP) nulas++;
				else if (etapa_Bin.an) {
					anuladas++;
					etapa_Bin = inula;
				}
				else {
					validas++;
					if (etapa_Bin.rd == 0) etapa_Bin.rd = -1;
					if (etapa_Bin.co == LOAD) loads++;
					if (etapa_Bin.co == STORE) stores++;
					if (etapa_Bin.co == FLOAT) floats++;
					if (etapa_Bin.co == BRCON || etapa_Bin.co == BRINC) saltos++;
				}
			}
			else /* !quedan_inst */
				etapa_Bin = inula;
		}

		reloj();
		chivato(instr, instrucciones);
	}
}


void reloj()
{
	if (carga_D) etapa_Din = etapa_Bout;
	tiempo++;
}


void inicpu()
{
	int i;

	etapa_Bin = inula; etapa_Bout = inula; etapa_Din = inula;
	for (i = 0; i < 64; i++) TRen[i] = i;
	ini_reg();
}

void fincpu()
{
	unsigned long todocpu;

	fprintf(fpout, "\n");
	fprintf(fpout, "CPI = ciclos/inst.jub. = %lu / %lu = %2.2f ciclos/inst.\n", 
		tiempo, jubiladas, (float)tiempo/(float)jubiladas);

	fprintf(fpout, "ciclos: %lu cpu + %lu IWllena + %lu ROBlleno + %lu NoRegistros = %lu\n", ccpu, cIW, cROB, creg, ccpu + cIW + cROB + creg);
	fprintf(fpout, "I: %lu buscadas = %lu NOPS + %lu anuladas + %lu jubiladas (%lu validas)\n",
		instrucciones, nulas, anuladas, jubiladas, validas);
	fprintf(fpout, "desglose I: %lu loads + %lu stores + %lu floats + %lu saltos\n",
		loads, stores, floats, saltos);
	fprintf(fpout, "-IW no tiene instrucción disponible: %lu (incluye NOPs obtenidas en búsqueda)\n",
		IW_no_tiene_disponibles);
	fprintf(fpout, "-Decode no carga instrucción: %lu (IWllena + ROBlleno)\n",
		parada_decode);
	fprintf(fpout, "-ROB no jubila: %lu \n",
		ROB_no_jubila);
	finiw();
}


/*	cpu.c	*/
/* version p8.
instrucciones multiciclo float, 5 ciclos latencia sin segmentar
varios caminos de ejecucion
estructural al escribir en banco de registros

ROB   
ejecucion fuera de orden (OOO)
Renombre de registros

Saltos con temporización

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

extern void ini_saltos();
extern char leerBTB();
extern char leerBHT();
extern void actualizarBTB();
extern void actualizarBHT();

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

static unsigned long saltos_ko = 0;
static char acierto_predictor;

static unsigned long long dir_salto;

static void reloj();

static char quedan_inst;

int wBR = 0;

void sim()
{
	unsigned long int ciclo_commit;
	unsigned int ciclos_parada_AF = 0;
	int aux, hay_wb;
	char B_parada = 0;
	IREG instr;

	/* cargamos la primera instruccion en etapa_Bin  */
	quedan_inst = 1;
	// quedan_inst = get_instr(&etapa_Bin);
	// while (quedan_inst)
	while (quedan_inst || (!ROBvacio()))
	{
		/* etapa consolidacion */
		/**********************************************************************/
		if (ROBjubila()) jubiladas++;

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

		/* iniciar contador parada en FLOAT */	
		if (instr.co == FLOAT)
			ciclos_parada_AF = latenciasWR[FLOAT];

		if (instr.co != NOP) {
			if (instr.rd > 0 && instr.co != STORE)	hay_wb = 1;
			else									hay_wb = 0;
			ciclo_commit = tiempo + latenciasWR[instr.co] + 2 + hay_wb;
			ROBejecuta(instr.ROBentry, ciclo_commit);
			if (instr.rd > 0 && instr.co != STORE) {
				aux = 1 << (latenciasWR[instr.co] + 1);
				wBR = wBR | aux;
				produce_reg(instr.rd, tiempo + latenciasWR[instr.co]);
			}
			if ((instr.co == BRCON || instr.co == BRINC) && instr.mispred) B_parada = 3; /*quedan 2 ciclos de parada en B*/
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


		/* etapa Busqueda */
		/**********************************************************************/
		/* en esta etapa simulamos predictor de saltos */	
		/* si el predictor falla, entrara inula a B hasta que el salto se resuelva */

		etapa_Bout = etapa_Bin;
		carga_B = carga_D & quedan_inst;
		if (B_parada > 0) B_parada--;

		/* solo llamamos al predictor si la instruccion en B continua hacia D */
		if (carga_B && B_parada == 0) {
				acierto_predictor = 1;
				etapa_Bout.mispred = 0;
				if (etapa_Bin.co == BRINC){
					acierto_predictor = 0;
				    if (leerBTB (etapa_Bin.pc, &dir_salto))
				    	if (dir_salto == etapa_Bin.ea) acierto_predictor = 1;
					actualizarBTB(etapa_Bin.pc, etapa_Bin.ea);
				}
				if (etapa_Bin.co == BRCON){
					acierto_predictor = 0;
				    if (leerBTB (etapa_Bin.pc, &dir_salto) && leerBHT (etapa_Bin.pc)) {
						if (etapa_Bin.taken && dir_salto == etapa_Bin.ea) acierto_predictor = 1;
					}
					else if (!etapa_Bin.taken) acierto_predictor = 1;
					actualizarBHT(etapa_Bin.pc, etapa_Bin.taken);
					if (etapa_Bin.taken) actualizarBTB(etapa_Bin.pc, etapa_Bin.ea);
				}
				/* CUIDADO: la siguiente linea es para desconectar predictor */
				//acierto_predictor = 1; 
				if (!acierto_predictor) {
					carga_B = 0;
					etapa_Bout.mispred = 1;
					B_parada = -1; /* esperando que salto fallado se lance a ejecucion */
					etapa_Bin = inula;
					saltos_ko++;
				}
		}

		/* solo entra nueva instruccion a B si la instruccion en B continua hacia D 
		  y el predictor de saltos ha acertado */
		if (carga_B && B_parada == 0) {
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
			else /* !quedan_inst */ /* no hay instrucciones: hay que drenar el ROB */
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
	ini_saltos();
}

void fincpu()
{
	unsigned long todocpu;

	fprintf(fpout, "\n");
	fprintf(fpout, "CPI = ciclos/inst.jub. = %lu / %lu = %2.2f ciclos/inst.\n", 
		tiempo, jubiladas, (float)tiempo/(float)jubiladas);

	fprintf(fpout, "ciclos: %lu cpu + %lu IWllena + %lu ROBlleno + %lu NoRegistros = %lu\n", ccpu, cIW, cROB, creg, ccpu + cIW + cROB + creg);
	fprintf(fpout, "I: %lu buscadas = %lu NOPS + %lu jubiladas (%lu validas)\n",
		instrucciones, nulas, jubiladas, validas);
	fprintf(fpout, "desglose I: %lu loads + %lu stores + %lu floats + %lu saltos\n",
		loads, stores, floats, saltos);
	fprintf(fpout, "Predictor saltos: %lu saltos  %lu fallos  %2.2f%%\n",
		saltos, saltos_ko, 100.0*(float)saltos_ko/(float)saltos);
}


/*	simulador.c   pablo  */

#include <shade.h>
#include <shade_sparcv9.h>
#include "cabecera.h"

struct shade_trace_s {SHADE_SPARCV9_TRACE};

char top3[64] =		/* tabla de load y store OP=3 */
		   {1,1,1,1, 2,2,2,2,	/* normal */
		    1,1,1,1, 0,1,2,1,
		    1,1,1,1, 2,2,2,2,	/* alternate space */
		    1,1,1,1, 0,1,2,1,
		    1,1,1,1, 2,2,2,2,	/* float */
		    0,0,0,0, 0,1,0,0,
		    1,0,1,1, 2,0,2,2,	/* float alternate */
		    0,0,0,0, 1,1,1,0
};

char top2[64] =
		   {0,0,0,0, 0,0,0,0,	/* tabla de operandos para OP=2 */
		    0,4,0,0, 0,4,0,0,
		    0,0,0,0, 0,0,0,0,	/* 0:fuentes y destino	*/
		    0,4,0,0, 0,4,0,0,	/* 1:solo fuentes	*/
		    0,0,0,0, 0,0,0,0,	/* 2:solo destino	*/
		    2,4,4,4, 4,4,4,4,	/* 3:float no CMPF	*/
		    1,4,4,4, 3,5,4,4,	/* 4:no valida, priv. o copro*/
		    0,1,1,1, 0,0,4,4	/* 5:float CMPF		*/
};


void
traduce(shade_trace_t *t, IREG *ir)
{
    spix_sparc_inst_t iw;
    unsigned long auxop3, auxopf;

    ir->pc = t->tr_pc;
    ir->ea =t->tr_ea;
    ir->an = t->tr_annulled;
    iw = t->tr_i;
    ir->iw = iw.inst;
    ir->rd = getRD(ir->iw);
    ir->rs1 = getRS1(ir->iw);
    ir->taken = t->tr_taken;

    if (getI(ir->iw) != 0)
    {
	ir->nreg = 1;
	ir->rs2 =- 1;
    }
    else
    {
	ir->nreg = 2;
	ir->rs2 = getRS2(ir->iw);
    }

    ir->co = ARITM;

    switch (getOP(ir->iw))
    {
	case 1:
	    ir->rd = ir->rs1 = ir->rs2 = -1;           /*CALL */
	    ir->co=BRINC;
	    ir->taken = 1;
	    break;

	case 0:
	    if (getOP2(ir->iw) != 4)
	    {	/* SALTO */
		ir->rd = -1;         	/* OP2==4 => SETHI */ 
		if (getCOND(ir->iw) == 8)	ir->co = BRINC;
		else				ir->co = BRCON;
	    }
	    else
	    {
		if (ir->rd==0)
		    ir->co = NOP;
		else
		    ir->co = ARITM;  /* SETHI */

		ir->rs1 = ir->rs2 = -1;             
		if (getOP2(ir->iw) == 6)
		    ir->co = BRCON;  /* JMPF, salto P.F. */
	    }
	    break;		

	case 2:
	    auxop3 = getOP3(ir->iw);
	    if (auxop3==0x38)
	    {
		ir->co = BRINC;
		ir->taken = 1;
		ir->rd = -1;
	    } /* JMPLINK simplificado*/
	    if (auxop3 == 0x3c) ir->co = ARITM; /* SAVE */
	    if (auxop3 == 0x3d) ir->co = ARITM; /* RESTORE */
	    switch(top2[auxop3])
	    {
		case 0: break;
		case 1: ir->rd = -1; break;
		case 2: ir->rs1 = ir->rs2 = -1; break;
		case 3: ir->co = FLOAT;
			ir->rs1 += 32;
			if (ir->rs2 != -1) ir->rs2 += 32;
			if (ir->rd != -1)  ir->rd += 32;
			break;
		case 5: ir->co = FLOAT; 	/* CMPF */
			ir->rs1 += 32;
			if (ir->rs2 >= 0) ir->rs2 += 32;
		default: ;
	    }
	    break;

	case 3:		/* LOAD, STORE instructions */
	    ir->co = top3[getOP3(ir->iw)];
	    if (getOP3(ir->iw)>31) ir->rd += 32; /* float */
			if (getOP3(ir->iw)==0x2d) ir->rd =-1; /* prefetch */
	    default: ;
    }
}


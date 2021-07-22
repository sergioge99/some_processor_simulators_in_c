#define SETS 64  /* no se pueden cambiar sin cambiar codigo */
#define ASOC 2

typedef struct CACHEblock {
	unsigned long long tag;
	char valido;
	char sucio; 		/* en valido y sucio solo se usa un bit */
};

typedef struct CACHEset { 
	struct CACHEblock bloques[ASOC];
	char LRU;		/* en LRU solo se usa un bit para ASOC=2 */
};

struct CACHEset Cache_datos[SETS];

static unsigned long lecturas=0, escrituras=0, fallos=0, sucios=0, expulsados=0;

char leer(unsigned long long dir){
	
	lecturas++;
	/* CALCULAMOS CONJUNTO */
	char conjunto = (dir >> 6) & (SETS-1);
	unsigned long long tag = dir >> 12;
	char LRU = Cache_datos[conjunto].LRU;
	
	/* BUSCAMOS EN EL CONJUNTO */
	for(int i = 0; i < ASOC; i++){
		if(Cache_datos[conjunto].bloques[i].tag == tag && Cache_datos[conjunto].bloques[i].valido == 1){
			/* ACIERTO */
			if(i == LRU){
				Cache_datos[conjunto].LRU = (LRU + 1) & 1;
			}
			return 1;
		}
	}
	/* FALLO */
	fallos++;
	if(Cache_datos[conjunto].bloques[LRU].valido == 1){
		expulsados++;
		if(Cache_datos[conjunto].bloques[LRU].sucio == 1){
			sucios++;
		}
	}
	Cache_datos[conjunto].bloques[LRU].tag = tag;
	Cache_datos[conjunto].bloques[LRU].valido = 1;
	Cache_datos[conjunto].bloques[LRU].sucio = 0;
	
	Cache_datos[conjunto].LRU = (LRU + 1) & 1;
	return 0;
}


char escribir(unsigned long long dir){
	
	escrituras++;
	/* CALCULAMOS CONJUNTO */
	char conjunto = (dir >> 6) & (SETS-1);
	unsigned long long tag = dir >> 12;
	char LRU = Cache_datos[conjunto].LRU;
	
	/* BUSCAMOS EN EL CONJUNTO */
	for(int i = 0; i < ASOC; i++){
		if(Cache_datos[conjunto].bloques[i].tag == tag && Cache_datos[conjunto].bloques[i].valido == 1){
			/* ACIERTO */
			Cache_datos[conjunto].bloques[i].sucio = 1;
			if(i == LRU){
				Cache_datos[conjunto].LRU = (LRU + 1) & 1;
			}
			return 1;
		}
	}

	/* FALLO */
	fallos++;
	if(Cache_datos[conjunto].bloques[LRU].valido == 1){
		expulsados++;
		if(Cache_datos[conjunto].bloques[LRU].sucio == 1){
			sucios++;
		}
	}
	Cache_datos[conjunto].bloques[LRU].tag = tag;
	Cache_datos[conjunto].bloques[LRU].valido = 1;
	Cache_datos[conjunto].bloques[LRU].sucio = 1;
	
	Cache_datos[conjunto].LRU = (LRU + 1) & 1;
	return 0;
}

/* llamar a esta funcion desde inicpu() */
void inicache(){
	for(int i=0; i<SETS; i++){
		for(int j=0; j<ASOC; j++){
			Cache_datos[i].bloques[j].tag=0;
			Cache_datos[i].bloques[j].valido=0;
			Cache_datos[i].bloques[j].sucio=0;
		}
		Cache_datos[i].LRU=0;
	}
}

/* llamar a esta funcion desde fincpu() */
void print_cache(){
	printf("CACHE: %lu lect %lu escr %lu acc %lu fallos (%2.1f%%) %lu expul %lu sucios \n",
	lecturas, escrituras, lecturas+escrituras, fallos, 100*(float)fallos/(lecturas+escrituras), expulsados, sucios);

}

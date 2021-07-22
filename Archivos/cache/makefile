SHADE = /usr/local/shade/shade-32
SPIXT = /usr/local/shade/shade-32
INC = -I$(SHADE)/inc
DEFS = -DSPIX_ADDR=64 $(ARCH_DEFS)
MAPFILE = $(SHADE)/lib/mapfile
LDFLAGS = -Wl,-M,$(MAPFILE)
OPT = -O
#OPT = -xO3
CFLAGS = $(INC) $(OPT) $(DEFS)
LIBS    = $(SHADE)/lib/libshade.a $(SPIXT)/lib/libspix_sparc.a

simseg: main.o chivato.o traduce.o cpu.o rob.o iw.o registros.o cache.o $(MAPFILE)
	cc  $(LDFLAGS) -o simseg main.o cpu.o chivato.o traduce.o rob.o iw.o registros.o cache.o $(LIBS) -lc
cache.o: cache.c
	cc -c $(CFLAGS) cache.c
registros.o: registros.c registros.h cabecera.h
	cc -c $(CFLAGS) registros.c
iw.o: iw.c cabecera.h registros.h
	cc -c $(CFLAGS) iw.c
rob.o: rob.c cabecera.h
	cc -c $(CFLAGS) rob.c
chivato.o: chivato.c cabecera.h
	cc -c $(CFLAGS) chivato.c
cpu.o: cpu.c cabecera.h registros.h
	cc -c $(CFLAGS) cpu.c
traduce.o: traduce.c cabecera.h
	cc -c $(CFLAGS) traduce.c
main.o: main.c
	cc -c $(CFLAGS) main.c


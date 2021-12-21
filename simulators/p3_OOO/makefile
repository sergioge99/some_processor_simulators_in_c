SHADE = /usr/local/shade/shade-32
SPIXT = /usr/local/shade/shade-32
INC = -I$(SHADE)/inc
DEFS = -DSPIX_ADDR=64 $(ARCH_DEFS)
MAPFILE = $(SHADE)/lib/mapfile
LDFLAGS = -Wl,-M,$(MAPFILE)
OPT = -O
#OPT = -xO3
CFLAGS = $(INC) $(OPT) $(DEFS) -v
LIBS    = $(SHADE)/lib/libshade.a $(SPIXT)/lib/libspix_sparc.a
CC = cc

simseg: main.o chivato.o traduce.o cpu.o rob.o iw.o registros.o $(MAPFILE)
	$(CC)  $(LDFLAGS) -o simseg main.o cpu.o chivato.o traduce.o rob.o iw.o registros.o $(LIBS) -lc

registros.o: registros.c registros.h cabecera.h
	$(CC) -c $(CFLAGS) registros.c
iw.o: iw.c cabecera.h registros.h
	$(CC) -c $(CFLAGS) iw.c
rob.o: rob.c cabecera.h
	$(CC) -c $(CFLAGS) rob.c
chivato.o: chivato.c cabecera.h
	$(CC) -c $(CFLAGS) chivato.c
cpu.o: cpu.c cabecera.h registros.h
	$(CC) -c $(CFLAGS) cpu.c
traduce.o: traduce.c cabecera.h
	$(CC) -c $(CFLAGS) traduce.c
main.o: main.c
	$(CC) -c $(CFLAGS) main.c
clean:
	rm simseg *.o


all: main.o arvoreb.o arvorebe.o sequencialIndex.o geradorDeArquivos.o analiseExperimental.o
	@gcc main.o arvoreb.o arvorebe.o sequencialIndex.o geradorDeArquivos.o analiseExperimental.o -o pesquisa -lm
	@rm -f main.o arvoreb.o arvorebe.o sequencialIndex.o geradorDeArquivos.o analiseExperimental.o

main.o: main.c
	@gcc -c main.c -Wall

arvoreb.o: arvoreb.c
	@gcc -c arvoreb.c -Wall

arvorebe.o: arvorebe.c
	@gcc -c arvorebe.c -Wall

sequencialIndex.o: sequencialIndex.c
	@gcc -c sequencialIndex.c -Wall

geradorDeArquivos.o: geradorDeArquivos.c
	@gcc -c geradorDeArquivos.c -Wall

analiseExperimental.o: analiseExperimental.c
	@gcc -c analiseExperimental.c -Wall

clean:
	@rm -f pesquisa
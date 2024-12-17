CC := gcc
CFLAGS := -Wall -Wextra -pedantic -g

.PHONY: all clean 

all: paroliere_srv paroliere_cl

paroliere_srv: paroliere_srv.c messaggio.c paroliere_matrix.c paroliere_utenti.c paroliere_bacheca.c paroliere_utils.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

paroliere_cl: paroliere_cl.c messaggio.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -f *.o paroliere_srv paroliere_cl

test:
	./paroliere_srv --port 12345 &
	./paroliere_cl --server 127.0.0.1 --port 12345

test-base:
	./paroliere_srv 12345 --port 12345 --matrici matrixes.txt --durata 2 --seed 42 &
	./paroliere_cl --server 127.0.0.1 --port 12345 

test-advanced:
	./paroliere_srv --port 12345 --durata 2 --matrici matrixes.txt --seed 42 --disconnetti-dopo 5 &
	./paroliere_cl --server 127.0.0.1 --port 12345

test-quick:
	./paroliere_srv --port 12345 --durata 1 --matrici matrixes.txt --disconnetti-dopo 1 &
	./paroliere_cl --server 127.0.0.1 --port 12345
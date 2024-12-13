CC := gcc
CFLAGS := -Wall -Wextra -pedantic -g

.PHONY: all clean 

all: paroliere_srv paroliere_cl

paroliere_srv: paroliere_srv.c messaggio.c paroliere_matrix.c paroliere_utenti.c paroliere_bacheca.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

paroliere_cl: paroliere_cl.c messaggio.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -f *.o paroliere_srv paroliere_cl

test: all
	./paroliere_srv 12345 &
	sleep 2
	xterm -hold -e ./paroliere_cl localhost 12345

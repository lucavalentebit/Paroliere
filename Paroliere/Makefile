CC := gcc
CFLAGS := -Wall -Wextra -pedantic -g
LIBS := libs
SRC := $(wildcard $(LIBS)/*.c)

.PHONY: all clean 

all: paroliere_srv paroliere_cl

paroliere_srv: paroliere_srv$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

server.o: server.c
	$(CC) $(CFLAGS) -c $< -o $@

client: client.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
	
client.o: client.c
	$(CC) $(CFLAGS) -c $< -o $@ 

$(LIBS)/%.o: $(LIBS)/%.c $(HEADER)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f -r $(LIBS)/*.o *.o server client $(OBJ) *.log bib.conf new_bibData 

test : server client
	./server Massa bibData/bib1.txt 3 &
	./server Pisa bibData/bib2.txt 3 &
	./server Siena bibData/bib3.txt 3 &
	./server Firenze bibData/bib4.txt 3 &
	./server Lucca bibData/bib5.txt 3 &
	sleep 1
	./client --autore="ciccio" -p &
	./client --autore="ciccio" --titolo="manuale" -p &
	./client --autore="melis" -p &
	./client --autore="ciccio" --titolo="manuale" &
	./client --autore="ciccio" --titolo="manuale" --anno="2010" -p &
	sleep 1
	./client --autore="kernighan" -p &
	./client --autore="kernighan" --titolo="manuale" -p &
	./client --autore="aho" -p &
	./client --autore="stevens" --titolo="unix" -p &
	./client --autore="kernighan" --titolo="manuale" --anno="2010" &
	sleep 1
	./client --autore="simenon" -p &
	./client --autore="dick" -p &
	./client --autore="aho" -p &
	./client --autore="stevens" --titolo="unix" -p &
	./client --autore="kernighan" --titolo="manuale" --anno="2010" &
	sleep 1
	./client --autore="kernighan" -p &
	./client --autore="kernighan" --titolo="manuale" -p &
	./client --autore="aho" -p &
	./client --autore="stevens" --titolo="unix" -p &
	./client --autore="kernighan" --titolo="manuale" --anno="2010" &
	sleep 1
	./client --titolo="linguaggio" -p &
	./client --luogo_pubblicazione="Reading" &
	./client --nota="Edizione italiana" &
	./client --autore="Taibo" --titolo="Dias" &
	./client --autore="Knuth" -p &
	sleep 1
	./client --titolo="Hijacking" -p &
	./client --editore="Addison" &
	./client --anno="1996" -p &
	./client --autore="luccio" &
	./client --autore="Geronimo Stilton" &
	sleep 1
	./client --nota="divertente" &
	./client --scaffale="A.west.2" -p &
	./client --luogo_pubblicazione="Paris" &
	./client --autore="Marco Briglia" &
	./client --titolo="Mathematical" -p &
	sleep 1
	pkill server
	sleep 5
	chmod +x ./bibaccess.sh
	./bibaccess.sh --query Firenze.log Pisa.log Massa.log Siena.log Lucca.log
	sleep 1
	./bibaccess.sh --loan Firenze.log Pisa.log Massa.log Siena.log Lucca.log

kill:
	pkill server
	

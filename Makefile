CC=gcc
CFLAGS=-I.

# Définition des exécutables
all: jeu gestionnaire menu joueurH joueurR

jeu: Jeu.c
	$(CC) -o jeu Jeu.c $(CFLAGS)

gestionnaire: Gestionnaire.c
	$(CC) -o gestionnaire Gestionnaire.c $(CFLAGS)

menu: Menu.c
	$(CC) -o menu Menu.c $(CFLAGS)

joueurH: joueurH.c
	$(CC) -o joueurH joueurH.c $(CFLAGS)

joueurR: joueurR.c
	$(CC) -o joueurR joueurR.c $(CFLAGS)

# Nettoyage
clean:
	rm -f jeu gestionnaire menu joueurH joueurR

# Exécuter le jeu
run: jeu
	./jeu


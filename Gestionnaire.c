#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <limits.h>
#include<time.h>
#include <pthread.h>
#define PORT 8020
int NOMBRE_DE_CARTES = 104;
//int NOMBRE_DE_JOUEURS = 2; // Changer à 4 si nécessaire
bool cartesDistribuees[104];
bool connecter = true;
static int manche =1;
static int finpartie=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    int valeur_numerique;
    int valeur_tetes_de_boeuf;
    int id_joueur; // Ajout de l'identifiant du joueur
} Carte;
typedef struct {
    Carte main[10];
    int score;
} Joueur;
typedef struct{
    Joueur joueur[2];
} Equipe;
typedef struct {
    Carte cartes[5];
    int nombre_cartes;
    int teteDeBoeufTotal;
} Rangee;

typedef struct {
    Equipe equipe[2];
    Joueur joueurs[4]; // Changer à 4 si nécessaire
    Rangee rangees[4]; // Changer à 4 si nécessaire
    int tour_actuel;
   int partie_terminee;
} Partie;
typedef struct {
    int socket;
    Partie *partie;
    int *socketsJoueurs;
    int joueursConnectes;
    int NOMBRE_DE_JOUEURS;
    int NOMBRE_DE_MANCHE;
    int SCORE;
} ThreadArgs;


void initialiserPartie(Partie *partie) {
    // Initialiser les joueurs, les rangées, etc.
    for (int i = 0; i < 4; ++i) {
        partie->joueurs[i].score = 0;
    }
    partie->tour_actuel = 0;
    partie->partie_terminee=0;
}

Carte *initialiserCarte() {
    Carte *paquet = malloc(NOMBRE_DE_CARTES * sizeof(Carte));
    for (int i = 0; i < NOMBRE_DE_CARTES; ++i) {
        paquet[i].valeur_numerique = i + 1;
        paquet[i].valeur_tetes_de_boeuf = (i % 7) + 1;
        paquet[i].id_joueur = -1; // Initialise l'identifiant du joueur à -1 (non attribué)
    }
    return paquet;
}

void initialiserCartesDistribuees() {
    for (int j = 0; j < NOMBRE_DE_CARTES; ++j) {
        cartesDistribuees[j] = false;
    }
}

void melangerCartes(Carte *paquet) {
    for (int i = NOMBRE_DE_CARTES - 1; i > 0; --i) {
        int j = rand() % (i + 1);

        // Échanger paquet[i] et paquet[j]
        Carte temp = paquet[i];
        paquet[i] = paquet[j];
        paquet[j] = temp;
    }
}

Carte *distribuerCartes(int joueur, Carte *paquet) {
    Carte *paquetj = malloc(10 * sizeof(Carte));

    for (int j = 0; j < 10; ++j) {
        // Trouver un indice non distribué
        int indice_carte;
        do {
            indice_carte = rand() % NOMBRE_DE_CARTES;
        } while (cartesDistribuees[indice_carte]);

        // Marquer la carte comme distribuée pour ce joueur
        cartesDistribuees[indice_carte] = true;

        // Distribuer la carte avec l'identifiant du joueur
        paquet[indice_carte].id_joueur = joueur;
        paquetj[j] = paquet[indice_carte];
    }

    return paquetj;
}
void viderrangee(Partie * partie, int indice)
{
    for(int i=0;i<5;i++)
    {
        partie->rangees[indice].cartes[i].valeur_numerique=0;
        partie->rangees[indice].cartes[i].valeur_tetes_de_boeuf=0;
        partie->rangees[indice].cartes[i].id_joueur=-1;
    }
    partie->rangees[indice].nombre_cartes=0;
    partie->rangees[indice].teteDeBoeufTotal=0;
}
void intitialiserRanger(Carte *paquet, Partie *partie) {

    for (int i = 0; i < 4; i++) {
    viderrangee(partie,i);
        int indice_carte;
        do {
            indice_carte = rand() % NOMBRE_DE_CARTES;
        } while (cartesDistribuees[indice_carte]);

        cartesDistribuees[indice_carte] = true;

        partie->rangees[i].cartes[0].valeur_numerique = paquet[indice_carte].valeur_numerique;
        partie->rangees[i].cartes[0].valeur_tetes_de_boeuf = paquet[indice_carte].valeur_tetes_de_boeuf;
        partie->rangees[i].nombre_cartes=1;
        partie->rangees[i].teteDeBoeufTotal=paquet[indice_carte].valeur_tetes_de_boeuf;

    }
}

void affichertable(Partie *partie) {
    for (int i = 0; i < 4; i++) {
        // Afficher le titre de la rangée en gras
        printf("\033[1mRangée %d (Têtes de bœuf: %d \U0001F42E)\033[0m\n", i, partie->rangees[i].teteDeBoeufTotal);

        for (int j = 0; j < partie->rangees[i].nombre_cartes; j++) {
            // Afficher les valeurs des cartes en gras
            printf("\033[1m %d || %d\033[0m\n", partie->rangees[i].cartes[j].valeur_numerique, partie->rangees[i].cartes[j].valeur_tetes_de_boeuf);
        }
        printf("\n");
    }
}

Carte *recupererpaquet(Carte paquet[]) {
    Carte *paquetG = malloc(10 * sizeof(Carte));
    int n = 0;
    for (int i = 0; i < 104; i++) {
        if (cartesDistribuees[i] == false) {
            paquetG[n].valeur_numerique = paquet[i].valeur_numerique;
            paquetG[n].valeur_tetes_de_boeuf = paquet[i].valeur_tetes_de_boeuf;
            n++;
        }
    }
    return paquetG;
}

void serializerpaquet(char *buffer, size_t buffersize, const Carte *c) {
    if (c == NULL || buffer == NULL || buffersize < sizeof(Carte)) {
        return;
    }
    memcpy(buffer, c, sizeof(Carte));
}

void envoyerPaquetAuJoueur(int socket, Carte paquet[]) {
    size_t buffer = sizeof(Carte);

    for (int i = 0; i < 10; ++i) {
        char *sb = (char *)malloc(buffer);
        serializerpaquet(sb, buffer, &paquet[i]);
        send(socket, sb, buffer, 0);
        free(sb);
        usleep(100000);
    }
}

void deserializecarte(char *buffer, size_t buffersize, Carte *c) {
    if (c == NULL || buffer == NULL || buffersize < sizeof(Carte)) {
        return;
    }
    memcpy(c, buffer, sizeof(Carte));
}

Carte *recevoirCarte(int socket[], Carte *carte,int nombrej) {
    char buffer[sizeof(Carte)];
    Carte *tabCarte = malloc(nombrej * sizeof(Carte));

    for (int i = 0; i < nombrej; ++i) {
        int bytesRead = recv(socket[i], buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            perror("Erreur lors de la réception des données");
        } else if (bytesRead == 0) {
            printf("Le joueur %d a fermé la connexion\n",i);
        } else {
            Carte c;
            deserializecarte(buffer, sizeof(buffer), &c);
            carte->valeur_numerique = c.valeur_numerique;
            carte->valeur_tetes_de_boeuf = c.valeur_tetes_de_boeuf;
            tabCarte[i].valeur_numerique = c.valeur_numerique;
            tabCarte[i].valeur_tetes_de_boeuf = c.valeur_tetes_de_boeuf;
            carte->id_joueur = c.id_joueur; // Met à jour l'identifiant du joueur
            tabCarte[i].id_joueur = c.id_joueur;
            printf("Carte reçue du joueur %d: %d || %d\n", carte->id_joueur, carte->valeur_numerique, carte->valeur_tetes_de_boeuf);
              for (int j = 0; j < nombrej; ++j) {
                if (j != i) {
                    send(socket[j], buffer, sizeof(buffer), 0);
                }
            }
        }
    }
    return tabCarte;
}
int obtenirIndiceRaneeAvecTeteDeBoeufMinimum(Partie *partie) {
    int teteDeBoeufMinimum = INT_MAX;  // Initialisation à une valeur maximale pour comparer
    int indiceRaneeMinimum = -1;

    for (int i = 0; i < 4; ++i) {
        if (partie->rangees[i].teteDeBoeufTotal < teteDeBoeufMinimum) {
            teteDeBoeufMinimum = partie->rangees[i].teteDeBoeufTotal;
            indiceRaneeMinimum = i;
        }
    }

    return indiceRaneeMinimum;
}
int obtenirTeteDeBoeufMinimum(Partie *partie) {
    int teteDeBoeufMinimum = INT_MAX;  // Initialisation à une valeur maximale pour comparer

    for (int i = 0; i < 4; ++i) {
        if (partie->rangees[i].teteDeBoeufTotal < teteDeBoeufMinimum) {
            teteDeBoeufMinimum = partie->rangees[i].teteDeBoeufTotal;
        }
    }

    return teteDeBoeufMinimum;
}
bool inferieur(Carte tab,Partie *partie)
{
   bool inf=false;
    int n=0;
    for(int i=0;i<4;i++)
    {
        if(tab.valeur_numerique<partie->rangees[i].cartes[partie->rangees[i].nombre_cartes-1].valeur_numerique)
        {
            n++;
        }
    }
    if(n==4)
    {
        inf=true;
    }
    return inf;
}
bool peutEtrePlaceeDansRangee(Carte *carte, Rangee *rangee) {
    // Vérifier si la carte peut être placée dans la rangée
    if (carte->valeur_numerique >= rangee->cartes[rangee->nombre_cartes - 1].valeur_numerique) {
        // Vérifier si la carte est inférieure à toutes les cartes de la rangée
        for (int j = 0; j < rangee->nombre_cartes; ++j) {
            if (carte->valeur_numerique <= rangee->cartes[j].valeur_numerique) {
                return false;
            }
        }
        // La carte peut être placée dans la rangée
        return true;
    }
    // La carte ne peut pas être placée dans la rangée
    return false;
}
bool peutEtrePlaceeDansUneRangee(Carte *carte, Partie *partie) {
    for (int i = 0; i < 4; ++i) {
        Rangee *rangee = &partie->rangees[i];
        if (peutEtrePlaceeDansRangee(carte, rangee)) {
            // La carte peut être placée dans au moins une rangée
            return true;
        }
    }
    // La carte ne peut être placée dans aucune rangée
    return false;
}

int obtenirIndiceRangeePlacable(Carte *carte, Partie *partie) {
    int indiceRangeePlacable = -1;

    for (int i = 0; i < 4; ++i) {
        Rangee *rangee = &partie->rangees[i];

        // Vérifier si la carte peut être placée dans la rangée
        if (peutEtrePlaceeDansRangee(carte, rangee)) {
            indiceRangeePlacable = i;
            break; // Sortir de la boucle dès que la première rangée est trouvée
        }
    }

    return indiceRangeePlacable;
}

void serializetable(char *buffer, size_t buffersize, const Rangee *rangee) {
    if (rangee == NULL || buffer == NULL || buffersize < sizeof(Rangee)) {
        return;
    }

    // Sérialiser les données de la rangée
    memcpy(buffer, rangee, sizeof(Rangee));
}

void envoyerTableAuxJoueurs(int *socketsJoueurs, Partie *partie,int nombrej) {
    // Taille totale du buffer nécessaire pour sérialiser une rangée
    size_t buffer_size = sizeof(Rangee);

    // Allouer le buffer
    char *buffer = malloc(buffer_size);

    // Parcourir tous les joueurs
    for (int i = 0; i < nombrej; ++i) {
        // Sérialiser et envoyer chaque rangée
        for (int j = 0; j < 4; ++j) {
            const Rangee *rangee = &(partie->rangees[j]);
            serializetable(buffer, buffer_size, rangee);
            send(socketsJoueurs[i], buffer, buffer_size, 0);
            usleep(100000); // Pause pour éviter la perte de paquets
        }
    }

    // Libérer le buffer après l'envoi à tous les joueurs
    free(buffer);
}
bool estRangeePleine(Rangee *rangee) {
    return rangee->nombre_cartes == 5;
}
void placerDansRangee(Partie *partie, Carte *carte, int socket) {
    // Initialiser la différence minimale à une valeur maximale
    int differenceMinimale = INT_MAX;
    int rangeeCible = -1;

    // Parcourir les rangées pour trouver la rangée cible
    for (int i = 0; i < 4; ++i) {
        Rangee *rangee = &partie->rangees[i];

        // Vérifier si la carte peut être placée dans la rangée
        if (carte->valeur_numerique >= rangee->cartes[rangee->nombre_cartes - 1].valeur_numerique) {
            // Vérifier si la carte est inférieure à toutes les cartes de la rangée
            bool peutEtrePlacee = true;
            for (int j = 0; j < rangee->nombre_cartes; ++j) {
                if (carte->valeur_numerique <= rangee->cartes[j].valeur_numerique) {
                    peutEtrePlacee = false;
                    break;
                }
            }

            // Si la carte peut être placée, mettre à jour la rangée cible si la différence est plus petite
            if (peutEtrePlacee) {
                int difference = carte->valeur_numerique - rangee->cartes[rangee->nombre_cartes - 1].valeur_numerique;

                // Mettre à jour la rangée cible si la différence est plus petite
                if (difference < differenceMinimale) {
                    differenceMinimale = difference;
                    rangeeCible = i;
                }
            }
        }
    }

    // Placer la carte dans la rangée cible
    if (rangeeCible != -1) {
        Rangee *rangeeCiblePtr = &partie->rangees[rangeeCible];

        // Ajouter la nouvelle carte à la rangée
        if (rangeeCiblePtr->nombre_cartes < 5) {
            // Si la rangée n'a pas encore atteint 5 cartes, ajouter la carte
            rangeeCiblePtr->cartes[rangeeCiblePtr->nombre_cartes] = *carte;
            rangeeCiblePtr->teteDeBoeufTotal += carte->valeur_tetes_de_boeuf;
            rangeeCiblePtr->nombre_cartes++;
            // Envoyer 0 comme tête de bœuf au joueur
            int teteDeBoeufMinimum = 0;
            // Envoyer des informations aux joueurs si nécessaire
            send(socket, &teteDeBoeufMinimum, sizeof(teteDeBoeufMinimum), 0);
        } else {
            // Si la rangée est pleine, stocker le teteDeBoeufTotal et vider la rangée
            int teteDeBoeufTotalAvantVidage = rangeeCiblePtr->teteDeBoeufTotal;
            viderrangee(partie, rangeeCible);
            // Placer la carte dans la première position de la rangée
            rangeeCiblePtr->cartes[0] = *carte;
            rangeeCiblePtr->teteDeBoeufTotal = carte->valeur_tetes_de_boeuf;
            rangeeCiblePtr->nombre_cartes = 1;
            partie->joueurs[carte->id_joueur].score+=teteDeBoeufTotalAvantVidage;
            // Envoyer le teteDeBoeufTotal stocké au joueur
            send(socket, &teteDeBoeufTotalAvantVidage, sizeof(teteDeBoeufTotalAvantVidage), 0);
        }

    } else {
        // Afficher un message si aucune rangée cible n'a été trouvée
        printf("La carte ne peut pas être placée dans aucune rangée.\n");
    }
}

bool pleinrangee(Partie * partie,Carte c)
{
    int indice = obtenirIndiceRangeePlacable(&c , partie);
    if(partie->rangees[indice].nombre_cartes==5)
    {
        return true;
    }
    return false;
}
void envoyerScore(int socket[], Carte tab[], Partie *partie,int nombrej)
{
    for (int i = 0; i < nombrej; i++)
    {
        if (inferieur(tab[i], partie))
        {
            int teteDeBoeufMinimum = obtenirTeteDeBoeufMinimum(partie);
            int indice = obtenirIndiceRaneeAvecTeteDeBoeufMinimum(partie);
            send(socket[tab[i].id_joueur], &teteDeBoeufMinimum, sizeof(teteDeBoeufMinimum), 0);
            partie->joueurs[tab[i].id_joueur].score += teteDeBoeufMinimum;
            viderrangee(partie, indice);
            partie->rangees[indice].cartes[0].valeur_numerique = tab[i].valeur_numerique;
            partie->rangees[indice].cartes[0].valeur_tetes_de_boeuf = tab[i].valeur_tetes_de_boeuf;
            partie->rangees[indice].cartes[0].id_joueur = tab[i].id_joueur;
            partie->rangees[indice].nombre_cartes += 1;
            partie->rangees[indice].teteDeBoeufTotal += tab[i].valeur_tetes_de_boeuf;
        }
        else
        {
            placerDansRangee(partie, &tab[i], socket[tab[i].id_joueur]);
        }
    }
    envoyerTableAuxJoueurs(socket, partie,nombrej);
}
void envoyerMessageATousLesJoueurs(int *socketsJoueurs, int nombreJoueurs, const char *message) {
    // Envoyer la taille du message d'abord
    size_t tailleMessage = strlen(message) + 1; // +1 pour inclure le caractère nul

    for (int i = 0; i < nombreJoueurs; ++i) {
        // Envoyer la taille du message
        send(socketsJoueurs[i], &tailleMessage, sizeof(size_t), 0);

        // Envoyer le message lui-même
        send(socketsJoueurs[i], message, tailleMessage, 0);
    }
}
void envoyerMessageAuJoueur(int socket, const char *message) {
    // Envoie la taille du message
    int tailleMessage = strlen(message);
    send(socket, &tailleMessage, sizeof(int), 0);

    // Envoie le message lui-même
    send(socket, message, tailleMessage, 0);
}

void trierCartesCroissant(Carte tableau[], int taille) {
    int i, j;
    Carte cle;

    for (i = 1; i < taille; i++) {
        cle = tableau[i];
        j = i - 1;

        while (j >= 0 && tableau[j].valeur_numerique > cle.valeur_numerique) {
            tableau[j + 1] = tableau[j];
            j = j - 1;
        }
        tableau[j + 1] = cle;
    }
}
int determinerJoueurGagnant(Partie *partie,int nombrej) {
    int joueurGagnant = 0;

    for (int i = 1; i < nombrej; i++) {
        if (partie->joueurs[i].score < partie->joueurs[joueurGagnant].score) {
            joueurGagnant = i;
        }
    }

    return joueurGagnant;
}

bool joueurAtteintScore(Partie *partie, int scoreCible,int nombrej) {
    for (int i = 0; i < nombrej; ++i) {
        if (partie->joueurs[i].score >= scoreCible) {
            return true;  // Le joueur a atteint ou dépassé le score cible
        }
    }
    return false;  // Aucun joueur n'a atteint le score cible
}
void finPartieAuJoueur(int *socket,int finpartie,int nombrej) {
     for(int i = 0; i < nombrej; i++) {
    send(socket[i], &finpartie, sizeof(int), 0);
}
}
void envoyernbrJoueurs(int *socket,int nombrej) {
     for(int i = 0; i < nombrej; i++) {
    send(socket[i], &nombrej, sizeof(int), 0);
}
}
void envoyernbrManche(int *socket,int nombrej,int Manche) {
     for(int i = 0; i < nombrej; i++) {
    send(socket[i], &Manche, sizeof(int), 0);
}
}
void envoyerScorepartie(int *socket,int nombrej,int Score) {
     for(int i = 0; i < nombrej; i++) {
    send(socket[i], &Score, sizeof(int), 0);
}
}
void envoyerscoregagnant(int* socketsJoueurs, Partie* partie, int scoreGagnant,int nombrej) {
    for (int i = 0; i < nombrej; i++) {
        // Vérifiez si le score du joueur est inférieur à 80
//        if (partie->joueurs[i].score < 80) {
//            // Envoyez le score gagnant au joueur
            if (send(socketsJoueurs[i], &scoreGagnant, sizeof(int), 0) == -1) {
                perror("Erreur lors de l'envoi du score gagnant");
                // Gérer l'erreur
            }
        //}
    }
}
void classementJoueurCroissant(Partie *partie, int nombrej)
{
    // Créer un tableau d'indices pour stocker l'ordre initial des joueurs
    int indices[nombrej];
    for (int i = 0; i < nombrej; i++)
    {
        indices[i] = i;
    }

    // Trier les indices en fonction des scores de manière croissante
    for (int i = 0; i < nombrej - 1; i++)
    {
        for (int j = i + 1; j < nombrej; j++)
        {
            if (partie->joueurs[indices[j]].score < partie->joueurs[indices[i]].score)
            {
                // Échanger les indices si le score de j est inférieur à celui de i
                int temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
    }

    // Afficher le classement croissant
    static int classementAffiche = 0; // Variable statique pour vérifier si le classement a déjà été affiché

    if (!classementAffiche)
    {
        printf("Classement Des Joueurs :\n");
        for (int i = 0; i < nombrej; i++)
        {
            if (i == 0)
            {
                printf("\033[1;32mJoueur %d avec un score de %d\033[0m\n", indices[i], partie->joueurs[indices[i]].score);
            }
            else
            {
                printf("\033[1;31mJoueur %d avec un score de %d\033[0m\n", indices[i], partie->joueurs[indices[i]].score);
            }
        }

        classementAffiche = 1; // Mettez à jour la variable pour indiquer que le classement a été affiché

    }
}

void *threadJoueur(void *arg) {
   ThreadArgs *args = (ThreadArgs *)arg;
    int joueurSocket = args->socket;
    Partie *partie = args->partie;
    int *socketsJoueurs = args->socketsJoueurs;
    int joueursConnectes = args->joueursConnectes;
    int NOMBRE_DE_JOUEURS= args->NOMBRE_DE_JOUEURS;
    int NOMBRE_DE_MANCHE=args->NOMBRE_DE_MANCHE;
    int SCORE =args->SCORE;
    // Logique spécifique au joueur à intégrer ici

    // Exemple de manipulation sécurisée des données partagées avec un mutex
    pthread_mutex_lock(&mutex);
    bool arreter = false;
    while (!arreter && manche<=NOMBRE_DE_MANCHE) {
        Carte *paquet = initialiserCarte();
        initialiserCartesDistribuees();
        melangerCartes(paquet);
        intitialiserRanger(paquet, partie);

        for (int i = 0; i < NOMBRE_DE_JOUEURS; i++) {
            Carte *paquetJoueur = distribuerCartes(i, paquet);
            envoyerPaquetAuJoueur(socketsJoueurs[i], paquetJoueur);
            free(paquetJoueur);
        }

        envoyerTableAuxJoueurs(socketsJoueurs, partie,NOMBRE_DE_JOUEURS);
        envoyerMessageATousLesJoueurs(socketsJoueurs, NOMBRE_DE_JOUEURS, "La partie va commencer maintenant !");

        affichertable(partie);
        for (int tour = 0; tour < 10; tour++) {
            Carte *c = malloc(sizeof(Carte));
            Carte *tab = recevoirCarte(socketsJoueurs, c,NOMBRE_DE_JOUEURS);
            free(c);

            for (int i = 0; i < NOMBRE_DE_JOUEURS; i++) {
                printf("%d || %d || %d\n", tab[i].valeur_numerique, tab[i].valeur_tetes_de_boeuf, tab[i].id_joueur);
            }

            trierCartesCroissant(tab, NOMBRE_DE_JOUEURS);
            printf("Les cartes après le trie : \n");
            for (int i = 0; i < NOMBRE_DE_JOUEURS; i++) {
                printf("%d || %d || %d\n", tab[i].valeur_numerique, tab[i].valeur_tetes_de_boeuf,tab[i].id_joueur);
            }

            envoyerScore(socketsJoueurs, tab, partie,NOMBRE_DE_JOUEURS);
            printf("\033[1;31mLa table de la manche %d tour %d\033[0m : \n", manche, tour+1);
            affichertable(partie);

            for (int i = 0; i < NOMBRE_DE_JOUEURS; i++) {
                printf("le score de joueur %d est %d \U0001F42E\n", i, partie->joueurs[i].score);
            }

            free(tab);

            if (tour < 9) {
                envoyerMessageATousLesJoueurs(socketsJoueurs, NOMBRE_DE_JOUEURS, "le tour suivant !");
            } else {
                int *scores = (int *)malloc(NOMBRE_DE_JOUEURS * sizeof(int));

                // Remplissage du tableau des scores
                for (int i = 0; i < NOMBRE_DE_JOUEURS; i++) {
                    scores[i] = partie->joueurs[i].score;
                }

                // Utilisation de snprintf pour créer la commande avec les valeurs des variables
                char command[1000];  // Ajustez la taille du tampon selon vos besoins
                char temp[50];       // Tampon temporaire pour stocker chaque score

                // Initialisation de la commande
                snprintf(command, sizeof(command), "./Statique.sh %d", manche);

                // Ajout des scores à la commande
                for (int i = 0; i < NOMBRE_DE_JOUEURS; i++) {
                    snprintf(temp, sizeof(temp), " %d", scores[i]);
                    strncat(command, temp, sizeof(command) - strlen(command) - 1);
                }

                // Exécution de la commande
                int statut=system(command);
                printf("STATUT : %d \n",statut);


                // Libération de la mémoire
                free(scores);
                envoyerMessageATousLesJoueurs(socketsJoueurs, NOMBRE_DE_JOUEURS, "La manche est terminée !");
            }
        }

        arreter = joueurAtteintScore(partie, SCORE,NOMBRE_DE_JOUEURS);

        if (!arreter) {
            manche++;
        }
        else
        {
        	finpartie=1;

        }
        finPartieAuJoueur(socketsJoueurs,finpartie,NOMBRE_DE_JOUEURS);
        if(finpartie==1 || manche> NOMBRE_DE_MANCHE)
        {

              // Déterminez le joueur gagnant
            int joueurGagnant = determinerJoueurGagnant(partie,NOMBRE_DE_JOUEURS);
            int scoreGagnant= partie->joueurs[joueurGagnant].score;
            envoyerscoregagnant(socketsJoueurs,partie,scoreGagnant,NOMBRE_DE_JOUEURS);

        }
        else{
            envoyerMessageATousLesJoueurs(socketsJoueurs,NOMBRE_DE_JOUEURS,"La manche suivante !!");
        }
    }
    pthread_mutex_unlock(&mutex);
      classementJoueurCroissant(partie,NOMBRE_DE_JOUEURS);
     printf("\033[1;35mFin De La Partie !! 🏁\033[0m\n");

    close(joueurSocket);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Veuillez spécifier NOMBRE_DE_JOUEURS en tant qu'argument de ligne de commande.\n");
        return 1;
    }

    int NOMBRE_DE_JOUEURS = atoi(argv[2]);
    int NOMBRE_DE_MANCHE = atoi(argv[3]);
    int SCORE = atoi(argv[4]);
    srand(time(NULL));
    int serveurSocket, nouveauSocket;
    struct sockaddr_in adresseServeur, adresseClient;
    socklen_t tailleAdresse = sizeof(adresseClient);

    // Créer le socket du serveur
    serveurSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serveurSocket == -1) {
        perror("Erreur lors de la création du socket du serveur");
        exit(EXIT_FAILURE);
    }

    adresseServeur.sin_family = AF_INET;
    adresseServeur.sin_addr.s_addr = INADDR_ANY;
    adresseServeur.sin_port = htons(PORT);

    // Lier le socket à une adresse et un port
    if (bind(serveurSocket, (struct sockaddr *)&adresseServeur, sizeof(adresseServeur)) < 0) {
        perror("Erreur lors de la liaison du socket du serveur");
        exit(EXIT_FAILURE);
    }

    // Attendre les connexions entrantes
    listen(serveurSocket, NOMBRE_DE_JOUEURS);
    printf("En attente de connexions...\n");

    int joueursConnectes = 0;
    int socketsJoueurs[NOMBRE_DE_JOUEURS];

    // Accepter les connexions jusqu'à ce que le nombre de joueurs atteigne NOMBRE_DE_JOUEURS
    while (joueursConnectes < NOMBRE_DE_JOUEURS) {
        // Accepter la connexion entrante
        nouveauSocket = accept(serveurSocket, (struct sockaddr *)&adresseClient, &tailleAdresse);
        if (nouveauSocket < 0) {
            perror("Erreur lors de l'acceptation de la connexion");
            exit(EXIT_FAILURE);
        }

        printf("Joueur N°%d connecté !\n", joueursConnectes);

        // Stocker le socket du joueur dans le tableau
        socketsJoueurs[joueursConnectes] = nouveauSocket;
        joueursConnectes++;
    }

    bool arreter = false;
    Partie *partie = malloc(sizeof(Partie));
    initialiserPartie(partie);
    envoyernbrJoueurs(socketsJoueurs,NOMBRE_DE_JOUEURS);
    envoyernbrManche(socketsJoueurs,NOMBRE_DE_JOUEURS,NOMBRE_DE_MANCHE);
    envoyerScorepartie(socketsJoueurs,NOMBRE_DE_JOUEURS,SCORE);
    while (!arreter && manche<=NOMBRE_DE_MANCHE) {

pthread_t thread;
ThreadArgs args;
args.socket = nouveauSocket;
args.partie = partie;
args.socketsJoueurs = socketsJoueurs;
args.joueursConnectes = joueursConnectes;
args.NOMBRE_DE_JOUEURS=NOMBRE_DE_JOUEURS;
args.NOMBRE_DE_MANCHE=NOMBRE_DE_MANCHE;
args.SCORE=SCORE;
pthread_create(&thread, NULL, threadJoueur, (void *)&args);
pthread_detach(thread);
    }
    free(partie);

    // Fermer la socket du serveur
    close(serveurSocket);

    return 0;
}

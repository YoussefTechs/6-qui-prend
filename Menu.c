#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int i = 0;

    while (1) {
        int choix;
          do {
            printf("Menu:\n");
            printf("1. Lancer un gestionnaire 🎲\n");
            printf("2. Joueur humain 👤\n");
            printf("3. Joueur robot 🤖\n");
            printf("4. Voir les statistiques 📊\n");
            printf("Faites votre choix (1, 2, 3 ou 4) : \n");
            // Vérifier si l'entrée est un entier
            if (scanf("%d", &choix) != 1) {
                // Si l'entrée n'est pas un entier, nettoyer le tampon d'entrée
                while (getchar() != '\n');
                printf("Veuillez entrer un choix valide.\n");
                choix = -1; // Réinitialiser choix à une valeur invalide
            }
        } while (choix < 1 || choix > 4);

        switch (choix) {
            case 1:
                printf("Vous avez choisi de lancer un gestionnaire.\n");
                // Récupérer l'adresse IP du gestionnaire
                char adresseIP[16];
                printf("Entrez l'adresse IP du gestionnaire : \n");
                scanf("%s", adresseIP);

                int nombreDeJoueurs;  // Correction de la variable
                printf("Entrer le nombre de joueurs pour cette partie : ");
                scanf("%d", &nombreDeJoueurs);  // Correction de la saisie
                int nombreDeManche;
                 printf("Entrer le nombre de manche pour cette partie : ");
                scanf("%d", &nombreDeManche);
                int score;
                 printf("Entrer le score max pour cette partie : ");
                scanf("%d", &score);
                // Compile le fichier gestionnaire.c
                system("gcc -o gestionnaire Gestionnaire.c");
                system("clear");

                char commande[100];
                snprintf(commande, sizeof(commande), "./gestionnaire %s %d %d %d", adresseIP, nombreDeJoueurs,nombreDeManche,score);  // Correction ici
                system(commande);
                break;


            case 2:
                printf("Vous avez choisi un joueur humain.\n");

                // Récupérer l'adresse IP du gestionnaire pour la passer au joueur humain
                char adresseIPJoueur[16];
                printf("Entrez l'adresse IP du gestionnaire : ");
                scanf("%s", adresseIPJoueur);

                // Compile le fichier joueurH.c
                system("gcc -o joueurh joueurH.c");
                system("clear");
                snprintf(commande, sizeof(commande), "./joueurh %s", adresseIPJoueur);
                system(commande);
                break;

            case 3:
                printf("Vous avez choisi un joueur robot.\n");

                // Récupérer l'adresse IP du gestionnaire pour la passer au joueur robot
                char adresseIPRobot[16];
                printf("Entrez l'adresse IP du gestionnaire : ");
                scanf("%s", adresseIPRobot);

                // Compile le fichier joueurR.c
                system("gcc -o joueurR joueurR.c");
                system("clear");
                snprintf(commande, sizeof(commande), "./joueurR %s", adresseIPRobot);
                system(commande);
                break;
            case 4:
                system("xdg-open scores.txt");
                break;
            default:
                printf("Choix invalide.\n");
                break;
        }
    }

    return 0;
}

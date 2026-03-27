#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void afficherMessageCentree(const char *message, int largeurTerminal) {
    int longueurMessage = strlen(message);

    // Calculer la position de début pour centrer le message
    int debut = (largeurTerminal - longueurMessage) / 2;
    char *couleurs[] = {
        "\033[1;31m", // Rouge
        "\033[1;32m", // Vert
        "\033[1;33m", // Jaune
        "\033[1;34m", // Bleu
        "\033[1;35m", // Magenta
        "\033[1;36m", // Cyan
    };

    int nombreCouleurs = sizeof(couleurs) / sizeof(couleurs[0]);
    // Afficher des espaces pour centrer le message
    for (int i = 0; i < debut; i++) {
        printf(" ");
    }
    // Afficher le message centré avec chaque caractère dans une couleur différente
    for (int i = 0; i < longueurMessage; i++) {
        int couleurIndex = i % nombreCouleurs;
       printf("%s%c", couleurs[couleurIndex], message[i]);
    }

    printf("\033[0m \U0001F42E\n"); // Réinitialiser la couleur et la taille du texte à la normale
}

void attendreEntree() {
    printf("Appuyez sur Entrer pour continuer...");
    fflush(stdout); // Assurer que le message est affiché immédiatement

    // Attendre l'appui sur Entrée
    while (getchar() != '\n');
}
int main() {
    const char *messageBienvenue = "Bienvenue sur le jeu 6 qui prend";
    int largeurTerminal = 80;
    afficherMessageCentree(messageBienvenue, largeurTerminal);
    attendreEntree();
    system("gcc -o menu Menu.c");
    system("clear");
    // Lancer le programme "menu"
    system("./menu");

    return 0;
}


#!/bin/bash

# Nom du fichier pour stocker les scores
score_file="scores.txt"
chemin_courant=$(pwd)
pdf_output="$chemin_courant/resultats_partie.pdf"

# Vérifier si le fichier existe, sinon le créer
if [ ! -e "$score_file" ]; then
    touch "$score_file"
    chmod 644 "$score_file"
fi

# Vérifier le nombre d'arguments
if [ "$#" -lt 2 ]; then
    echo "Usage: $0 <num_manche> <score_joueur1> <score_joueur2> ..."
    exit 1
fi

# Récupérer la première valeur, qui correspond à "manche"
manche="$1"

# Afficher le numéro de la manche
echo "Score de la Manche $manche: " >> "$score_file"

# Afficher chaque joueur avec son score et écrire dans le fichier
for ((i = 2; i <= $#; i++)); do
    joueur=$((i - 2))
    score="${!i}"

    # Vérifier si le score est un nombre entier
    if ! [[ "$score" =~ ^[0-9]+$ ]]; then
        echo "Erreur : Le score du joueur $joueur n'est pas un nombre entier."
        exit 1
    fi

    echo "Joueur $joueur : $score" >> "$score_file"
done

echo "Scores mis à jour dans $score_file"

# Génération du document PDF
pdflatex -output-directory=/tmp > /dev/null 2>&1 <<EOF
\documentclass{article}
\usepackage{graphicx}

\begin{document}

\section*{Statistiques de la Partie}

% Insérer ici les statistiques que vous souhaitez inclure dans le document PDF
\begin{verbatim}
$(cat $score_file)
\end{verbatim}

\end{document}
EOF

# Déplacer le document PDF généré vers le répertoire actuel

mv /tmp/texput.pdf "$pdf_output"

echo "Document PDF généré "


# Notre décodeur JPEG à nous

Bienvenue sur la page d'accueil de _votre_ projet JPEG, un grand espace de liberté, sous le regard bienveillant de vos enseignants préférés.
Le sujet sera disponible dès le lundi 5 mai à l'adresse suivante : [https://formationc.pages.ensimag.fr/projet/jpeg/jpeg/](https://formationc.pages.ensimag.fr/projet/jpeg/jpeg/).

Comme indiqué lors de l'amphi de présentation, vous devrez organiser un point d'étape avec vos enseignants pour valider cette architecture logicielle.
Cette page d'accueil servira de base à cette discussion. En pratique, vous pouvez reprendre son contenu comme bon vous semble, mais elle devra au moins comporter les infos suivantes :

1. des informations sur le découpage des fonctionnalités du projet en modules, en spécifiant les données en entrée et sortie de chaque étape ;
2. (au moins) un dessin des structures de données de votre projet (format libre, ça peut être une photo d'un dessin manuscrit par exemple) ;
3. une répartition des tâches au sein de votre équipe de développement, comportant une estimation du temps consacré à chacune d'elle (là encore, format libre, du truc cracra fait à la main, au joli Gantt chart).

Rajouter **régulièrement** des informations sur l'avancement de votre projet est aussi **une très bonne idée** (prendre 10 min tous les trois chaque matin pour résumer ce qui a été fait la veille, établir un plan d'action pour la journée qui commence et reporter tout ça ici, par exemple).

# Planning

**Planning prévisionnel :**  

| Version | Nom de code   | Caractéristiques                                         | Temps estimé |
|:-------:|:--------------|:---------------------------------------------------------|:-------------|
| 1       | Invader       | Décodeur d'images 8x8 en niveaux de gris                 | J+ 4         |
| 2       | Noir et blanc | Extension à des images grises comportant plusieurs blocs | J + 6        |
| 3       | Couleur       | Extension à des images en couleur                        | J+8          |
| 4       | Sous-ech      | Extension avec des images avec sous-échantionnage        | J +10        |

**Planning réel :**  

| Version | Nom de code   | Caractéristiques                                         				   	  | Temps		 |
|:-------:|:--------------|:------------------------------------------------------------------------------|:-------------|
| 1       | Invader       | Décodeur d'images 8x8 en niveaux de gris                 					  | J+2          |
| 2       | Noir et blanc | Extension à des images grises comportant plusieurs blocs 					  | J+2          |
| 3       | Couleur       | Extension à des images en couleur                        					  | J+2          |
| 4       | Sous-ech      | Extension à des images avec sous-échantionnage        						  | J+3          |
| 5       | Spectral      | Extension à des images en mode progressif avec sélection spéctrale uniquement | J+6          |
| 6       | Progressif 	  | Extension à des images en mode progressif avec approximations successives     | J+8          |



# Organisation

**Modules principaux**

| Module      | Fonctionnalités                                                                                           | Répartition              |
|:-----------:|-----------------------------------------------------------------------------------------------------------|--------------------------|
| entete      | Parse l'entête du fichier et remplit `img`, la structure contenant toutes les informations sur le fichier | Albin                    |
| vld         | Décode les coefficients DC et AC d'un bloc                                                                | Gabriel, Albin           |
| iqzz        | Inverse la quantification et transforme le vecteur 1x64 en un tableau 8x8                                 | Philippe                 |
| idct        | Passage du domaine spectral au domaine spatial avec la transformée en cosinus discrète inverse            | Philippe                 |
| idct_op     | Implémentation de l'idct rapide                                                                           | Philippe                 |
| ycc2rgb     | Transforme les composantes Y, $C_b$, $C_r$ en R, G, B                                                     | Gabriel                  |
| baseline    | Décode complètement une image en mode baseline                                       					  | Gabriel, Philippe, Albin |
| progressive | Décode complètement une image en mode progressif                                        			      | Gabriel, Albin           |
| jpeg2ppm    | Fonction principale contenant le main, appelant les autres modules                                        | Gabriel, Philippe, Albin |


| Module      | Fonctions        	     | Entrées                                                                                                                                                                   | Sorties                                              |
|:-----------:|--------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------|
| entete      | decode_entete    	     | FILE *fichier <br> bool premier_passage <br> img_t *img                                                                                                                   | (img_t*) img                                         |
| vld         | decode_bloc_acdc 	     | bitstream_t *bs <br> img_t *img <br> huffman_tree_t *hdc <br> huffman_tree_t *hac <br> blocl16_t *sortie <br> int16_t *dc_prec <br> uint8_t *off <br> uint16_t *skip_bloc | blocl16_t *sortie (tableau contenant le bloc décodé) |
| vld         | correction_eob   	     | FILE *fichier <br> img_t *img <br> blocl16_t *sortie <br> uint64_t *resi 																								 | blocl16_t *sortie (tableau contenant le bloc corrigé)|
| iqzz        | iquant           	     | blocl16_t *entree <br> uint8_t s_start <br> uint8_t s_end <br> qtable_t *qtable                                                                                           | blocl16_t *entree (déquantification en place)        |
| iqzz        | izz              	     | blocl16_t *entree                                                                                                                                                         | (bloct16_t *) bloc de sortie alloué dans iquant      |
| idct        | idct             	     | bloct16_t *bloc_freq <br> float stockage_coef[8][8][8][8]                                                                                                                 | (bloctu8_t *) bloc de sortie alloué dans izz         |
| idct        | calc_coef        	     | float stockage_coef[8][8][8][8]                                                                                                                                           | float stockage_coef[8][8][8][8]                      |
| idct_opt    | idct_opt         	     | bloct16_t *mcu                                                                                                              											  	 | (bloctu8_t *) bloc de sortie alloué dans idct_opt    |
| ycc2rgb     | ycc2rgb_pixel    	     | uint8_t y <br> uint8_t cb <br> uint8_t cr <br> rgb_t *rgb                                                                                                                 | rgb_t *rgb                                           |
| baseline	  | decode_baseline_image    | FILE *infile <br> img_t *img                                                                                                              								 | $\emptyset$										 	|
| progressive | decode_progressive_image | FILE *infile <br> img_t *img                                                                                                              						  	  	 | $\emptyset$									 	 	|


**Modules annexes**

| Module        | Fonctionnalités                                                                                              | Répartition              |
|:-------------:|--------------------------------------------------------------------------------------------------------------|--------------------------|
| bitstream     | Permet de lire bit à bit le fichier pour décoder les DC et AC			 									   | Gabriel        		  |
| decoder_utils | Contient des fonctions utilisées pour les	modes baseline et progressif   									   | Gabriel                  |
| erreur   		| Contient les codes et la structure pour la gestion d'erreur												   | Philippe                 |
| file   		| Implémente le type abstrait file pour effectuer un parcours en largeur dans un arbre  					   | Gabriel, Albin           |
| img   		| Contient la structure `img` et les fonctions pour l'initialiser et la libérer							   	   | Albin                    |
| options   	| Permet la gestion d'options									 											   | Gabriel                  |
| timer   		| Permet d'initialiser, de démarrer	et d'arrêter un chronomètres								               | Gabriel, Philippe, Albin |
| utils   		| Contient des fonctions utilisées par les autres modules (affichage verbose, ouverture/fermeture de fichiers) | Gabriel, Philippe        |


**Tests**

| Module        | Fonctionnalités                                                    | Répartition         |
|:-------------:|--------------------------------------------------------------------|---------------------|
| entete_test   | Teste le décodage d'entêtes correctes ou contenant des erreurs	 | Philippe, Albin     |
| iqzz_test     | Teste les fonctions izz et iq du module iqzz			 			 | Philippe		 	   |
| idct_opt_test | Compare le résultat de l'idct rapide à celui de l'idct classique	 | Philippe  		   |
| vld_test      | Teste le décodage d'un bloc de DC et d'AC 			 			 | Gabriel		  	   |



## Utilisation 

Usage : ./jpeg2ppm [option] fichier
Option : 
  -v             --verbose              Affiche des informations suplémentaire durant l'exécution.
  -t             --timer                Affiche le temps d'exécution de chaque partie.
  -h             --help                 Affiche cette aide.
  -f             --no-fast-idct         N'utilise pas la fast IDCT.
                 --tables               Affiche les tables de Huffman et de quantification
  -o <fichier>   --outfile=<fichier>    Placer la sortie dans le fichier.



## Schéma des structures utilisées

### Structures des blocs

On utilise plusieurs structures pour stocker en mémoire les blocs de l'image :  
![alt text](./schemas/diagram_bloc.png)  
Ces structure stockent des `uint8_t`, et il existe les mêmes structures mais stockant des `uint16_t`.  

Ce schéma représente l'implémentation de la structure de donnée contenant toutes les informations sur l'image à décoder, renseignée par `entete.c`.  
![alt text](./schemas/diagram_img.png)  

Ce schéma décrit quant à lui les structures utilisées pour la gestion des erreurs et des options en ligne de commande.  
![alt text](./schemas/diagram_err_opt.png)  

Structure du code permettant le décodage des coefficients DC et AC, mode *baseline* uniquement
![alt text](./schemas/diagram_vld_baseline.svg)  

Structure du code permettant le décodage des coefficients DC et AC, mode *baseline* et *progressive*.
![alt text](./schemas/diagram_vld_full.svg)  

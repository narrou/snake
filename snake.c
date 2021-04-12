#include "snake.h"


// VARIABLES GLOBALES
cellule ** grille;
int nbLignes = 0;
int nbColonnes = 0;
int mode = 1; // mode lolilol
int pid_adv;
int id_joueur = 0;
int key = 123456;
int shm_id;
struct shmid_ds * buf;
sem_t *temp;
// Creation d'un snake de base
unSnake creerSnake() {

  unSnake snake;

  uneCellule * teteSnake = malloc(sizeof(uneCellule));
  uneCellule * queueSnake = malloc(sizeof(uneCellule));
  uneCellule * milieuSnake = malloc(sizeof(uneCellule));

  teteSnake->ligne = 1 + 4*id_joueur;
  teteSnake->colonne = 4;
  teteSnake->suiv = NULL;

  milieuSnake->ligne = 1 + 4*id_joueur;
  milieuSnake->colonne = 3;
  milieuSnake->suiv = teteSnake;

  queueSnake->ligne = 1 + 4*id_joueur;
  queueSnake->colonne = 2;
  queueSnake->suiv = milieuSnake;

  snake.teteSnake = teteSnake;
  snake.queueSnake = queueSnake;

  return snake;
}

// Redéfini la tête du snake aux coordonnées indiquées
// Vérifie si le snake mange quelque chose

void ajouterEnTete (unSnake * snake, int ligne, int colonne,int * aMange, int * fail) {
  uneCellule * nouvelleTete = malloc (sizeof(uneCellule));

  nouvelleTete->ligne = ligne;
  nouvelleTete->colonne = colonne;
  nouvelleTete->suiv = NULL;

  sem_wait(temp);
  snake->teteSnake->suiv = nouvelleTete;
  grille[(snake->teteSnake->ligne)][(snake->teteSnake->colonne)].affichage = CORP_SNAKE;
  grille[(snake->teteSnake->ligne)][(snake->teteSnake->colonne)].couleur = 1;
  snake->teteSnake = snake->teteSnake->suiv;
  sem_post(temp);

  // GESTION DES COLLISIONS 
  if ( mode) { 
    if( snake->teteSnake->ligne < 0) {
      snake->teteSnake->ligne = nbLignes - 1; 
    }

    else if( snake->teteSnake->ligne > nbLignes-1) {
      snake->teteSnake->ligne = 0; 
    }
    else if (snake->teteSnake->colonne < 0) {
      snake->teteSnake->colonne = nbColonnes - 1; 
    }
    else if ( snake->teteSnake->colonne > nbColonnes-1) {
      snake->teteSnake->colonne = 0; 
    }
    else if (grille[snake->teteSnake->ligne][snake->teteSnake->colonne].affichage == CORP_SNAKE) {
      *fail = 1;
    }
  }
  else {
    if( snake->teteSnake->ligne < 0 ||
	snake->teteSnake->ligne > nbLignes-1 ||
	snake->teteSnake->colonne < 0 ||
	snake->teteSnake->colonne > nbColonnes-1)
      *fail = 1;
  }


  if(!*fail) {
    sem_wait(temp);
    *aMange = (grille[snake->teteSnake->ligne][snake->teteSnake->colonne].affichage == DU_MANGER) ? 1 : 0;
    grille[snake->teteSnake->ligne][snake->teteSnake->colonne].couleur = 1;
    grille[snake->teteSnake->ligne][snake->teteSnake->colonne].affichage = TETE_SNAKE;
    sem_post(temp);
  }
}

// Supprime la queue du snake
void supprimerQueue(unSnake * snake) {
  uneCellule * auxi;

  auxi = snake->queueSnake;
  sem_wait(temp);
  grille[snake->queueSnake->ligne][snake->queueSnake->colonne].affichage = CASE_VIDE;
  grille[snake->queueSnake->ligne][snake->queueSnake->colonne].couleur = 0;
  snake->queueSnake = snake->queueSnake->suiv;
  sem_post(temp);
  free(auxi);
}

void initGrille() {
  int i , j = 0;
  for (i = 0; i<nbLignes;i++) {
    for (j=0;j<nbColonnes;j++) {
      sem_wait(temp);
      grille[i][j].affichage = CASE_VIDE;
      sem_post(temp);
    }
  }
}

void afficherMenu() {
  printf("Mon pid : %d\n", getpid());
  printf("Entrez le pid de votre adversaire : ");
  scanf("%d", &pid_adv);
}

void afficherGrille(unSnake snake) {
  int i , j = 0;
  for (i = 0; i<nbLignes;i++) {
    for (j=0;j<nbColonnes;j++) {
      attron(COLOR_PAIR(grille[i][j].couleur));
      printw("%c",grille[i][j].affichage);
      attroff(COLOR_PAIR(grille[i][j].couleur));

    }
  }
}

void gererEvenement(unSnake * snake, int touche, int * fail, uneDirection * direction,int * aMange) {
  if(direction->ligne == 0) { // Pour ne pas 'aller en arrière'
    if (touche == KEY_UP){ 
      direction->ligne = -1;
      direction->colonne = 0; // Pour ne pas aller en diagonale
    }
    if (touche == KEY_DOWN) {
      direction->ligne = 1;
      direction->colonne = 0;
    }
  }
  if (direction->colonne == 0) {
    if (touche == KEY_LEFT) {
      direction->colonne = -1;
      direction->ligne = 0;
    }
    if (touche == KEY_RIGHT) {
      direction->colonne = 1;
      direction->ligne = 0;
    }
  }

  ajouterEnTete(snake,snake->teteSnake->ligne + direction->ligne,snake->teteSnake->colonne + direction->colonne, aMange,fail);
  if(!*aMange)
    supprimerQueue(snake);
}

// genere une case de bouffe à des coordonnées aléatoires
void genererDuManger(cellule ** grille) {
  int ligne = 0;
  int colonne = 0;
  int done = 0;

  srand(time(NULL));
  while (!done){
    ligne = rand() % (nbLignes-1);
    colonne = rand() % (nbColonnes-1);
    if(grille[ligne][colonne].affichage == CASE_VIDE) {
      sem_wait(temp);
      grille[ligne][colonne].affichage = DU_MANGER;
      sem_post(temp);
      done = 1;
    }
  }
}

void printFail() {
  move(nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("    _/_/_/_/    _/_/    _/_/_/  _/   \n");
  move(1 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("   _/        _/    _/    _/    _/    \n");
  move(2 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("  _/_/_/    _/_/_/_/    _/    _/     \n");
  move(3 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw(" _/        _/    _/    _/    _/      \n");  
  move(4 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("_/        _/    _/  _/_/_/  _/_/_/_/ \n");
}

void envoieSignal(int pid) {
  
  kill(pid, SIGINT);
}

void client_signal(int signal){
  id_joueur ++;
  shm_id = shmget(key, 1024, 0);
  grille = (cellule **) shmat(shm_id, NULL, 0);
  lancer_partie();
}


int lancer_partie(){
  int i = 0;
  int touche = 0; // touche pressee par le joueur
  int fail = 0; // bool
  unSnake snake = creerSnake();
  uneDirection direction = {0,1};
  int aMange = 1; // bool
  int nbCasesMangees = 0;
  int delay = 0;
 

  getmaxyx(stdscr,nbLignes,nbColonnes);

  grille = malloc(nbLignes * sizeof(cellule *));
  for (i=0;i<nbLignes;i++) {
    grille[i] = malloc(nbColonnes*sizeof(cellule));
  }
  if (id_joueur == 0)
  {
    initGrille();
  }
  getchar();
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  cbreak();
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(0, COLOR_WHITE, COLOR_BLACK);
  // BOUCLE DE JEU
  sem_wait(temp);
  while (!fail) {
    if (aMange){
      genererDuManger(grille);
      nbCasesMangees ++;
      delay = 101 - nbCasesMangees;
      delay = (delay < 60) ? 60 : delay;
      timeout(delay); // On raffraichi toutes les 60 ms au max
    }
    afficherGrille(snake);
    touche = getch();
    gererEvenement(&snake,touche,&fail,&direction,&aMange);
    erase();
  }

  // Le joueur a fail
  timeout(5000);
  erase();
  printFail();
  getch();

  endwin();
  return 0;
}


int main (int argc, char * argv []) {
  getmaxyx(stdscr,nbLignes,nbColonnes);
  temp = sem_open("/tmp/mysem", O_CREAT, 0666, 1);
  if (temp == -1) {
    perror("Erreur pendant la création du sémaphore\n");
  }
  sem_unlink("/tmp/mysem");
  signal(SIGINT, client_signal);
  // INITIALISATIONS

  afficherMenu();
  
  shm_id = shmget(key+1, nbLignes*sizeof(cellule), IPC_CREAT | IPC_EXCL);
  if (shm_id == -1){
    perror("Erreur pendant la création de la mémoire partagée, suppression de la mémoire\n");
    shm_id = shmget(key+1, 1024, 0);
    shmctl(shm_id, IPC_RMID, buf);
  }
  shm_id = shmget(key+1, 1024, IPC_CREAT | IPC_EXCL);
  if (shm_id == -1) {
    perror("Erreur pendant la création");
  }
  
  grille = (cellule **) shmat(shm_id, NULL, 0);
  /*
  envoieSignal(pid_adv);
  lancer_partie();*/
}
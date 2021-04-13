#include "snake.h"


// VARIABLES GLOBALES
cellule (*grille)[100];
int nbLignes = 50;
int nbColonnes = 100;
int mode = 1; // mode lolilol
int pid_adv;
int id_joueur = 0;
int key = 123456;
int shm_id, shm_id2;
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
  //sem_wait(temp);
  grille[snake->queueSnake->ligne][snake->queueSnake->colonne].affichage = CASE_VIDE;
  grille[snake->queueSnake->ligne][snake->queueSnake->colonne].couleur = 0;
  snake->queueSnake = snake->queueSnake->suiv;
  //sem_post(temp);
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
void genererDuManger() {
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
  
  key_t key = ftok("/tmp/myshm", 'r');
  shm_id = shmget(key, nbLignes * nbColonnes * sizeof(cellule *), 0666);
  if (shm_id == -1){
    perror("Erreur recupération client\n");
    exit(1);
  }
  grille = shmat(shm_id, NULL, 0);
  if (grille == (void * ) -1){
    perror("Erreur shmat client\n");
    exit(1);
  }
  //affichage(shm_id);
  lancer_partie();
}

void timestampToRead(time_t rawtime){
    struct tm  ts;
    char       buf[80];
    ts = *localtime(&rawtime);
    //strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    strftime(buf, sizeof(buf), "%H:%M:%S ", &ts);
    printf("%s", buf);
}


void affichage(int shm_id){
  struct shmid_ds buf;
  shmctl(shm_id, IPC_STAT, &buf);
	printf("Des informations sur la mémoire partagée : ");
	printf ("\nThe USER ID = %d\n",
         buf.shm_perm.uid);
	printf ("The GROUP ID = %d\n",
	 buf.shm_perm.gid);
	printf ("The creator's ID = %d\n",
	 buf.shm_perm.cuid);
	printf ("The creator's group ID = %d\n",
	 buf.shm_perm.cgid);
	printf ("The operation permissions = 0%o\n",
	 buf.shm_perm.mode);
	printf ("The slot usage sequence\n");
	printf ("number = 0%x\n",
	 buf.shm_perm.__seq);
	printf ("The key= 0%x\n",
	 buf.shm_perm.__key);
	printf ("The segment size = %ld\n",
	 buf.shm_segsz);
	printf ("The pid of last shmop = %d\n",
	 buf.shm_lpid);
	printf ("The pid of creator = %d\n",
	 buf.shm_cpid);
	printf ("The current # attached = %ld\n",
	 buf.shm_nattch);
	printf("The last shmat time = ");
	timestampToRead(buf.shm_atime);
   printf("\n");
	printf("The last shmdt time = ");
   timestampToRead(buf.shm_dtime);
   printf("\n");
	printf("The last change time = ");
   timestampToRead(buf.shm_ctime);
   printf("\n");
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
  if (id_joueur == 0)
  {
    initGrille();
  }

  initscr();
  keypad(stdscr, TRUE);
  noecho();
  cbreak();
  start_color();
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(0, COLOR_WHITE, COLOR_BLACK);
  // BOUCLE DE JEU
  while (!fail) {
    if (aMange){
      genererDuManger();
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
  getchar();
  return 0;
}


int main (int argc, char * argv []) {
  system("resize -s 50 100");
  nbLignes = 50;
  nbColonnes = 100;
  sem_unlink("/mysem");
  temp = (sem_t *)sem_open("/mysem", O_CREAT, 0666, 1);
  if (temp == SEM_FAILED) {
    perror("Erreur pendant la création du sémaphore\n");
  }
  
  signal(SIGINT, client_signal);
  grille = malloc(nbLignes * nbColonnes * sizeof(cellule *));
  // INITIALISATIONS
  afficherMenu();
  key_t key = ftok("/tmp/myshm", 'r');
  shm_id = shmget(key, nbLignes * nbColonnes * sizeof(cellule *), 0666 | IPC_CREAT);

  if (shm_id == -1) {
    perror("Erreur pendant la création");
    exit(1);
  }
  grille = shmat(shm_id, NULL, 0);
  if (grille == (void *) -1){
    perror("Erreur shmat serveur\n");
    exit(1);
  }
  //affichage(shm_id);
  envoieSignal(pid_adv);
  lancer_partie();
}
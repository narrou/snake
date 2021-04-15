#include "snake.h"

/* 
    Nom : SNAKE 
    Auteur : Julien Domont, Axel Facqueur
    Github : https://github.com/narrou/snake
    Description : SNAKE multi-processus, pour plus d'informations voir README
*/


/* -------------------------------------------------------------------*/
/*                                                                    */
/*                         VARIABLES GLOBALES                         */
/*                                                                    */
/* -------------------------------------------------------------------*/    

/* grille est la mémoire partagée permettant de récupérer la grille de jeu pour tout les joueurs
   et avoir la même pour tous
*/
cellule (*grille)[100];

/* nbLignes et nbColonnes sont littéralement le nombre de lignes et colonnes que l'on voudra afficher
   à l'écran.
*/
int nbLignes = 50;
int nbColonnes = 100;

/* Le mode permet de choisr le mode de jeu 
   0 : Les bords de la fenêtre vous fera perdre
   1 : Les bords de la fenêtre ne vous tue pas
*/
int mode = 1;

/* mon_pid et pid_adv sont respectivement mon pid et le pid du joueur adverse, util pour l'envoi de signaux*/
int pid_adv, mon_pid;

/* id_joueur permet de savoir quel joueur execute le programme */
int id_joueur = 0;

/* id de la mémoire partagée*/
int shm_id;

/* buffer pour les informations de la mémoire partagée*/
struct shmid_ds * buf;

/* mon semaphore pour protéger l'écriture et la lecture de la mémoire partagée*/
sem_t *semaphore;

/* -------------------------------------------------------------------*/
/*                                                                    */
/*                              FONCTION                              */
/*                                                                    */
/* -------------------------------------------------------------------*/     

/* 
    fonction : void main()
    description : On crée un sémaphore, une mémoire partagée et des signaux puis on appelle afficherMenu() qui est bloquant 
    et attend un pid de l'utilisateur. Seulement celui qui a entré le pid effectuera la suite du code, c'est-à-dire l'envoi du
    signal au pid adverse pour lancer la partie de son côté et lancer_partie() de son côté
*/
void main () {
  system("resize -s 50 100"); //On resize la fenêtre du terminal pour être en cohérence avec nbLignes et nbColonnes
  int rep;

  struct sigaction s;
  s.sa_flags = SA_SIGINFO;
  s.sa_sigaction = client_signal;
  mon_pid = getpid();
  sigaction(SIGINT, &s, NULL); // FIN DU JEU
	sigaction(SIGUSR1, &s, NULL); // LANCEMENT DU JEU

  sem_unlink("/mysem");
  semaphore = (sem_t *)sem_open("/mysem", O_CREAT, 0666, 1);
  if (semaphore == SEM_FAILED) {
    perror("Erreur pendant la création du sémaphore\n");
  }
  grille = malloc(nbLignes * nbColonnes * sizeof(cellule *));

  key_t key = ftok("/myshm", 'r');
  shm_id = shmget(key, nbLignes * nbColonnes * sizeof(cellule *), 0666 | IPC_CREAT);
  if (shm_id == -1) {
    perror("Erreur pendant la création");
    exit(1);
  }

  afficherMenu();

  grille = shmat(shm_id, NULL, 0);
  if (grille == (void *) -1){
    perror("Erreur shmat serveur\n");
    exit(1);
  }

  kill(pid_adv, SIGUSR1);
  lancer_partie();
}

/*
    fonction : void timestampToRead(time_t rawtime)
    argument : rawtime est la structure time_t que l'on doit printf
    description : printf de rawtime, utilisé dans affichage
*/
void timestampToRead(time_t rawtime){
    struct tm  ts;
    char       buf[80];
    ts = *localtime(&rawtime);
    strftime(buf, sizeof(buf), "%H:%M:%S ", &ts);
    printf("%s", buf);
}

/*
    fonction : void affichage(int shm_id)
    argument : shm_id id de la mémoire partagée à afficher
    description : Affiche les informations du buffer de la mémoire partagée
*/
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



/* 
   fonction : unSnake creerSnake()
   return : une structure snake
   description : créer une structure snake et choisit sa place dans la grille,
   on le déplace arbitrairement de 4 ligne en bas pour chaque nouveau joueur
*/
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

/*   
    fonction : void ajouterEnTete(unSnake * snake, int ligne, int colonne,int * aMange, int * fail)
    arguments : snake, le snake du joueur actuel
    ligne, la ligne de l'entête 
    colonne, la colonne de l'entête
    aMange, boolean pour savoir s'il y a bien a manger
    fail, boolean pour savoir si on a perdu
    description : ajoute une tête au SNAKE
*/
void ajouterEnTete (unSnake * snake, int ligne, int colonne,int * aMange, int * fail) {
  uneCellule * nouvelleTete = malloc (sizeof(uneCellule));

  nouvelleTete->ligne = ligne;
  nouvelleTete->colonne = colonne;
  nouvelleTete->suiv = NULL;

  sem_wait(semaphore);
  snake->teteSnake->suiv = nouvelleTete;
  grille[(snake->teteSnake->ligne)][(snake->teteSnake->colonne)].affichage = CORP_SNAKE;
  if (id_joueur == 1)
      grille[snake->teteSnake->ligne][snake->teteSnake->colonne].couleur = 1;
    else
      grille[snake->teteSnake->ligne][snake->teteSnake->colonne].couleur = 0;
  snake->teteSnake = snake->teteSnake->suiv;
  sem_post(semaphore);

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
    sem_wait(semaphore);
    *aMange = (grille[snake->teteSnake->ligne][snake->teteSnake->colonne].affichage == DU_MANGER) ? 1 : 0;
    if (id_joueur == 1)
      grille[snake->teteSnake->ligne][snake->teteSnake->colonne].couleur = 1;
    else
      grille[snake->teteSnake->ligne][snake->teteSnake->colonne].couleur = 0;
    grille[snake->teteSnake->ligne][snake->teteSnake->colonne].affichage = TETE_SNAKE;
    sem_post(semaphore);
  }
}

/*
    fonction : void supprimerQueue(unSnake * snake)
    argument : snake, le snake du joueur
    description : Supprime la queue du snake a chaque mouvement où l'on ne mange pas
 */
void supprimerQueue(unSnake * snake) {
  uneCellule * auxi;

  auxi = snake->queueSnake;
  sem_wait(semaphore);
  grille[snake->queueSnake->ligne][snake->queueSnake->colonne].affichage = CASE_VIDE;
  grille[snake->queueSnake->ligne][snake->queueSnake->colonne].couleur = 0;
  snake->queueSnake = snake->queueSnake->suiv;
  sem_post(semaphore);
  free(auxi);
}

/*
    fonction : void initGrille()
    description : Initialise la grille de jeu de case vide
*/
void initGrille() {
  int i , j = 0;
  for (i = 0; i<nbLignes;i++) {
    for (j=0;j<nbColonnes;j++) {
      sem_wait(semaphore);
      grille[i][j].affichage = CASE_VIDE;
      sem_post(semaphore);
    }
  }
}

/*
    fonction : afficherMenu()
    description : Affiche le menu de départ demandant le pid de l'adversaire, bloquant jusqu'a l'entrée du pid
*/
void afficherMenu() {
  printf("Mon pid : %d\n", getpid());
  printf("Entrez le pid de votre adversaire : ");
  scanf("%d", &pid_adv);
}


/* 
  function : afficherGrille
  Description : On affiche tout les charactères de la mémoire partagée 'grille' sur l'écran.
  A chaque charactère, on récupère son attribut couleur pour l'afficher dans sa couleur grâce à attron et attroff et des paires de couleurs
  défini dans lancer_partie() 
*/
void afficherGrille() {
  int i , j = 0;
  for (i = 0; i<nbLignes;i++) {
    for (j=0;j<nbColonnes;j++) {
      attron(COLOR_PAIR(grille[i][j].couleur));
      printw("%c",grille[i][j].affichage);
      attroff(COLOR_PAIR(grille[i][j].couleur));

    }
  }
}

/*
    fonction : void gererEvenement(unSnake * snake, int touche, int * fail, uneDirection * direction,int * aMange)
    argument : snake, snake du joueur
    touche, la touche appuyé par le joueur
    fail, boolean pour savoir si on a perdu
    direction, direction du snake
    aMange, boolean pour savoir si on a manger
*/
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

/*
    fonction : void genererDuManger()
    description : genere aléatoirement sur la grille de la nourriture
*/
void genererDuManger() {
  int ligne = 0;
  int colonne = 0;
  int done = 0;

  srand(time(NULL));
  while (!done){
    ligne = rand() % (nbLignes-1);
    colonne = rand() % (nbColonnes-1);
    if(grille[ligne][colonne].affichage == CASE_VIDE) {
      sem_wait(semaphore);
      grille[ligne][colonne].affichage = DU_MANGER;
      sem_post(semaphore);
      done = 1;
    }
  }
}

/* 
    fonction : void printFail()
    description : Affiche le message "FAIL" sur l'écran
*/
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

/* 
    fonction : void printWin()
    description : Affiche le message "WIN" sur l'écran
*/
void printWin() {
  move(nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("    __          _______ _   _    \n");
  move(1 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("    \\ \\        / /_   _| \\ | |    \n");
  move(2 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("     \\ \\  /\\  / /  | | |  \\| |     \n");
  move(3 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("      \\ \\/  \\/ /   | | | . ` |      \n");  
  move(4 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("       \\  /\\  /   _| |_| |\\  | \n");
  move(5 + nbLignes/2 - 5/2,nbColonnes/2 - 37/2);
  printw("        \\/  \\/   |_____|_| \\_| \n");
}

/*
    fonction : client_signal(int signal, siginfo_t *info, void *data)
    argument : signal, SIGUSR1 ou SIGINT
    info, information sur le processus appelant
    data, NULL
    description : gere les signaux envoyés aux processus 
*/
void client_signal(int signal, siginfo_t *info, void *data){
  switch(signal){
		case SIGUSR1: 
      id_joueur ++;
      pid_adv =(int) info->si_pid;
      grille = shmat(shm_id, NULL, 0);
      if (grille == (void * ) -1){
        perror("Erreur shmat client\n");
        exit(1);
      }
      lancer_partie();
      break;
		case SIGINT:
      // Le joueur a win
      erase();
      printWin();
      timeout(5000);
      getch();
      shmdt(grille);
      endwin();
      exit(1);
      break;
		default: break;
	}
  
}

/*
    fonction : int lancer_partie()
    description : Lance la partie en iniialisant la fenêtre et la grille de jeu puis on boucle sur l'affichage de la grille en gétant les évènements (appuie sur
    les flèches par le joueurs).
*/
int lancer_partie(){
  int i = 0;
  int touche = 0; // touche pressee par le joueur
  int fail = 0; // bool pour savoir si le joueur a perdu
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
  afficherGrille(snake);
  // BOUCLE DE JEU
  
  while (!fail) {
    if (aMange){
      genererDuManger();
      nbCasesMangees ++;
      delay = 101 - nbCasesMangees;
      delay = (delay < 60) ? 60 : delay;
      timeout(delay); // On raffraichi toutes les 60 ms au max
    }
    afficherGrille();
    touche = getch();
    gererEvenement(&snake,touche,&fail,&direction,&aMange);
    erase();
  }

  // Le joueur a perdu
  
  erase();
  kill(pid_adv, SIGINT);
  printFail();
  timeout(5000);
  getch();
  shmdt(grille);
  sleep(1);
  shmctl(shm_id, IPC_RMID, buf);
  endwin();
  return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ncurses.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>	
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CORP_SNAKE '-'
#define TETE_SNAKE 'B'
#define DU_MANGER '*'
#define CASE_VIDE ' '

struct uneCellule {
  int ligne;
  int colonne;
  struct uneCellule * suiv;
};
typedef struct uneCellule uneCellule;

struct cellule {
  char affichage;
  int couleur;
};
typedef struct cellule cellule;


struct unSnake {
  uneCellule * teteSnake;
  uneCellule * queueSnake;
};
typedef struct unSnake unSnake;

struct uneDirection {
  int ligne;
  int colonne;
};
typedef struct uneDirection uneDirection;

unSnake creerSnake();
void ajouterEnTete (unSnake * snake, int ligne, int colonne,int * aMange, int * fail);
void supprimerQueue(unSnake * snake);
void initGrille();
void afficherMenu();
void afficherGrille();
void gererEvenement(unSnake * snake, int touche, int * fail, uneDirection * direction,int * aMange);
void genererDuManger();
void printFail();
void client_signal(int signal,  siginfo_t *info, void *data);
int lancer_partie();
void affichage(int shm_id);
void timestampToRead(time_t rawtime);
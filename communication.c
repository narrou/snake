#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>	
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>

int memoire_partagee_cree(size_t memory_size, shm_key) {
	int shm_id = shmget (shm_key, memory_size, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (shm_id < 0) {
		perror("Création de la mémoire partagée impossible");
		exit(0);
	}
	return shm_id;
}

void lancer_partie(int pid){

	key_t shm_key = 6166529;
	const int shm_size = 1024;
	int shm_id;
	char* shmaddr, *ptr;

	if (pid == 0) {
		int next[2];
		shm_id = shmget (shm_key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR);
		shmaddr = (char*) shmat (shm_id, 0, 0);

		ptr = shmaddr + sizeof(next);
		next[0] = sprintf(ptr, "test") + 1;
		ptr += next[0];
		next[1] = sprintf(ptr, "test2") + 1;
		ptr += next[1];
		sprintf(ptr, "test3");
		memcpy(shmaddr, &next, sizeof(next));
		printf("writer ended\n");

		sleep(1000);
		shmdt(shmaddr);
		shmctl(shm_id, IPC_RMID, 0);
	}
	else {
		
		char* shared_memory[3];
		int *p;
	
		shm_id = shmget (shm_key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR);
		shmaddr = (char*) shmat (shm_id, 0, 0);

		printf("Reader\n");
		p = (int *)shmaddr;
		ptr = shmaddr + sizeof(int) * 2;
		shared_memory[0] = ptr;
		ptr += *p++;
		shared_memory[1] = ptr;
		ptr += *p++;
		shared_memory[2] = ptr;
		ptr += *p++;
		printf("0:%s\n1:%s\n2:%s\n", shared_memory[0], shared_memory[1], shared_memory[2]);
		shmdt(shmaddr);

	}
}


void client_signal(int sig) {

	printf("Signal Recu\n");
	lancer_partie(1);
}

void main(){
	int sonPid;
	int monPid = getpid();

	signal(SIGINT, client_signal);
	printf("mon pid est : %d\n Entrez un pid :", getpid());
	scanf("%d", &sonPid);
	kill(sonPid, SIGINT);
	lancer_partie(0);
}




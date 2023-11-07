// user process
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>

struct pcb {
	int pid;
	int id;
	int state; // 0 = blocked, 1 = waiting, 2 = running, 3 = done
	int priority;
};

int main(int argc, char * argv[]) {

	key_t key1, key2, key3;
	
	// create semaphore set
        key1 = ftok("semkey", 0); //generate key using ftok
        int sem = semget(key1, 1, 0600 | IPC_CREAT); // generate semaphore and check for errors
        if (sem == -1) {
                perror("semget failed");
                exit(0);
        }

	struct sembuf semWait = {
    		.sem_num = 0,
		.sem_op = -1,
		.sem_flg = 0
	};
	if (semop(sem, &semWait, 1) == -1) { // take control of sem before oss
		perror("semop failed");
		exit(0);
	}

	const int j = atoi(argv[1]); // own process
	const int n = atoi(argv[2]); // number of processes

	struct pcb * processTable;
	// generate key and allocate shared memory for process table
        int pcbId;
	key2 = ftok("./oss.c", 0);
	if ((pcbId = shmget(key2, n * sizeof(struct pcb), IPC_CREAT | 0666)) == -1) {
                perror("shmget");
                exit(1);
        }
        processTable = (struct pcb *) shmat(pcbId, NULL, 0);
        if (processTable == (struct pcb *) (-1)) {
                perror("shmat ");
                exit(1);
        }

	processTable[j].pid = getpid(); // set pid variable in process table to own pid
        processTable[j].state = 2; // set state to running

	printf("Process %d doing some work\n", j);
	sleep(3); // simulate doing something

	processTable[j].state = 3; // set state to done

	struct sembuf semSignal = {
    		.sem_num = 0,
    		.sem_op = 1,
    		.sem_flg = 0
	};
	if (semop(sem, &semSignal, 1) == -1) { // give control back
                perror("semop failed");
                exit(0);
        }
	
	return 0;
}

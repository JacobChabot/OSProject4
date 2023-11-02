// user process
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>

struct mymsg {
        long mtype;
        char mtext[100];
};

struct pcb {
	int pid;
	int state; // 0 = not running, 1 = waiting, 2 = running
	int priority;
};

int main(int argc, char * argv[]) {
	
	const int i = atoi(argv[1]); // process
	const int n = atoi(argv[2]); // number of processes

	struct pcb * processTable;
	key_t key1, key2, key3;

	// generate key and allocate shared memory for process table
        int pcbId;
	key1 = ftok("./oss.c", 0);
	if ((pcbId = shmget(key1, n * sizeof(struct pcb), IPC_CREAT | 0666)) == -1) {
                perror("shmget");
                exit(1);
        }
        processTable = (struct pcb *) shmat(pcbId, NULL, 0);
        if (processTable == (struct pcb *) (-1)) {
                perror("shmat ");
                exit(1);
        }

	// create semaphore set
        key2 = ftok("./master.c", 0); //generate key using ftok
        int sem = semget(key2, 1, 0600 | IPC_CREAT); // generate semaphore and check for errors
        if (sem == -1) {
                perror("semget failed");
                exit(0);
        }

	struct mymsg rmessage;
        key3 = ftok("jacobchabot", 'S');
        int msgId;
        if ((msgId = msgget(key3, IPC_CREAT | 0666)) == -1) {
                perror("msgget");
                exit(1);
        }

	struct sembuf semWait = {
    		.sem_num = 0,
		.sem_op = -1,
		.sem_flg = 0
	};
	if (semop(sem, &semWait, 1) == -1) { // wait for control
		perror("semop failed");
		exit(0);
	}
	
	// receive message and print
        msgrcv(msgId, &rmessage, sizeof(rmessage), 1, 0);
	printf("Message is: %s\n", rmessage.mtext);

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

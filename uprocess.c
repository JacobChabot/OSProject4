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

int main() {
	
	// generate key and allocate shared memory for clock
        // clock[0] == seconds
        // clock[1] == nanoseconds
        key_t key = ftok("oss", 'C');
        /*
	int shmId;
        if ((shmId = shmget(key, 2 * sizeof(unsigned int), IPC_CREAT | 0666)) == -1) {
                perror("shmget ");
                exit(1);
        }
        unsigned int * clock = (unsigned int *) shmat(shmId, NULL, 0);
        if (clock == (unsigned int *)(-1)) {
                perror("shmat ");
                exit(1);
        } */

	// create semaphore set
        key = ftok("./master.c", 0); //generate key using ftok
        int sem = semget(key, 1, 0600 | IPC_CREAT); // generate semaphore and check for errors
        if (sem == -1) {
                perror("semget failed");
                exit(0);
        }

	struct mymsg rmessage;
        key = ftok("jacobchabot", 'S');
        int msgId;
        if ((msgId = msgget(key, IPC_CREAT | 0666)) == -1) {
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/sem.h>


// message queue struct
struct mymsg {
	long mtype;
	char mtext[100];
};

// clock struct
struct clock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

// process control block struct
struct pcb {
	int pid;
	int state;
	int priority;
};

int main() {
	printf("Main\n");
	
	// generate key and allocate shared memory for clock
	// clock[0] == seconds
	// clock[1] == nanoseconds
	key_t key;
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

	// create semaphore
	key = ftok("./master.c", 0); // create key using ftok
	int sem = semget(key, 1, 0600 | IPC_CREAT); // generate semaphore and check for errors 
	if (sem == -1) {
		perror("semget");
		exit(0);
	} 
	// initialize semaphore to 1
	// 1 = CS is available, 0 = CS is in use
	struct sembuf initSem = {
		.sem_num = 0,
		.sem_op = 1, // set to 1 
		.sem_flg = 0
	};
	if (semop(sem, &initSem, 1) == -1) {
		perror("semop failed");
		exit(0);
	}

	// generate message queue
	struct mymsg smessage;
	key = ftok("jacobchabot", 'S');
	int msgId;
	if ((msgId = msgget(key, IPC_CREAT | 0666)) == -1) {
		perror("msgget");
		exit(1);
	}
	smessage.mtype = 1;
	strcpy(smessage.mtext, "Hello child process");
	printf("Message sent: %s\n", smessage.mtext);
	if (msgsnd(msgId, &smessage, sizeof(smessage), 0) == -1) {
		perror("msgsnd");
		exit(1);
	}



	// fork
	pid_t pid = fork();
	if (pid == 0) {
		execl("./uprocess.out", "./uprocess.out", NULL);
		exit(1);
	}
	else
		wait();

	// free memory
	//shmctl(shmId, IPC_RMID, NULL);

	// delete semaphore
	semctl(sem, 0, IPC_RMID);
	return 0;
}

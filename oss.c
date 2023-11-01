#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/sem.h>

const int n = 18; // max number of processes

// message queue struct
struct mymsg {
	long mtype;
	char mtext[100];
};

// clock struct
struct timer {
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
	
	struct pcb * processTable; // initialize an array of pcb structs
	struct timer * clock; // initialize a variable of timer struct
	key_t key1, key2, key3, key4;

	// generate key and allocate shared memory for clock
	int clockId;
	key1 = ftok("clockkey", 0);
	if ((clockId = shmget(key1, sizeof(struct timer), IPC_CREAT | 0666)) == -1) {
		perror("shmget ");
		exit(1);
	}
	clock = (struct timer *) shmat(clockId, NULL, 0);
	if (clock == (void *)(-1)) {
		perror("shmat ");
		exit(1);
	}
	
	//generate key and allocate shared mememory for process table
	int pcbId;
	key2 = ftok("./oss.c", 0);
        if ((pcbId = shmget(key2, n * sizeof(struct pcb), IPC_CREAT | 0666)) == -1) {
                perror("shmget");
                exit(1);
        }
        processTable = (struct pcb *) shmat(pcbId, NULL, 0);
        if (clock == (void *)(-1)) {
                perror("shmat ");
                exit(1);
        }

	// create semaphore
	key3 = ftok("semkey", 0); // create key using ftok
	int sem = semget(key3, 1, 0600 | IPC_CREAT); // generate semaphore and check for errors 
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
	key4 = ftok("msgqkey", 'S');
	int msgId;
	if ((msgId = msgget(key4, IPC_CREAT | 0666)) == -1) {
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
		// set process tables entries
		processTable[0].pid = getpid();
        	processTable[0].state = 0;
        	processTable[0].priority = 0;
		execl("./uprocess.out", "./uprocess.out", NULL); // execute user process and terminate
		exit(1);
	}
	else
		wait();

	// free memory
	shmctl(clockId, IPC_RMID, NULL);
	shmctl(pcbId, IPC_RMID, NULL);

	// delete semaphore
	semctl(sem, 0, IPC_RMID);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/sem.h>
#include <time.h>
#include <stdbool.h>

int n = 18; // max number of processes

void help() {
	printf("help function\n");
	exit(1);
}

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
	int state; // 0 = not running, 1 = waiting, 2 = running
	int priority; // 0 = normal process, 1 = high priority process
};

int main(int argc, char * argv[]) {
	printf("Main\n");

	// case and switch to handle command line arguments
	char ch;
	while ((ch = getopt(argc, argv, "hn:t:")) != -1) {
		switch (ch) {
			case 'h':
				help();
				break;
			case 'n':				
				sscanf(optarg, "%d", &n);
				if ( n > 18) {
					fprintf(stderr, "\nMax limit of n reached.\n");
					exit(0);
				}
				continue;
			default:
				fprintf(stderr, "Unrecononized options!\n");
				break;
			}
	}
	
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
	
	// create process tables for up to n processes
	srand(time(NULL));
	int i;
	for (i = 0; i < n; i++) {
		processTable[i].state = 1; // waiting
		processTable[i].priority = (rand() % (2)); // randomly generate if the process will be a normal user process or a high priority process
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
	
	while (true) {
		// loop through the process table looking for a high priority process thats waiting
		bool nprocessExe = false;
		bool hprocessExe = false;
		for (i = 0; i < n; i++) {
			char temp[2];
                        char nTemp[2];
                        snprintf(temp, sizeof(temp), "%d", i);
                        snprintf(nTemp, sizeof(nTemp), "%d", n);
                        pid_t pid;

			if (processTable[i].priority == 1 && processTable[i].state == 1) {
				hprocessExe = true; 
				pid = fork();
				if (pid == 0) {
					execl("./uprocess.out", "./uprocess.out", temp, nTemp, NULL);
					exit(1);
				}
			}
		}


		// if high priority process was not executed, loop again only looking for a process thats waiting
		if (hprocessExe == false)
			for (i = 0; i < n; i++) {
                        	char temp[2];
                        	char nTemp[2];
                        	snprintf(temp, sizeof(temp), "%d", i);
                        	snprintf(nTemp, sizeof(nTemp), "%d", n);
                        	pid_t pid;

                        	if (processTable[i].state == 1) { // execute process thats waiting
                                	nprocessExe = true;
					pid = fork();
                                	if (pid == 0) {
                                        	execl("./uprocess.out", "./uprocess.out", temp, nTemp, NULL);
                                        	exit(1);
                                	}
                        	}
                	}

		sleep(1); // this allows the child process to take control of the semaphore before the parent

		// if a process was executed, wait on semaphore
		// no point in waiting if a process was not executed
		if (nprocessExe == true || hprocessExe == true) {
			struct sembuf semWait = {
    				.sem_num = 0,
				.sem_op = -1,
				.sem_flg = 0
			};
			if (semop(sem, &semWait, 1) == -1) { // wait for CS to be open
				perror("semop failed");
				exit(0);
			}
		}
	
	
	
	}
		
	// free memory
	shmctl(clockId, IPC_RMID, NULL);
	shmctl(pcbId, IPC_RMID, NULL);

	// delete semaphore
	semctl(sem, 0, IPC_RMID);
	return 0;
}

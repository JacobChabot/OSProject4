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
	printf("OSS Help Function\n");
	printf("-n <number of processes>(optional)");
	exit(1);
}

// process control block struct
struct pcb {
	int pid;
	int id;
	int state; // 0 = blocked, 1 = waiting, 2 = running, 3 = done
	int priority; // 0 = normal process, 1 = high priority process
};

void logFile(int id, int queue, char * text) {
	// open log file
	FILE * file;
	file = fopen("logfile.dat", "a");
	if (file == NULL)
		perror("logfile");
	
	// get time
	time_t currentTime;
	struct tm * timeInfo;
	char timeString[9];
	time(&currentTime);
	timeInfo = localtime(&currentTime);
	strftime(timeString, sizeof(timeString), "%H:%M:%S", timeInfo);
	
	fprintf(file, "%s process with PID %d in queue %d at time %s\n", text, id, queue, timeString);

	fclose(file);
}


int main(int argc, char * argv[]) {

	// case and switch to handle command line arguments
	char ch;
	while ((ch = getopt(argc, argv, "hn::")) != -1) {
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

	int hqCount = 0;
	int nqCount = 0;

	// create process tables for up to n processes
	srand(time(NULL));
	int i;
	for (i = 0; i < n; i++) {
		processTable[i].id = i;
		processTable[i].state = 1; // waiting
		processTable[i].priority = rand() % 2; // randomly generate if the process will be a normal user process (0) or a high priority process (1)
		if (processTable[i].priority == 0)
			nqCount++;
		else
			hqCount++;
	}
	
	int highQueue[hqCount];
	int normalQueue[nqCount];
	hqCount = 0;
	nqCount = 0;
	size_t sizeHQ = sizeof(highQueue) / sizeof(highQueue[0]);
        size_t sizeNQ = sizeof(normalQueue) / sizeof(normalQueue[0]);

	// loop through processes and assign processes to the correct queue
	// high priority or normal priority
	for (i = 0; i < n; i++) {
		if (processTable[i].priority == 1) {
			highQueue[hqCount] = processTable[i].id;
			hqCount++;
		}
		if (processTable[i].priority == 0) {
			normalQueue[nqCount] = processTable[i].id;
			nqCount++;
		}
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
	
	bool hqEmpty = false;
	bool processExe;
	char temp[10];
	char nTemp[10];
	char gen[] = "Generating";
	char dis[] = "Dispatching";

	// scheduling loop
	while (true) {
		processExe = false;
		sizeHQ = sizeof(highQueue) / sizeof(highQueue[0]);
		sizeNQ = sizeof(normalQueue) / sizeof(normalQueue[0]);

		// loop through higher priority queue
		for (i = 0; i < sizeHQ; i++) {
			
			if (processTable[highQueue[i]].state == 3) // if process is alredy done, continue to next process
					continue;
			
                        snprintf(temp, sizeof(temp), "%d", highQueue[i]);
                        snprintf(nTemp, sizeof(nTemp), "%d", n);
                        pid_t pid;
			
			if (processTable[highQueue[i]].state == 1 && processTable[highQueue[i]].state != 3) {
				processExe = true; // process is being executed
				logFile(processTable[highQueue[i]].id, 1, gen);
				pid = fork();
				if (pid == 0) {
					logFile(processTable[highQueue[i]].id, 1, dis);
					execl("./uprocess.out", "./uprocess.out", temp, nTemp, NULL);
					exit(1);
				}
				break;
			}
		}
		
		// if a higher priority process was not executed, start looping through normal queue
		if (processExe == false) {
			for (i = 0; i < sizeNQ; i++) {
			       if (processTable[normalQueue[i]].state == 3) // if current process is already done, continue to next loop
				       continue;
				
			       snprintf(temp, sizeof(temp), "%d", normalQueue[i]);
                               snprintf(nTemp, sizeof(nTemp), "%d", n);
                               pid_t pid;
				
			       if (processTable[normalQueue[i]].state == 1 && processTable[normalQueue[i]].state != 3) {
				       processExe = true;
				       logFile(processTable[normalQueue[i]].id, 0, gen);
				       pid = fork();
				       if (pid == 0) {
					       logFile(processTable[normalQueue[i]].id, 0, dis);
					       execl("./uprocess.out", "./uprocess.out", temp, nTemp, NULL);
					       exit(1);
				       }
				       break;
			       }
		       }
		}
		// open the semaphore for the child processes
		struct sembuf semSignal = {
    			.sem_num = 0,
    			.sem_op = 1,
    			.sem_flg = 0
		};
		if (semop(sem, &semSignal, 1) == -1) { // signal that the sem can be opened
                	perror("semop failed");
                	exit(0);
        	}

		sleep(2); // this allows the child process to take control of the semaphore before the parent

		// if a process was executed, wait on semaphore
		// no point in waiting if a process was not executed
		if (processExe == true) {
			struct sembuf semWait = {
    				.sem_num = 0,
				.sem_op = -1,
				.sem_flg = 0
			};
			if (semop(sem, &semWait, 1) == -1) { // wait for sem to be open
				perror("semop failed");
				exit(0);
			}
		}
	
		if (processExe == false) // if no process was executed, processes are all done so breaking loop
			break;	
	}

	// free memory
	shmctl(pcbId, IPC_RMID, NULL);

	// delete semaphore
	semctl(sem, 0, IPC_RMID);

	return 0;
}

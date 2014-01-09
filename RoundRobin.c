
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sch-helpers.h"

process processes[MAX_PROCESSES+1];    
int numberOfProcesses=0;          
process_queue readyQueue;
process_queue waitingQueue;

/*Making CPU an array which can hold four processes at a time*/
process *cpu[NUMBER_OF_PROCESSORS]; 
process *prevP;

/*Array to hold the processes so that they can be sorted before going in the reaadyQ*/
process *tempProccess[MAX_PROCESSES+1];    
int tempProccessSize = 0;

/*Array to hold the processs going from running to readyQ */
process *switchingProcess[4];    
int switchingProcessSize = 0;


double cpus = 0, turnAroundTime = 0, waitingTime = 0;
int contextSwitch = 0, quantum; 

/*My compare by PID method, this takes in a pointer of a process.
Replicated & modified compareByArrival from sch-helpers.c*/

int compareByPIDs(const void *aa, const void *bb) {
    process *a = *((process**) aa);
    process *b = *((process**) bb);
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    return 0;
}

int main(int argc, char *argv[]){

	/*Making sure the user provides a time quantum*/
	if(argc != 2){
		printf("Error, Time Quantum for Round Robin not specified.\n");
		exit(1);
	}

	/*Read from file and populate the array*/
	int status;
	while (status=readProcess(&processes[numberOfProcesses])){
      	   if(status == 1)  
		 numberOfProcesses ++;
	}

	/*Set the quantum to the integer passed by the user*/	
	quantum = atoi(argv[1]);

	/* sorting all the processes by arrival time */ 
	qsort(processes, numberOfProcesses, sizeof(process), compareByArrival);

	/*Set all the CPU's to null*/
	int z;
	for(z = 0; z < NUMBER_OF_PROCESSORS; z++){
		cpu[z] = NULL;	
	}
	
	/*Initialize the queses*/
	initializeProcessQueue(&readyQueue);
	initializeProcessQueue(&waitingQueue);
	
	
	int clock;
	int totalProcesses = numberOfProcesses;
	
	for (clock = 0;  totalProcesses > 0; clock++){
	
		int i;
	
		/*Populating the temp array with process that are suppose
		to enter the readyQ based on their arrival time*/
		for(i = 0; i < numberOfProcesses; i++){
			if(processes[i].arrivalTime == clock){
				tempProccess[tempProccessSize] = &processes[i];
				tempProccessSize++;
			}
		}
		
		
		/*Pupulate the CPU's if they are empty*/
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){		

		/*If CPU is not empty*/			
		if (cpu[i] != NULL){
				/*If the Current CPU burst is ending*/
				if(cpu[i]->bursts[cpu[i]->currentBurst].step == cpu[i]->bursts[cpu[i]->currentBurst].length){
					cpu[i]->currentBurst++; 
					if(cpu[i]->currentBurst < cpu[i]->numberOfBursts)
					{					
						/*If there are bursts left, that means for sure the next burst is I/O, put it in the  waiting queue
						 Incrememnt the burst, throw the process into waiting queue, and take it out of the cpu*/
						enqueueProcess(&waitingQueue, cpu[i]);
					}
					else 
					{
					/*The process has no burst left, so its finished, now remove it from the cpu, readyQueue and WaitingQueue*/
					prevP = cpu[i];
					prevP->endTime = clock;
					totalProcesses--;
					}
					cpu[i] = NULL;
				}
				else{

				/*Increment the CPU burst step and the cpu utilization counter.*/
				/*Check if the quantum is equal to 0, if so this means the time slice has expired*/
					
					if(cpu[i]->quantumRemaining == 0){
						
						/*Put the Processes in a temporary array, sort them later, 
						these would be the first one to go back into the readyQueue*/
							switchingProcess[switchingProcessSize] = cpu[i];
							switchingProcessSize++;
							cpu[i] = NULL;
							contextSwitch++;
					}
					else{
					
					cpu[i]->bursts[cpu[i]->currentBurst].step++;
					cpu[i]->quantumRemaining--;
					cpus++;
					}
				}
			}
		}
		
		/*Go through every process in the waiting queue, if the I/O burst has completed, put it back in the ready queue, else increment the 
			step*/

		int size = waitingQueue.size;
		for(i = 0; i < size; i++){
			
		process *waitingProcess = waitingQueue.front->data;
		
		/*Dequeue process if it has finsihed it I/O burst, and put it in the ready queue*/	
		if(waitingProcess->bursts[waitingProcess->currentBurst].step  == waitingProcess->bursts[waitingProcess->currentBurst].length){
				
				/*increment the current burst, dequeue it and put it in the temp array to sort at the end. */
				waitingProcess->currentBurst++;
				dequeueProcess(&waitingQueue);
				tempProccess[tempProccessSize] = waitingProcess;
				tempProccessSize++;	
			}
			/*Increment the step, dequeue and then enque to get a new front.*/
			else{
				waitingProcess->bursts[waitingProcess->currentBurst].step++;
				dequeueProcess(&waitingQueue);
				enqueueProcess(&waitingQueue, waitingProcess);
			}
		}
			
			/*Sort both the process Arrays by PIDs*/
			qsort(tempProccess, tempProccessSize, sizeof(process*), compareByPIDs);
			qsort(switchingProcess, switchingProcessSize, sizeof(process*), compareByPIDs);
			
			/*Put the processes that are going from the running state to the ready state in the readyQ first*/
			for(i = 0; i < switchingProcessSize; i++)
				enqueueProcess(&readyQueue, switchingProcess[i]);
			switchingProcessSize = 0;


			/*Put the remaining processes ssorted by their pid's back in the ready queue*/
			for(i = 0; i < tempProccessSize; i++)
				enqueueProcess(&readyQueue, tempProccess[i]);
			tempProccessSize = 0;

			/*move from readyQueue to CPU if CPU is free */
			for(i = 0; i < NUMBER_OF_PROCESSORS && readyQueue.size > 0; i++)	{
				if(cpu[i] == NULL)
				{
					process *ready = readyQueue.front->data;
					dequeueProcess(&readyQueue);
					/*If this is the first time the process is coming in, set its start time*/
					if(ready->startTime == 0)
						ready->startTime = clock;
					ready->bursts[ready->currentBurst].step++;
					/*Set the quantum to the one specified by the user, we negate one because we consider this one step*/
					ready->quantumRemaining = quantum - 1;
					cpu[i] = ready;
					cpus++;
				}
			}
			
			/*Increment the waiting time for each process in the readyQueue
			using the same technique, accesing the front, dequeing and then 
			enqueing to move it to the back and get a new front*/	
			size = readyQueue.size;
			for(i = 0; i < size; i++){
					process *readyProcess = readyQueue.front->data;
					readyProcess->waitingTime++;
					dequeueProcess(&readyQueue);
					enqueueProcess(&readyQueue, readyProcess);
			}
	}

	/*Calcuate the waiting average waiting Time and Turnaround time*/
	int y;
	for(y = 0; y < numberOfProcesses; y++){
		waitingTime += processes[y].waitingTime;
		turnAroundTime += (processes[y].endTime - processes[y].arrivalTime);
	}
	
	/*Print out the final results*/
	printf("Average waiting time                 :  %.2f units\n", waitingTime/numberOfProcesses );
	printf("Average turnaround time              :  %.2f units\n", turnAroundTime / numberOfProcesses);
	printf("Time all processes finished          :  %d\n", prevP->endTime);
	printf("Average CPU utilization              :  %.1f\%\n", cpus/clock * 100);
	printf("Number of context switches           :  %d\n", contextSwitch);
	printf("PID(s) of last process(es) to finish :  %d\n", prevP->pid);

	return 0;
}


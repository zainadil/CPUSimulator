#include <stdio.h>
#include "sch-helpers.h"


process processes[MAX_PROCESSES+1];    
int numberOfProcesses=0;          
process_queue readyQueue;
process_queue waitingQueue;

/*Making CPU an array which can hold four processes at a time*/
process *cpu[NUMBER_OF_PROCESSORS]; 
process *prevP;

process *tempProccess[MAX_PROCESSES+1];    
int tempProccessSize = 0;
double cpus = 0, turnAroundTime = 0, waitingTime = 0;
int contextSwitch = 0; 

/*My compare by PID method, this takes in a pointer of a process.
Replicated & modified compareByArrival from sch-helpers.c*/
int compareByPIDs(const void *aa, const void *bb) {
    process *a = *((process**) aa);
    process *b = *((process**) bb);
    if (a->pid < b->pid) return -1;
    if (a->pid > b->pid) return 1;
    return 0;
}

int main(){

	/*Read from file and populate the array*/
	int status;
	while (status=readProcess(&processes[numberOfProcesses])){
      	   if(status == 1)  
		 numberOfProcesses ++;
	}
	
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
	
		/*Populate the readyQueue by arrival time*/
		for(i = 0; i < numberOfProcesses; i++){
			if(processes[i].arrivalTime == clock){
				tempProccess[tempProccessSize] = &processes[i];
				tempProccessSize++;
			}
		}
		
		
		/*Pupulate the CPU's if they are empty*/
		for(i = 0; i < NUMBER_OF_PROCESSORS; i++){		

		/*If PCU is not empty*/			
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
				cpu[i]->bursts[cpu[i]->currentBurst].step++;
				cpus++;
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
			
			/*
			Need to: Sort tempProccess with PID's and then enqueue 
			all of its elements into readyQueue for the CPU
			*/
			qsort(tempProccess, tempProccessSize, sizeof(process*), compareByPIDs);
			/*Put the processes sorted by their pid's back in the ready queue*/
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

	int y;
	for(y = 0; y < numberOfProcesses; y++){
		waitingTime = waitingTime + processes[y].waitingTime;
	}
	
	/*Calcualte turn around time*/
	for(y = 0; y < numberOfProcesses; y++){
		turnAroundTime = turnAroundTime + (processes[y].endTime - processes[y].arrivalTime);
	}
	
	printf("Average waiting time                 :  %.2f units\n", waitingTime/numberOfProcesses );
	printf("Average turnaround time              :  %.2f units\n", turnAroundTime / numberOfProcesses);
	printf("Time all processes finished          :  %d\n", prevP->endTime);
	printf("Average CPU utilization              :  %.1f\%\n", cpus/clock * 100);
	printf("Number of context switches           :  %d\n", contextSwitch);
	printf("PID(s) of last process(es) to finish :  %d\n", prevP->pid);

	return 0;
}



/*****************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 6/5/2017 to distribute to students to redo Lab 1                    *
* Updated 5/9/2017 for COMP 3500 labs                                         *
* Date  : February 20, 2009                                                   *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common2.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT,AWTJQ} Metric;

/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10 
#define FCFS            1 
#define RR              3 

#define OMAP            1
#define PAGING          2
#define BESTFIT         3
#define WORSFIT         4
int pageSize = 8192;
int NumberOfRequestedPages = 0;

#define MAXMETRICS      6
/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/

typedef struct MemoryManageTag
{
    int size;     // memory size
    int flag;     // A value of 0 means available, a value of 1 means occupied
    struct MemoryManageTag* prev;
    struct MemoryManageTag* next;
} MemoryManage,*pMemory;

/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/
Identifier			 memorypolicy;/*(1:OMAP) , (2:PAGING), (3:BESTFIT), (4:WORSFIT)*/
Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics

pMemory Head = NULL;
/*****************************************************************************\
*                               Function prototypes                           *
\*****************************************************************************/

void                 ManageProcesses(void);
void                 NewJobIn(ProcessControlBlock whichProcess);
void                 BookKeeping(void);
Flag                 ManagementInitialization(void);
void                 LongtermScheduler(void);
void                 IO();
void                 CPUScheduler(Identifier whichPolicy);
ProcessControlBlock *SRTF();
void                 Dispatcher();

int getMemoryPolicy(ProcessControlBlock *processPointer);
void releaseMemoryPolicy(ProcessControlBlock *processPointer);
int omap(ProcessControlBlock *processPointer);
int paging(ProcessControlBlock *processPointer);
int bestOrWorsFit(ProcessControlBlock *processPointer,int memoryPolicy);

/*****************************************************************************\
* function: main()                                                            *
* usage:    Create an artificial environment operating systems. The parent    *
*           process is the "Operating Systems" managing the processes using   *
*           the resources (CPU and Memory) of the system                      *
*******************************************************************************
* Inputs: ANSI flat C command line parameters                                 *
* Output: None                                                                *
*                                                                             *
* INITIALIZE PROGRAM ENVIRONMENT                                              *
* START CONTROL ROUTINE                                                       *
\*****************************************************************************/

int main (int argc, char **argv) {
   if (Initialization(argc,argv)){
     ManageProcesses();
   }
} /* end of main function */

/***********************************************************************\
* Input : none                                                          *
* Output: None                                                          *
* Function: Monitor Sources and process events (written by students)    *
\***********************************************************************/

void ManageProcesses(void){
	memorypolicy = PAGING
      ; //OMAP,PAGING,BESTFIT,WORSFIT
	ManagementInitialization();
	while (1) {
	IO();
	CPUScheduler(PolicyNumber);
	Dispatcher();
	}
}

/***********************************************************************\
* Input : none                                                          *          
* Output: None                                                          *        
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (RR) return to rReady Queue                         *                           
*    2) scan Waiting Queue to find processes with complete I/O          *
*           and move them to Ready Queue                                *         
\***********************************************************************/
void IO() {
  ProcessControlBlock *currentProcess = DequeueProcess(RUNNINGQUEUE); 
  if (currentProcess){
    if (currentProcess->RemainingCpuBurstTime <= 0) { // Finished current CPU Burst
      currentProcess->TimeEnterWaiting = Now(); // Record when entered the waiting queue
      EnqueueProcess(WAITINGQUEUE, currentProcess); // Move to Waiting Queue
      currentProcess->TimeIOBurstDone = Now() + currentProcess->IOBurstTime; // Record when IO completes
      currentProcess->state = WAITING;
    } else { // Must return to Ready Queue                
      currentProcess->JobStartTime = Now();                                               
      EnqueueProcess(READYQUEUE, currentProcess); // Mobe back to Ready Queue
      currentProcess->state = READY; // Update PCB state 
    }
  }

  /* Scan Waiting Queue to find processes that got IOs  complete*/
  ProcessControlBlock *ProcessToMove;
  /* Scan Waiting List to find processes that got complete IOs */
  ProcessToMove = DequeueProcess(WAITINGQUEUE);
  if (ProcessToMove){
    Identifier IDFirstProcess =ProcessToMove->ProcessID;
    EnqueueProcess(WAITINGQUEUE,ProcessToMove);
    ProcessToMove = DequeueProcess(WAITINGQUEUE);
    while (ProcessToMove){
      if (Now()>=ProcessToMove->TimeIOBurstDone){
	ProcessToMove->RemainingCpuBurstTime = ProcessToMove->CpuBurstTime;
	ProcessToMove->JobStartTime = Now();
	EnqueueProcess(READYQUEUE,ProcessToMove);
      } else {
	EnqueueProcess(WAITINGQUEUE,ProcessToMove);
      }
      if (ProcessToMove->ProcessID == IDFirstProcess){
	break;
      }
      ProcessToMove =DequeueProcess(WAITINGQUEUE);
    } // while (ProcessToMove)
  } // if (ProcessToMove)
}

/***********************************************************************\    
 * Input : whichPolicy (1:FCFS, 2: SRTF, and 3:RR)                      *        
 * Output: None                                                         * 
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock *selectedProcess;
  if ((whichPolicy == FCFS) || (whichPolicy == RR)) {
    selectedProcess = DequeueProcess(READYQUEUE);
  } else{ // Shortest Remaining Time First 
    selectedProcess = SRTF();
  }
  if (selectedProcess) {
    selectedProcess->state = RUNNING; // Process state becomes Running                                     
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue                         
  }
}

/***********************************************************************\                         
 * Input : None                                                         *                                     
 * Output: Pointer to the process with shortest remaining time (SRTF)   *                                     
 * Function: Returns process control block with SRTF                    *                                     
\***********************************************************************/
ProcessControlBlock *SRTF() {
  /* Select Process with Shortest Remaining Time*/
  ProcessControlBlock *selectedProcess, *currentProcess = DequeueProcess(READYQUEUE);
  selectedProcess = (ProcessControlBlock *) NULL;
  if (currentProcess){
    TimePeriod shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
    Identifier IDFirstProcess =currentProcess->ProcessID;
    EnqueueProcess(READYQUEUE,currentProcess);
    currentProcess = DequeueProcess(READYQUEUE);
    while (currentProcess){
      if (shortestRemainingTime >= (currentProcess->TotalJobDuration - currentProcess->TimeInCpu)){
	EnqueueProcess(READYQUEUE,selectedProcess);
	selectedProcess = currentProcess;
	shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
      } else {
	EnqueueProcess(READYQUEUE,currentProcess);
      }
      if (currentProcess->ProcessID == IDFirstProcess){
	break;
      }
      currentProcess =DequeueProcess(READYQUEUE);
    } // while (ProcessToMove)
  } // if (currentProcess)
  return(selectedProcess);
}

/***********************************************************************\  
 * Input : None                                                         *   
 * Output: None                                                         *   
 * Function:                                                            *
 *  1)If process in Running Queue needs computation, put it on CPU      *
 *              else move process from running queue to Exit Queue      *     
\***********************************************************************/
void Dispatcher() {
  double start;
  ProcessControlBlock *processOnCPU = Queues[RUNNINGQUEUE].Tail; // Pick Process on CPU
  if (!processOnCPU) { // No Process in Running Queue, i.e., on CPU
    return;
  }
  if(processOnCPU->TimeInCpu == 0.0) { // First time this process gets the CPU
    SumMetrics[RT] += Now()- processOnCPU->JobArrivalTime;
    NumberofJobs[RT]++;
    processOnCPU->StartCpuTime = Now(); // Set StartCpuTime
  }
  
  if (processOnCPU->TimeInCpu >= processOnCPU-> TotalJobDuration) { // Process Complete
    printf(" >>>>>Process # %d complete, %d Processes Completed So Far <<<<<<\n",
	   processOnCPU->ProcessID,NumberofJobs[THGT]);
    releaseMemoryPolicy(processOnCPU);
    processOnCPU=DequeueProcess(RUNNINGQUEUE);
    EnqueueProcess(EXITQUEUE,processOnCPU);

    NumberofJobs[THGT]++;
    NumberofJobs[TAT]++;
    NumberofJobs[WT]++;
    NumberofJobs[CBT]++;
    NumberofJobs[AWTJQ]++;
    SumMetrics[TAT]     += Now() - processOnCPU->JobArrivalTime;
    SumMetrics[WT]      += processOnCPU->TimeInReadyQueue;
    SumMetrics[AWTJQ]      += processOnCPU->TimeInJobQueue;

    // processOnCPU = DequeueProcess(EXITQUEUE);
    // XXX free(processOnCPU);

  } else { // Process still needs computing, out it on CPU
    TimePeriod CpuBurstTime = processOnCPU->CpuBurstTime;
    processOnCPU->TimeInReadyQueue += Now() - processOnCPU->JobStartTime;
    if (PolicyNumber == RR){
      CpuBurstTime = Quantum;
      if (processOnCPU->RemainingCpuBurstTime < Quantum)
	CpuBurstTime = processOnCPU->RemainingCpuBurstTime;
    }
    processOnCPU->RemainingCpuBurstTime -= CpuBurstTime;
    // SB_ 6/4 End Fixes RR 
    TimePeriod StartExecution = Now();
    OnCPU(processOnCPU, CpuBurstTime); // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTime
    processOnCPU->TimeInCpu += CpuBurstTime; // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTimeu
    SumMetrics[CBT] += CpuBurstTime;
  }
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: This routine is run when a job is added to the Job Queue    *
\***********************************************************************/
void NewJobIn(ProcessControlBlock whichProcess){
  ProcessControlBlock *NewProcess;
  /* Add Job to the Job Queue */
  NewProcess = (ProcessControlBlock *) malloc(sizeof(ProcessControlBlock));
  memcpy(NewProcess,&whichProcess,sizeof(whichProcess));
  NewProcess->TimeInCpu = 0; // Fixes TUX error
  NewProcess->RemainingCpuBurstTime = NewProcess->CpuBurstTime; // SB_ 6/4 Fixes RR
  EnqueueProcess(JOBQUEUE,NewProcess);
  DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
  LongtermScheduler(); /* Job Admission  */
}


/***********************************************************************\                                                   
* Input : None                                                         *                                                    
* Output: None                                                         *                                                    
* Function:                                                            *
* 1) BookKeeping is called automatically when 250 arrived              *
* 2) Computes and display metrics: average turnaround  time, throughput*
*     average response time, average waiting time in ready queue,      *
*     and CPU Utilization                                              *                                                     
\***********************************************************************/
void BookKeeping(void){
  char *memPolicy;
  double end = Now(); // Total time for all processes to arrive
  Metric m;

  // Compute averages and final results
  if (NumberofJobs[TAT] > 0){
    SumMetrics[TAT] = SumMetrics[TAT]/ (Average) NumberofJobs[TAT];
  }
  if (NumberofJobs[RT] > 0){
    SumMetrics[RT] = SumMetrics[RT]/ (Average) NumberofJobs[RT];
  }
  SumMetrics[CBT] = SumMetrics[CBT]/ Now();

  if (NumberofJobs[WT] > 0){
    SumMetrics[WT] = SumMetrics[WT]/ (Average) NumberofJobs[WT];
  }

  if (NumberofJobs[AWTJQ] > 0){
	  SumMetrics[AWTJQ] = SumMetrics[AWTJQ]/ (Average) NumberofJobs[AWTJQ];
   }

  if(1 == memorypolicy) memPolicy = "OMAP\0";
  else if(2 == memorypolicy) memPolicy = "PAGING\0";
  else if(3 == memorypolicy) memPolicy = "BESTFIT\0";
  else if(4 == memorypolicy) memPolicy = "WORSFIT\0";


  printf("\n********* Processes Managemenent Numbers ******************************\n");
  if(2 == memorypolicy)
	  printf("Policy Number = %d, Quantum = %.6f   Show = %d  Memory Policy = %s  pageSize = %d\n", PolicyNumber, Quantum, Show,memPolicy,pageSize);
  else
	  printf("Policy Number = %d, Quantum = %.6f   Show = %d  Memory Policy = %s\n", PolicyNumber, Quantum, Show,memPolicy);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f  AWTJQ=%f\n",
	 SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT], 
	 NumberofJobs[THGT]/Now(), SumMetrics[WT],SumMetrics[AWTJQ]);

  exit(0);
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: Decides which processes should be admitted in Ready Queue   *
*           If enough memory and within multiprogramming limit,         *
*           then move Process from Job Queue to Ready Queue             *
\***********************************************************************/
void LongtermScheduler(void){
	ProcessControlBlock *currentProcess = DequeueProcess(JOBQUEUE);

	while (currentProcess) {
		if(!getMemoryPolicy(currentProcess)) break;
		currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime; // Set TimeInJobQueue
		currentProcess->JobStartTime = Now(); // Set JobStartTime
		EnqueueProcess(READYQUEUE,currentProcess); // Place process in Ready Queue
		currentProcess->state = READY; // Update process state
		currentProcess = DequeueProcess(JOBQUEUE);

	}
}


/***********************************************************************\
* Input : None                                                          *
* Output: TRUE if Intialization successful                              *
\***********************************************************************/
Flag ManagementInitialization(void){
  Metric m;
  for (m = TAT; m < MAXMETRICS; m++){
     NumberofJobs[m] = 0;
     SumMetrics[m]   = 0.0;
  }
  return TRUE;
}

int getMemoryPolicy(ProcessControlBlock *processPointer){
	switch(memorypolicy){
		case OMAP:
			if(!omap(processPointer)) return 0;
			break;
		case PAGING:
				if(!paging(processPointer)) return 0;
				break;
		case BESTFIT:
		case WORSFIT:
			if(!bestOrWorsFit(processPointer,memorypolicy)) return 0;
			break;
	}
	return 1;
}

void releaseMemoryPolicy(ProcessControlBlock *processPointer){
	switch(memorypolicy){
		case OMAP:
			AvailableMemory += processPointer->MemoryRequested;
			break;
		case PAGING:
				NumberOfRequestedPages = (processPointer->MemoryRequested + (pageSize - 1) ) / pageSize;
				AvailableMemory += NumberOfRequestedPages * pageSize;
				break;
		case BESTFIT:
		case WORSFIT:
		{
			pMemory pCurrent = Head,pTemp;
			while(pCurrent != NULL){
				if((pCurrent->size == processPointer->MemoryRequested) && (1 == pCurrent->flag)){
					pCurrent->flag = 0;
					if((NULL != pCurrent->prev) && (0 == pCurrent->prev->flag)){
						pCurrent->prev->size += pCurrent->size;
						AvailableMemory += pCurrent->prev->size;
						pTemp = pCurrent->prev;
						pTemp->next = pCurrent->next;
						if(pCurrent->next)
							pCurrent->next->prev = pTemp;
						free(pCurrent);
					}else if((NULL != pCurrent->next) && (0 == pCurrent->next->flag)){
						pTemp = pCurrent->next;
						pCurrent->size += pCurrent->next->size;
						AvailableMemory += pCurrent->size;
						pCurrent->next = pCurrent->next->next;
						if(pCurrent->next)
							pCurrent->next->prev = pCurrent;

						free(pTemp);
					}else{
						AvailableMemory += pCurrent->size;
					}

				}
				pCurrent = pCurrent->next;
			}
		}
		break;
	}
}

int omap(ProcessControlBlock *processPointer){
	if(AvailableMemory >= processPointer->MemoryRequested){
		AvailableMemory -= processPointer->MemoryRequested;
	}
	else{
		EnqueueProcess(JOBQUEUE,processPointer);
		return 0;
	}
	return 1;
}

int paging(ProcessControlBlock *processPointer){
	NumberOfRequestedPages = (processPointer->MemoryRequested + (pageSize - 1) ) / pageSize;
	int NumberOfAvailablePages = (AvailableMemory + (pageSize - 1)) / pageSize;

	if(NumberOfAvailablePages >= NumberOfRequestedPages){
		AvailableMemory -= NumberOfRequestedPages * pageSize;
	}else{
		EnqueueProcess(JOBQUEUE,processPointer);
		return 0;
	}
	return 1;
}

int bestOrWorsFit(ProcessControlBlock *processPointer,int memoryPolicy){
	pMemory pNewNode,pCurrent,pMinOrMax = NULL;
	if(NULL == Head){
		if(AvailableMemory >= processPointer->MemoryRequested){
			Head = malloc(sizeof(MemoryManage));
			Head->size = processPointer->MemoryRequested;
			Head->flag = 1;
			Head->prev = NULL;
			Head->next = NULL;
			pNewNode = malloc(sizeof(MemoryManage));
			pNewNode->size = AvailableMemory - processPointer->MemoryRequested;
			pNewNode->flag = 0;
			Head->next = pNewNode;
			pNewNode->prev = Head;
			pNewNode->next = NULL;
		}else{
			EnqueueProcess(JOBQUEUE,processPointer);
			return 0;
		}
	}else{
		pCurrent = Head;
		while(pCurrent != NULL){
			if((pCurrent->size >= processPointer->MemoryRequested) && (0 == pCurrent->flag)){
				if(NULL == pMinOrMax){
					pMinOrMax = pCurrent;
				}else{
					if(pCurrent->size <= pMinOrMax->size){
						if(BESTFIT == memoryPolicy)
							pMinOrMax = pCurrent;
					}else{
						if(WORSFIT == memoryPolicy)
							pMinOrMax = pCurrent;
					}
				}
			}
			pCurrent = pCurrent->next;
		}

		if(NULL == pMinOrMax){
			EnqueueProcess(JOBQUEUE,processPointer);
			return 0;
		}
		pNewNode = malloc(sizeof(MemoryManage));
		pNewNode->size = pMinOrMax->size - processPointer->MemoryRequested;
		pNewNode->flag = 0;
		pMinOrMax->size = processPointer->MemoryRequested;
		pMinOrMax->flag = 1;

		pNewNode->next = Head;
		pNewNode->prev = NULL;
		Head = pNewNode;
	}

	return 1;
}


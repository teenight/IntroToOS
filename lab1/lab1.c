
/*****************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 5/22/2017 to distribute to students to do Lab 1                     *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT} Metric;


/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10 
#define FCFS            1 
#define SJF            2
#define RR              3 


#define MAXMETRICS      5 



/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/




/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics

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
ProcessControlBlock *SJF_Scheduler();
ProcessControlBlock *FCFS_Scheduler();
ProcessControlBlock *RR_Scheduler();
void                 Dispatcher();

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
  ManagementInitialization();
  while (1) {
    IO();
    CPUScheduler(PolicyNumber);
    Dispatcher();
  }
}

/* XXXXXXXXX Do Not Change IO() Routine XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/***********************************************************************\
* Input : none                                                          *          
* Output: None                                                          *        
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (for RR) return Process to Ready Queue              *                           
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
 * Input : whichPolicy (1:FCFS, 2: SJF, and 3:RR)                      *
 * Output: None                                                         * 
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock *selectedProcess;
  switch(whichPolicy){
    case FCFS : selectedProcess = FCFS_Scheduler();
      break;
    case SJF : selectedProcess = SJF_Scheduler();
      break;
    case RR   : selectedProcess = RR_Scheduler();
  }
  if (selectedProcess) {
    selectedProcess->state = RUNNING; // Process state becomes Running                                     
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue                         
  }
}

/***********************************************************************\                                               
 * Input : None                                                         *                                               
 * Output: Pointer to the process based on First Come First Serve (FCFS)*
 * Function: Returns process control block based on FCFS                *                                                \***********************************************************************/
ProcessControlBlock *FCFS_Scheduler() {
  /* Select Process based on FCFS */
  // Implement code for FCFS
  ProcessControlBlock *selectedProcess = DequeueProcess(READYQUEUE);

   if (selectedProcess) {
	 selectedProcess->TimeInReadyQueue = Now() - selectedProcess->JobStartTime;
	 SumMetrics[WT] += selectedProcess->TimeInReadyQueue;
   }

  return(selectedProcess);
}



/***********************************************************************\                         
 * Input : None                                                         *                                     
 * Output: Pointer to the process with shortest remaining time (SJF)   *
 * Function: Returns process control block with SJF                    *
\***********************************************************************/
ProcessControlBlock *SJF_Scheduler() {
  /* Select Process with Shortest Remaining Time*/

   ProcessControlBlock *currentProcess = DequeueProcess(READYQUEUE);
   ProcessControlBlock *firstProcess = currentProcess;    //Keep the first process
   ProcessControlBlock *selectedProcess = DequeueProcess(READYQUEUE);

   while(currentProcess)
   {
		 if(!selectedProcess)//If there is no second process, the only process is returned
		 {
			 selectedProcess = currentProcess;
			 break;
		 }

		 if(currentProcess == firstProcess) //If the current process is the first process added to the end of the queue, the loop ends and returns
		 {
			 EnqueueProcess(READYQUEUE, currentProcess);
			 break;
		 }
		 else if(currentProcess->CpuBurstTime < selectedProcess->CpuBurstTime)//Compare the current process with the process with the smallest time
		 {
			 EnqueueProcess(READYQUEUE, selectedProcess);  //Put a long process back into the queue
			 selectedProcess = currentProcess; //Choose a short process
		 }
		 else//Otherwise the current process is not a short time process, put it back to the ready queue
		 {
			 EnqueueProcess(READYQUEUE, currentProcess);
		 }
		 currentProcess = DequeueProcess(READYQUEUE);//Continue to visit the ready queue
   }

   if (selectedProcess) {
    	 selectedProcess->TimeInReadyQueue = Now() - selectedProcess->JobStartTime;
    	 SumMetrics[WT] += selectedProcess->TimeInReadyQueue;
   }

   return(selectedProcess);
}


/***********************************************************************\                                               
 * Input : None                                                         *                                               
 * Output: Pointer to the process based on Round Robin (RR)             *                                               
 * Function: Returns process control block based on RR                  *                                              \
 \***********************************************************************/
ProcessControlBlock *RR_Scheduler() {
  /* Select Process based on RR*/
  ProcessControlBlock *selectedProcess = DequeueProcess(READYQUEUE);

  if (selectedProcess) {
  	 selectedProcess->TimeInReadyQueue = Now() - selectedProcess->JobStartTime;
  	 SumMetrics[WT] += selectedProcess->TimeInReadyQueue;
  }

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
    ProcessControlBlock *selectedProcess = DequeueProcess(RUNNINGQUEUE);
    if (selectedProcess == NULL) return;

    if(selectedProcess->TimeInCpu == 0)//The process gets the CPU for the first time
    {
    	selectedProcess->StartCpuTime = Now();
    	SumMetrics[RT] += (selectedProcess->StartCpuTime - selectedProcess->JobStartTime);
    }

    if(selectedProcess->TimeInCpu >= selectedProcess->TotalJobDuration)//If the process finishes CPU operation
    {
    	selectedProcess->JobExitTime = Now();
    	EnqueueProcess(EXITQUEUE,selectedProcess);
    	NumberofJobs[THGT] += 1;
    	SumMetrics[TAT] += (selectedProcess->JobExitTime - selectedProcess->JobArrivalTime);
    }
    else//Process still needs computing
    {
    	TimePeriod CpuBurstTimeVar;
    	if(PolicyNumber == FCFS)
    	{
    		CpuBurstTimeVar = selectedProcess->TotalJobDuration;
    	}

    	if(PolicyNumber == SJF)
		{

			if(selectedProcess->TotalJobDuration - selectedProcess->TimeInCpu < selectedProcess->CpuBurstTime)
			{
				CpuBurstTimeVar = selectedProcess->TotalJobDuration - selectedProcess->TimeInCpu;
			}
			else
			{
				CpuBurstTimeVar = selectedProcess->CpuBurstTime;
			}
		}

    	if(PolicyNumber == RR)
    	{
    		if(selectedProcess->TotalJobDuration - selectedProcess->TimeInCpu < Quantum)
			{

    			CpuBurstTimeVar = selectedProcess->TotalJobDuration - selectedProcess->TimeInCpu;
			}
			else
			{
				CpuBurstTimeVar = Quantum;
			}
    	}

    	OnCPU(selectedProcess, CpuBurstTimeVar);//Let the process run on CPU during CpuBurstTimeVar

    	SumMetrics[CBT] += CpuBurstTimeVar;
    	selectedProcess->TimeInCpu += CpuBurstTimeVar;
    	EnqueueProcess(RUNNINGQUEUE, selectedProcess);
    }
    return;
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


/***********************************************************************\                                               * Input : None                                                         *                                                * Output: None                                                         *                
* Function:                                                            *
* 1) BookKeeping is called automatically when 250 arrived              *
* 2) Computes and display metrics: average turnaround  time, throughput*
*     average response time, average waiting time in ready queue,      *
*     and CPU Utilization                                              *                                                     
\***********************************************************************/
void BookKeeping(void){
  double end = Now(); // Total time for all processes to arrive
  Metric m;

  // Compute averages and final results
  SumMetrics[TAT] /= NumberofJobs[THGT];
  SumMetrics[RT] /= NumberofJobs[THGT];
  SumMetrics[CBT] /= end;
  SumMetrics[WT] /= NumberofJobs[THGT];

  printf("\n********* Processes Managemenent Numbers ******************************\n");
  printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f\n", 
	 SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT], 
	 NumberofJobs[THGT]/Now(), SumMetrics[WT]);

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

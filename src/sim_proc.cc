#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
using namespace std;
#include "sim_proc.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/
int main (int argc, char* argv[])
{
//    FILE *FP;               // File handler
//    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
//    int op_type, dest, src1, src2;  // Variables are read from trace file
//    unsigned long int pc; // Variable holds the pc read from input file
    
    if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
/*    printf("rob_size:%lu "
            "iq_size:%lu "
            "width:%lu "
            "tracefile:%s\n", params.rob_size, params.iq_size, params.width, trace_file);*/
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
	
	// Storing values in global variables
	rob_size = params.rob_size;
	iq_size = params.iq_size;
	width = params.width;
	ex_size = width*5;
	
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
	//Initializing stages
	int i;
	instruction = (stage *)malloc(100000*sizeof(stage));
	
	fe = (stage *)malloc(width*sizeof(stage));
	for(i=0; i<width; i++)
		fe[i].valid = 0;
	
	de = (stage *)malloc(width*sizeof(stage));
	for(i=0; i<width; i++)
		de[i].valid = 0;
	
	rn = (stage *)malloc(width*sizeof(stage));
	for(i=0; i<width; i++)
		rn[i].valid = 0;
	
	rr = (stage *)malloc(width*sizeof(stage));
	for(i=0; i<width; i++)
		rr[i].valid = 0;
	
	di = (stage *)malloc(width*sizeof(stage));
	for(i=0; i<width; i++)
		di[i].valid = 0;
	
	is = (stage *)malloc(iq_size*sizeof(stage));
	for(i=0; i<iq_size; i++)
		is[i].valid = 0;
	
	ex = (stage *)malloc(ex_size*sizeof(stage));
	for(i=0; i<ex_size; i++)
		ex[i].valid = 0;
	
	wb = (stage *)malloc(ex_size*sizeof(stage));
	for(i=0; i<ex_size; i++)
		wb[i].valid = 0;
	
	// Initializing ROB
	rob = (stage *)malloc(rob_size*sizeof(stage));
	robhead = 0;
	robtail = 0;
	for(i=0; i<rob_size; i++)
		rob[i].valid = 0;
	
	//Initializing RMT
	rmt = (RMtable *)malloc(total_reg*sizeof(RMtable));
	for(i=0; i<total_reg; i++)
	{
		rmt[i].valid = 0;
		rmt[i].tag = -1;
	}
	
	
	
	// Getting values from trace
	i=0;
	while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
	{
		instruction[i].valid = 1;
		instruction[i].pc = pc;
		instruction[i].dest = dest;
		instruction[i].source1 = src1;
		instruction[i].source2 = src2;
		instruction[i].type = op_type;
		
		++i;
		total++;
	}
	
	
	//Initializing stage-empty variables - Initially all stages are empty
	fe_empty = de_empty = rn_empty = rr_empty = di_empty = is_empty = ex_empty = wb_empty = rt_empty = rob_empty = 1;
	empty_pipeline = 1;
	
	//Main simulation loop
	do
	{
		Retire();
		WriteBack();
		Execute();
		Issue();
		Dispatch();
		RegisterRead();
		Rename();
		Decode();
		Fetch();
		
		++cycle;
	}while(Advance_Cycle());
	
	//Calculating IPC
	float tot,cyc;
	float ipc;
	tot = total;
	cyc = cycle;
	ipc = tot/cyc;
	
	//Printing final simulator contents
	printf("# === Simulator Command =========");
	printf("\n# ./sim %d %d %d ", rob_size, iq_size, width);
	cout<<trace_file;
	printf("\n# === Processor Configuration ===");
	printf("\n# ROB_SIZE = %d", rob_size);
	printf("\n# IQ_SIZE  = %d", iq_size);
	printf("\n# WIDTH    = %d", width);
	printf("\n# === Simulation Results ========");
	printf("\n# Dynamic Instruction Count    = %d", total);
	printf("\n# Cycles                       = %d", cycle);
	printf("\n# Instructions Per Cycle (IPC) = %.2f", ipc);
    return 0;
}

// Checking for empty
bool checkforempty(stage *temp, int type)
{
	int i; 
	
	if(type == 0)
	{
		for(i = 0; i< width; i++)
		{
			if(temp[i].valid == 1)
				return false;
		}
		return true;
	}
	else if(type == 1)
	{
		for(i = 0; i< iq_size; i++)
		{
			if(temp[i].valid == 1)
					return false;
		}
		return true;
	}
	else if(type == 2)
	{
		for(i = 0; i< ex_size; i++)
		{
			if(temp[i].valid == 1)
				return false;
		}
		return true;
	}
	else
		return false;
}

//Advance cycle function for do-while loop
bool Advance_Cycle(void)
{
	if(robhead == robtail)
		rob_empty = 1;
	else
		rob_empty = 0;
	
	fe_empty = checkforempty(fe,0);
	de_empty = checkforempty(de,0);
	rn_empty = checkforempty(rn,0);
	rr_empty = checkforempty(rr,0);
	di_empty = checkforempty(di,0);
	is_empty = checkforempty(is,1);
	ex_empty = checkforempty(ex,2);
	wb_empty = checkforempty(wb,2);
	if(fe_empty && de_empty && rn_empty && rr_empty && di_empty && is_empty && ex_empty && wb_empty && rob_empty)
		empty_pipeline = 1;	
	else	
		empty_pipeline = 0;
	
	if(empty_pipeline)
	{
		return false;
	}
	else
		return true;
}

//Fetch function
void Fetch()
{
	int i;
	de_empty = checkforempty(de,0);
	if(de_empty)
	{
		for(i=0;i<width;i++)
		{
		  if(m < total)
		  {
			fe_empty = 0;
			fe[i].valid = 1;												// Fetching contents
			fe[i].pc = instruction[m].pc;
			fe[i].dest = instruction[m].dest;
			fe[i].source1 = instruction[m].source1;
			fe[i].source2 = instruction[m].source2;
			fe[i].desta = instruction[m].dest;
			fe[i].source1a = instruction[m].source1;
			fe[i].source2a = instruction[m].source2;
			fe[i].type = instruction[m].type;
			fe[i].age = age;
			++age;
			fe[i].clockin = cycle;
			fe[i].timespent = 1;
			fe[i].destready = 0;
			
			if(instruction[m].source1 == -1)
				fe[i].src1ready = 1;
			else
				fe[i].src1ready = 0;
			if(instruction[m].source2 == -1)
				fe[i].src2ready = 1;
			else
				fe[i].src2ready = 0;
			
			if(instruction[m].type == 0)
				fe[i].time = 1;
			else if(instruction[m].type == 1)
				fe[i].time = 2;
			else if(instruction[m].type == 2)
				fe[i].time = 5;
			
			
			de[i] = fe[i];												//Moving contents to decode stage
			de[i].valid = 1;
			de[i].clockin = cycle+1;
			fe[i].valid = 0;
			
			de[i].fe_arrival = cycle;
			de[i].fe_duration = 1;
			de[i].de_arrival = cycle+1;
			
			
			++m;
		  }
		  else 
			  return;
		}
	}
}

void Decode()
{
	int i;
	rn_empty = checkforempty(rn,0);
	if(rn_empty)
	{
		for(i=0;i<width;i++)
		{
			if(de[i].valid)
			{
				rn[i] = de[i];													//Moving contents to rename stage
				rn[i].valid = 1;
				rn[i].clockin = cycle+1;
				de[i].timespent = rn[i].clockin - de[i].clockin;
				
				rn[i].rn_arrival = cycle+1;
				rn[i].de_duration = rn[i].rn_arrival - rn[i].de_arrival;
				
				de[i].valid = 0;
			}
		}
	}
}

void Rename()
{
	robspace = 0;
	int i;
	for(i=0;i<rob_size;i++)
	{
		if(rob[i].valid == 0)
			++robspace;
	}
	
	rr_empty = checkforempty(rr,0);
	if(rr_empty)
	{
		if(robspace >= width)
		{
			for(i=0;i<width;i++)
			{
				if(rn[i].valid)
				{
					rob[robtail] = rn[i];											//Storing contents in ROB
					
					if(rob[robtail].source1 != -1)
					{
						if(rmt[rob[robtail].source1].valid == 1)
						{
							rn[i].source1 = rmt[rob[robtail].source1].tag;
							rn[i].source1_inrob = 1;
						}
						else
							rn[i].source1_inrob = 0;
					}
					else
						rn[i].source1_inrob = 0;
					
					if(rob[robtail].source2 != -1)
					{
						if(rmt[rob[robtail].source2].valid == 1)
						{
							rn[i].source2 = rmt[rob[robtail].source2].tag;
							rn[i].source2_inrob = 1;
						}
						else
							rn[i].source2_inrob = 0;
					}
					else
						rn[i].source2_inrob = 0;
					
					if(rob[robtail].dest != -1)
					{
						rmt[rob[robtail].dest].valid = 1;
						rmt[rob[robtail].dest].tag = robtail;
					}
					rn[i].dest = robtail;
					
					rr[i] = rn[i];													//Moving contents to Register read stage
					rr[i].valid = 1;
					rr[i].clockin = cycle+1;
					rn[i].timespent = rr[i].clockin - rn[i].clockin;
					
					rr[i].rr_arrival = cycle+1;
					rr[i].rn_duration = rr[i].rr_arrival - rr[i].rn_arrival;
					

					rn[i].valid = 0;
					
					if(robtail == (rob_size-1))
						robtail = 0;
					else
						++robtail;
					
				}
			}
		}
	}
		
}

void RegisterRead()
{
	
	int i;
	di_empty = checkforempty(di,0);
	if(di_empty)
	{
		for(i=0;i<width;i++)
		{
			if(rr[i].valid)
			{
			  if(rr[i].source1 != -1)
			  {
				if(rr[i].source1_inrob)											//Checking if in ROB
				{
					if(rob[rr[i].source1].destready)							//Checking for readiness in ROB
						rr[i].src1ready = 1;
				}
				else 
					rr[i].src1ready = 1;										//If not in ROB, setting ready bit since it is in ARF
			  }
			  else
				  rr[i].src1ready = 1;
			
			  if(rr[i].source2 != -1)
			  {
				if(rr[i].source2_inrob)
				{
					if(rob[rr[i].source2].destready)
						rr[i].src2ready = 1;
				}
				else 
					rr[i].src2ready = 1;
			  }
			  else
				  rr[i].src2ready = 1;
				
				di[i] = rr[i];											//Moving contents to dispatch stage
				di[i].valid = 1;
				di[i].clockin = cycle+1;
				rr[i].timespent = di[i].clockin - rr[i].clockin;
				
				di[i].di_arrival = cycle+1;
				di[i].rr_duration = di[i].di_arrival - di[i].rr_arrival;
				
				rr[i].valid = 0;
				di_empty = 0;
			}
		}
	}
				
}

void Dispatch()
{
	
	int i;
	iq_space = 0;
	for(i=0;i<iq_size;i++)
	{
		if(is[i].valid == 0)
			iq_space++;
	}
	dispatch = 0;
	for(i=0;i<width;i++)
	{
		if(di[i].valid == 1)
			dispatch++;
	}
	
	int j;
	int issued;
	if(iq_space >= dispatch)										//Checking if issue queue has enough space
	{
		for(i=0;i<width;i++)
		{
			if(di[i].valid == 1)
			{
				issued = 0;
				for(j=0; j<iq_size; j++)
				{
					if(is[j].valid == 0)
					{
						is[j] = di[i];								//Moving contents to issue queue
						is[j].valid = 1;
						is[j].clockin = cycle+1;
						di[i].timespent = is[j].clockin - di[i].clockin;
						
						is[j].is_arrival = cycle+1;
						is[j].di_duration = is[j].is_arrival - is[j].di_arrival;
						

						di[i].valid = 0;
						issued = 1;
					}
					if(issued)
						break;
				}
			}
		}
	}
}

void Issue()
{
		
	// Sorting IQ contents based on age
	stage temp_is;
	int i,j;
	for(i=0;i<iq_size;i++)
	{
		for(j=i+1;j<iq_size;j++)
		{
			if(is[j].age < is[i].age)
			{
				temp_is = is[i];
				is[i] = is[j];
				is[j] = temp_is;
			}
		}
	}
	
	int issue_to_ex = 0;
	int issued;
	for(i=0;i<iq_size;i++)
	{
		if(is[i].valid)
		{
			issued = 0;
			if(is[i].src1ready && is[i].src2ready)								//Checking if ready for execution
			{
				for(j=0;j<ex_size;j++)
				{
					if(ex[j].valid == 0)
					{
						ex[j] = is[i];											//Moving contents to execute stage
						ex[j].valid = 1;
						ex[j].clockin = cycle+1;
						is[i].timespent = ex[j].clockin - is[i].clockin;
						
						ex[j].ex_arrival = cycle+1;
						ex[j].is_duration = ex[j].ex_arrival - ex[j].is_arrival;
						

						is[i].valid = 0;
						issue_to_ex++;
						issued = 1;
						if(issue_to_ex == width)
							return;
					}
					if(issued)
						break;
				}
			}
		}
	}	
}


void Execute()
{
	
	int i,j;
	for(i=0;i<ex_size;i++)
	{
		if(ex[i].valid)
		{
			--ex[i].time;											//Decrementing execution time taken
			if(ex[i].time == 0)
			{
				if(ex[i].dest != -1)
				{
					for(j=0;j<iq_size;j++)							//Wakeup in IS
					{
						if(is[j].valid)
						{
							if((ex[i].dest == is[j].source1) && (is[j].source1_inrob))
							{
								is[j].src1ready = 1;				//Waking up source1
							}
							if((ex[i].dest == is[j].source2) && (is[j].source2_inrob))
							{
								is[j].src2ready = 1;				//Waking up source2
							}
						}
					}
					
					for(j=0;j<width;j++)							//Wakeup in DI
					{
						if(di[j].valid)
						{
							if((ex[i].dest == di[j].source1) && (di[j].source1_inrob))
							{
								di[j].src1ready = 1;				//Waking up source1
							}
							if((ex[i].dest == di[j].source2) && (di[j].source2_inrob))
							{
								di[j].src2ready = 1;				//Waking up source2
							}
						}
					}
					
					for(j=0;j<width;j++)							//Wakeup in RR
					{
						if(rr[j].valid)
						{
							if((ex[i].dest == rr[j].source1) && (rr[j].source1_inrob))
							{
								rr[j].src1ready = 1;				//Waking up source1
							}
							if((ex[i].dest == rr[j].source2) && (rr[j].source2_inrob))
							{
								rr[j].src2ready = 1;				//Waking up source2
							}
						}
					}
				}
			}
		}
	}
	
	//To WB stage
	int wroteback;
	for(i=0;i<ex_size;i++)
	{
		if(ex[i].valid)
		{
			if(ex[i].time == 0)
			{
				wroteback = 0;
			  for(j=0;j<ex_size;j++)
			  {
				if(wb[j].valid == 0)
				{
					wb[j] = ex[i];														//Moving contents to writeback stage
					wb[j].valid = 1;
					wb[j].clockin = cycle+1;
					ex[i].timespent = wb[j].clockin - ex[i].clockin;
					
					wb[j].wb_arrival = cycle+1;
					wb[j].ex_duration = wb[j].wb_arrival - wb[j].ex_arrival;
					

					
					wb[j].destready = 1;
					ex[i].valid = 0;
					wroteback = 1;
				}
				if(wroteback)
					break;
			  }
			}
		}
	}
}

void WriteBack()
{
	
	int i;
	for(i=0;i<ex_size;i++)
	{
		if(wb[i].valid)
		{
			rob[wb[i].dest] = wb[i];													//Writing back to ROB
			rob[wb[i].dest].valid = 1;
			rob[wb[i].dest].destready = 1;
			rob[wb[i].dest].clockin = cycle+1;
			wb[i].timespent = rob[wb[i].dest].clockin - wb[i].clockin;
			
			rob[wb[i].dest].rt_arrival = cycle+1;
			rob[wb[i].dest].wb_duration = rob[wb[i].dest].rt_arrival - rob[wb[i].dest].wb_arrival;
						
			wb[i].valid = 0;
		}
	}
}

void Retire()
{
	
	int i;
	for(i=0;i<width;i++)
	{
		if((rob[robhead].valid) && (rob[robhead].destready))
		{
				rob[robhead].timespent = (cycle+1) - rob[robhead].clockin;
				
				rob[robhead].rt_duration = (cycle+1) - rob[robhead].rt_arrival;
				
				if(rob[robhead].desta != -1)
				{
					if(rmt[rob[robhead].desta].valid == 1)
					{
						if(rmt[rob[robhead].desta].tag == robhead)
							rmt[rob[robhead].desta].valid = 0;								//Retiring by invalidating RMT 
					}
				}
				rob[robhead].valid = 0;
				printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} RR{%d,%d} DI{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d} RT{%d,%d}\n",
					num,rob[robhead].type,rob[robhead].source1a,rob[robhead].source2a,rob[robhead].desta,
					rob[robhead].fe_arrival,rob[robhead].fe_duration, 
					rob[robhead].de_arrival, rob[robhead].de_duration,
					rob[robhead].rn_arrival,rob[robhead].rn_duration,
					rob[robhead].rr_arrival,rob[robhead].rr_duration,
					rob[robhead].di_arrival,rob[robhead].di_duration,
					rob[robhead].is_arrival,rob[robhead].is_duration,
					rob[robhead].ex_arrival,rob[robhead].ex_duration,
					rob[robhead].wb_arrival,rob[robhead].wb_duration,
					rob[robhead].rt_arrival,rob[robhead].rt_duration);
										
				num++;
				
				if(robhead == (rob_size-1))
					robhead = 0;
				else
					robhead++;
		}
		else
			return;
	}
}
#ifndef SIM_PROC_H
#define SIM_PROC_H

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct{
	int valid;
	unsigned long int pc;
	int source1;
	int source1a;
	int src1ready;
	int source1_inrob;
	int source2;
	int source2a;
	int src2ready;
	int source2_inrob;
	int dest;
	int desta;
	int destready;
	int clockin;
	int timespent;
	int time;
	int age;
	int type;
	
	int fe_arrival;
	int de_arrival;
	int rn_arrival;
	int rr_arrival;
	int di_arrival;
	int is_arrival;
	int ex_arrival;
	int wb_arrival;
	int rt_arrival;
	int fe_duration;
	int de_duration;
	int rn_duration;
	int rr_duration;
	int di_duration;
	int is_duration;
	int ex_duration;
	int wb_duration;
	int rt_duration;
}stage;	
	

typedef struct{
	int tag;
	int valid;
}RMtable;

//Pointer objects for stages
stage *fe,*de,*rn,*rr,*di,*is,*ex,*wb,*rt,*rob;
stage *instruction;
RMtable *rmt;

//Variables to check stage-empty
int fe_empty, de_empty, rn_empty, rr_empty, di_empty, is_empty, ex_empty, wb_empty, rt_empty, rob_empty;
int empty_pipeline;

int total_reg = 67;
FILE *FP;               // File handler
char *trace_file;       // Variable that holds trace file name;
int op_type, dest, src1, src2;  // Variables are read from trace file
unsigned long int pc; // Variable holds the pc read from input file

//Control variables
int width;
int rob_size, iq_size, ex_size;
int robspace,iq_space,dispatch;
int robhead, robtail;

//Counters used
int total = 0;
int cycle = 0;
int fe_cycle = 0;
int age = 0;
int total_cycles = 0;
int num = 0;
int m = 0;

//Functions
bool Advance_Cycle(void);
void Retire(void);
void WriteBack(void);
void Execute(void);
void Issue(void);
void Dispatch(void);
void RegisterRead(void);
void Rename(void);
void Decode(void);
void Fetch(void);
void agesort(stage*);
bool checkforempty(stage*,int);

#endif

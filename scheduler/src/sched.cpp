#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <deque>
#include <queue>
#include <algorithm>

using namespace std;


FILE *infile;
// FILE *rand;

struct Token {
     char *token;
    int line , lineofs;
};

// typedef struct Token Struct;

Token findtoken(){
  Token tokum;
  static bool newline = 1; // you must read a new line
  const char delimeter[] = "\n \t";
  static char *buffer; // large buffer that must be static because we reuse across calls.
  static int linesize;
  static int line = 0;
  size_t BUFSZ = 1024;
  tokum.token = NULL;
  int sz;

//   tokum.lineofs = 0;
//   tokum.line= 1;
  char *tok = NULL;
  while (1) {
    while (newline)  {
      sz = getline(&buffer, &BUFSZ, infile);
      if (sz == -1){
           tokum.token = NULL;
           tokum.line = line;
           tokum.lineofs = linesize;
           return tokum; // eof
      }
      line++;
     linesize = strlen(buffer);
      // now lets seed the tokenizer
      tok = strtok(buffer,delimeter);
      if (tok != NULL) {
           newline = 0;  // next time we call getToken we don't need to call a new line
           tokum.token= tok;
           tokum.line = line;
           tokum.lineofs = (tok - buffer+1);
           return tokum;
      }
      // looks like an empty line so read another
   }
   tok = strtok(NULL, delimeter); // note we use NULL to read from the same line
   if (tok == NULL) {
        newline = 1;  // next time we call getToken we don't need to call a new line
        continue;     // forces reading a new line
   }
   tokum.token = tok;
   tokum.line = line;
   tokum.lineofs = (tok - buffer+1);
   return tokum;
}}


class Process
{
public:
     int arrival_time;
     int total_cpu;
     int cb;
     int io;

     int finishing_time; 
     int turnaround_time; //finishing time - arrival_time
     int full_io_time = 0; // time in blocked state
     int PRIO; // static priority
     int cpu_waiting = 0; // cpu waiting time (time in ready state)
     int ready_cpu_waiting_start;

     int dynamic_priority;
     int cpu_done_time;
     int process_id;
     int burst_remaining = 0;

};

class Scheduler
{
       // dequeue or Vector
  public:
     std::deque <Process *> runQ;
     // string SchedType; // this will be FCFS, LC, RR, etc
     // int maxprio = 4;
     // int quantum = 10000;

     virtual void add_process(Process *) = 0;
     virtual Process *get_next_process() = 0;

     // virtual void test_preempt(Process *, int curtime); // typically NULL but for ‘E’

};

class FCFS : public Scheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:
  string SchedType = "FCFS";

  void add_process(Process *p)
  {
       runQ.push_back(p);
  }

  Process *get_next_process()
  {
       Process *p;
       if (runQ.empty()){return nullptr;}
       else 
       {
          p = runQ.front();
          runQ.pop_front();
          return p;
       }
  }

};

class LCFS : public Scheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:

     string SchedType = "LCFS";

  void add_process(Process *p)
  {
       runQ.push_front(p);
  }

  Process *get_next_process()
  {
       Process *p;
       if (runQ.empty()){return nullptr;}
       else 
       {
          p = runQ.front();
          runQ.pop_front();
          return p;
       }
  }

};

class SRTF : public Scheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:

  string SchedType = "SRTF";

  void add_process(Process *p)
  {
     //   runQ.push_front(p);
     int i =0;
		while (i < runQ.size() && (p->total_cpu - p->cpu_done_time) >= (runQ[i]->total_cpu - runQ[i]->cpu_done_time)) 
          { 
			i++;
		}
		runQ.insert(runQ.begin()+i,p);
  }

  Process *get_next_process()
  {
       Process *p;
       if (runQ.empty()){return nullptr;}
       else 
       {
          p = runQ.front();
          runQ.pop_front();
          return p;
       }
  }

};

class PrioSched : public Scheduler
{
  deque<Process *> *activeQueue;
  deque<Process *> *expiredQueue;
  
  void swapQueues()
  {
    deque<Process *> *temp = activeQueue;
    activeQueue = expiredQueue;
    expiredQueue = temp;
  }


public:
     int max_prio;

  PrioSched(int x)
  {
     max_prio = x;
     // int dynamic_prio = 10;
    // set quantum and max prio
    this->activeQueue = new deque<Process *>[max_prio];
    this->expiredQueue = new deque<Process *>[max_prio];
  }

     bool activeqe_empty()
     {
          bool empty = 1;
          for (int i = max_prio-1; i>=0;i--)
          {    
               empty = activeQueue[i].empty() && empty;

          }
          return empty;
     }

     bool all_empty()
     {
          bool empty = 1;
          for (int i = max_prio-1; i>=0;i--)
          {    
               empty = activeQueue[i].empty() && expiredQueue[i].empty() && empty;

          }
          return empty;
     }


     void add_process(Process *process)
     {
     // code
     // I want to add the process to the activeQueue here
     if (process->dynamic_priority == -1)
     {
          process->dynamic_priority = (process->PRIO -1);
          expiredQueue[process->dynamic_priority].push_back(process);
     }
     else
     {
          activeQueue[process->dynamic_priority].push_back(process);
     }

    
  }


     Process *get_next_process()
  {
    // code
    Process *p;
    if (all_empty()){return nullptr;}

    if (activeqe_empty()){swapQueues();}

    for (int i = max_prio-1; i>=0;i--)
    {

       if (!activeQueue[i].empty())
       {

          p = activeQueue[i].front();
          activeQueue[i].pop_front();
          return p;
       }

    }
    return p;
  }
};

enum  state_tr {STATE_CREATED, STATE_READY, STATE_RUNNING, STATE_BLOCKED, STATE_DONE, STATE_PREMPT} ;

class Event
{
  public:
     int timestamp; // when transition wants to happen
     int process_id;  // Process *process (pid)
     
     // { "CREATED", "READY", "RUNNING", "BLOCKED", "DONE" };
     state_tr oldstate;
     state_tr newstate;
     int event_create_time;
};

class DES
{
     deque <Event *> eventq; // dequeue
public:

     Event* get_event()
     {
          Event *e = eventq.front(); 
          eventq.pop_front();
          return e;
     }

     int get_size()
     {
          // cout<<"SIZE of DES ";eventq.size();cout<<endl;
          return eventq.size();
     }

     void put_event(Event *e)
     {    

          int i =0;
          for (i;i < eventq.size() && e->timestamp >= eventq[i]->timestamp;i++)
          {}

          eventq.insert(eventq.begin()+i,e);
     }

     void printq()
     {    
          cout<<"PRINTING Q"<<endl;
          for (size_t i = 0; i < eventq.size(); i++)
          {
               cout<<eventq[i]->timestamp<<" "<<eventq[i]->process_id<<" "<<eventq[i]->oldstate
               <<" "<<eventq[i]->newstate<<endl;
          }
     }

     int get_next_event_time()
     {
          if (eventq.empty()) {return -1;}
          return eventq.front()->timestamp; //if (get_next_event_time == CURRENT_TIME) continue..
     }
};

std::deque <Process> processes; 
const char * filename;
const char * random_f;
std::deque <int> randoms;
static int ofs = 0; 
// deque <Event> eventq;
deque<string> STATES = { "CREATED", "READY", "RUNNG", "BLOCK", "DONE" , "PREEMPTION"};
int CURRENT_TIME;
// static int maxprio = 4;
// static int quantum = 10000;

void random_file(const char * random_f)
{

     infile = fopen(random_f,"r");
     Token toking;
     toking = findtoken();
     int size = atoi(toking.token);
     long int word;
     for (toking = findtoken();!(toking.token == NULL);toking = findtoken())
     {
          word = atoi(toking.token);
          randoms.push_back(word);
     }
     fclose(infile);
}

int myrandom(int burst) {
	if (ofs == randoms.size()) {ofs = 0;}
     return 1 + (randoms[ofs++] % burst);
}

deque <Process> input_file(const char * filename, int maxprio)
{
     infile = fopen(filename,"r");
     Token toking;
     Process p;
     int i= 0;
     for (toking = findtoken();!(toking.token == NULL);toking = findtoken())
     {
          p.arrival_time = atoi(toking.token);
          toking = findtoken();
          p.total_cpu = atoi(toking.token);
          toking = findtoken();
          p.cb = atoi(toking.token);
          toking = findtoken();
          p.io = atoi(toking.token);
          p.PRIO = myrandom(maxprio);
          p.process_id = i++;
          p.cpu_done_time = 0;
          p.dynamic_priority = p.PRIO - 1;
          // p.burst_remaining = 0;
          // cout<<"PRIORITY "<<p.PRIO<<endl;
          processes.push_back(p);
          // cout<<p.arrival_time<<endl<<p.total_cpu<<endl<<p.cb<<endl<<p.io<<endl;
          // toking = findtoken();
     }
     fclose(infile);
     return processes;
}



int main(int argc, char *argv[])
{    
     Scheduler *scheduler;
     bool VERBOSE = false;
     int quantum = 10000;
     int maxprio = 4;
     bool CALL_SCHEDULER;
     Process *CURRENT_RUNNING_PROCESS = nullptr;
     int burst;
     int ib;

     int maxfintime = 0;
     int total_cpu = 0;
     int total_cpu_waiting = 0;
     int total_turnaround = 0;
     int total_io = 0;
     int io_processes = 0;
     int io_start_time = 0;

     int opt; 
     string s;
     string type_s;
     string printing;
      
    // put ':' in the starting of the 
    // string so that program can  
    //distinguish between '?' and ':'  
    while((opt = getopt(argc, argv, "vtes:f:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'v':
               VERBOSE = true;
               printf("option verbose: %c\n", opt); 
  
                break;  
            case 't':  
            case 'e':
  
            case 's':  
               //  printf("filename: %s\n", optarg);
                s = string(optarg);
               //  cout<<"FULL "<<s<<endl;
               //  sscanf(s.substr(0,1).c_str(), "%s",&type_s);
               type_s = s.substr(0,1);
               //  printf("%s\n", s.substr(0,1).c_str());
               //  printf("type_s  %s\n",type_s);
               //  cout<< (type_s == "S")<<endl;
               //  printf("LATER %s\n", s.substr(1).c_str());
                sscanf(s.substr(1).c_str(), "%d:%d",&quantum,&maxprio);
               //  printf("quantum:%d and maxprio: %d\n", quantum, maxprio); 
                break;      
            case ':':  
               //  printf("option needs a value\n");  
                break;  
            case '?':  
                if (optopt == 's')
                    fprintf (stderr, "Option -%c requires an argument. \n", optopt);
                else if (isprint (optopt))
                    // fprintf (stderr, "Unknown option '-%c'.\n", optopt);
                return false;  
        }  
    }  
      
    // optind is for the extra arguments 
    // which are not parsed 
//     for(; optind < argc; optind++){      
//         printf("extra arguments: %s\n", argv[optind]);  
//     } 
     if (optind < argc)
     {
          filename = argv[optind];
          // printf("filename %s\n", filename);
          random_f = argv[++optind];
          // printf("randfile %s\n", random_f);
     }
     // cout<<"verbose: "<< VERBOSE<<endl;
      

     // random_f = argv[2];
     random_file(random_f);
     // filename = argv[1];
     input_file(filename, maxprio);

     // cout<<randoms[0]<<endl;
     // cout<<randoms.size()<<endl;

     if (type_s == "F")
     {
          scheduler = new FCFS();
          printing = "FCFS";
     }
     else if(type_s == "L")
     {
          scheduler = new LCFS();
          printing = "LCFS";
     }
     else if(type_s == "S")
     {
          scheduler = new SRTF();
          printing = "SRTF";
     }
     else if(type_s == "R")
     {
          scheduler = new FCFS();
          printing = "RR " + to_string(quantum);
     }
     else if(type_s == "P")
     {
          scheduler = new PrioSched(maxprio);
          printing = "PRIO " + to_string(quantum);
     }
     else if(type_s == "E")
     {
          scheduler = new PrioSched(maxprio);
     }

     
     // scheduler = new FCFS();
     // scheduler = new LCFS();
     // scheduler = new SRTF();
     // scheduler = new PrioSched(maxprio);
     // quantum = 2;

     // cout<<"SIZE of Processes "<<processes.size()<<endl;
     // for (size_t i = 0; i < processes.size(); i++)
     // {
     //      cout<<processes[i].arrival_time<<" "<<processes[i].total_cpu<<" "<<processes[i].cb
     //      <<" "<<processes[i].io<<endl;
     // }
     // cout<<"END PROCESSES"<<endl<<endl;
     DES eventqs;
     for (size_t i = 0; i < processes.size();i++)
     {
          Event *new_e = new Event();//processes[i].arrival_time , i, 1, 0); // transition to ready (1)
          new_e->timestamp = processes[i].arrival_time;
          new_e->process_id = i;
          new_e->newstate = STATE_READY;
          new_e->oldstate = STATE_CREATED;

          new_e->event_create_time = processes[i].arrival_time;
          eventqs.put_event(new_e); // same here


     }
     // cout<<"SIZE of EVENTQ "<<eventqs.get_size()<<endl;

     // eventqs.printq();

     // cout<<"END EVENTQ"<<endl<<endl;
     // cout<<"NEXT EVENT TIME "<<eventqs.get_next_event_time()<<endl;
     // cout<<"NEXT EVENT TIMESTAMP "<<eventq.get_event().timestamp<<endl<<"NEW State "<<STATES[eventq.get_event().newstate]<<endl;
     // for (size_t i = 0; i < STATES.size();i++)
     // {
     //      cout<<STATES[i]<<" ";
     // } 

     Event *evt;
     // cout<<"SIZE of EVENTQ "<<eventqs.get_size()<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl<<endl;
     while (eventqs.get_size())
     {    
          // Event *top_event = eventqs.get_event();
          // evt = &top_event;
          evt = eventqs.get_event();
          // cout<<"STATE GOING TO "<<STATES[evt->newstate]<<endl;
          CURRENT_TIME = evt->timestamp;
          Process *proc = &processes[evt->process_id];
          int timeinprevstate = CURRENT_TIME - evt->event_create_time;
          // cout<<proc->total_cpu<<endl;
          // if (VERBOSE){cout<<"Current Time "<<CURRENT_TIME<<endl;}
          // cout<< evt->newstate<<endl;
          switch (evt->newstate)
          {
          case STATE_READY:

              { if (VERBOSE){cout<<CURRENT_TIME<<" "<<evt->process_id<<" "<<timeinprevstate<<": "
               <<STATES[evt->oldstate]<<" -> "<<STATES[evt->newstate]<<endl;}


               proc->ready_cpu_waiting_start = CURRENT_TIME;
               if (evt->oldstate == STATE_BLOCKED)
               {
                    io_processes--;
                    if(io_processes == 0){total_io += (CURRENT_TIME - io_start_time );}
               }
               scheduler->add_process(proc);
               CALL_SCHEDULER = true;

               break;}

          case STATE_RUNNING:

               {if (VERBOSE){cout<<CURRENT_TIME<<" "<<evt->process_id<<" "<<timeinprevstate<<": "
               <<STATES[evt->oldstate]<<" -> "<<STATES[evt->newstate];}

               CURRENT_RUNNING_PROCESS = proc;

               if (CURRENT_RUNNING_PROCESS->burst_remaining > 0)
                    {burst = CURRENT_RUNNING_PROCESS->burst_remaining;}
               else 
               {
                    burst = myrandom(CURRENT_RUNNING_PROCESS->cb);
                    if (burst >= CURRENT_RUNNING_PROCESS->total_cpu - CURRENT_RUNNING_PROCESS->cpu_done_time)
                    {burst = CURRENT_RUNNING_PROCESS->total_cpu - CURRENT_RUNNING_PROCESS->cpu_done_time;}
               
               }
               // burst = myrandom(CURRENT_RUNNING_PROCESS->cb);
               CURRENT_RUNNING_PROCESS->cpu_waiting += CURRENT_TIME - CURRENT_RUNNING_PROCESS->ready_cpu_waiting_start;


               if (VERBOSE){cout<<" cb="<<burst<<" rem="<<CURRENT_RUNNING_PROCESS->total_cpu - CURRENT_RUNNING_PROCESS->cpu_done_time
                                   <<" prio="<<CURRENT_RUNNING_PROCESS->dynamic_priority<<endl;}

               if( quantum < burst)
               {
                    CURRENT_RUNNING_PROCESS->burst_remaining = burst - quantum;
                    burst = quantum;  
                      
                    Event *new_e = new Event();//processes[i].arrival_time , i, 1, 0); // transition to ready (1)
                    new_e->timestamp = CURRENT_TIME + burst;
                    new_e->process_id = CURRENT_RUNNING_PROCESS->process_id;
                    new_e->newstate = STATE_PREMPT;
                    new_e->oldstate = STATE_RUNNING;
                    new_e->event_create_time = CURRENT_TIME;
                    eventqs.put_event(new_e); // same here

               }
               // BLOCK EVENT
               
               else if (burst < CURRENT_RUNNING_PROCESS->total_cpu - CURRENT_RUNNING_PROCESS->cpu_done_time) // BLOCK EVENT
               {

                    // CURRENT_RUNNING_PROCESS->cpu_done_time += burst;
                    Event *new_e = new Event();//processes[i].arrival_time , i, 1, 0); // transition to ready (1)
                    new_e->timestamp = CURRENT_TIME + burst;
                    new_e->process_id = CURRENT_RUNNING_PROCESS->process_id;
                    new_e->newstate = STATE_BLOCKED;
                    new_e->oldstate = STATE_RUNNING;
                    new_e->event_create_time = CURRENT_TIME;
                    CURRENT_RUNNING_PROCESS->burst_remaining = 0;

                    eventqs.put_event(new_e); // same here
               }
               // DONE eVENT
               else if( burst == CURRENT_RUNNING_PROCESS->total_cpu - CURRENT_RUNNING_PROCESS->cpu_done_time) //DONE
               {
                    // CURRENT_RUNNING_PROCESS->cpu_done_time += burst;
                    Event *new_e = new Event();//processes[i].arrival_time , i, 1, 0); // transition to ready (1)
                    new_e->timestamp = CURRENT_TIME + burst;
                    new_e->process_id = CURRENT_RUNNING_PROCESS->process_id;
                    new_e->newstate = STATE_DONE;
                    new_e->oldstate = STATE_RUNNING;
                    new_e->event_create_time = CURRENT_TIME;
                    CURRENT_RUNNING_PROCESS->burst_remaining = 0;
                    eventqs.put_event(new_e); // same here
               }
               CURRENT_RUNNING_PROCESS->cpu_done_time += burst;

               break;}

          // RUN TO BLOCK
          case STATE_BLOCKED:

               {if (VERBOSE){cout<<CURRENT_TIME<<" "<<evt->process_id<<" "<<timeinprevstate<<": "
               <<STATES[evt->oldstate]<<" -> "<<STATES[evt->newstate];}

               CURRENT_RUNNING_PROCESS = nullptr;
               ib = myrandom(proc->io);
               proc->full_io_time +=ib;

               if (VERBOSE){cout<<" ib="<<ib<<" rem="<<proc->total_cpu - proc->cpu_done_time
                                   <<" prio="<<proc->dynamic_priority<<endl;}


               if (io_processes == 0){io_start_time = CURRENT_TIME;}
               io_processes++;
               proc->dynamic_priority = proc->PRIO - 1;
               
               Event *new_e = new Event();//processes[i].arrival_time , i, 1, 0); // transition to ready (1)
               new_e->timestamp = CURRENT_TIME + ib;
               new_e->process_id = evt->process_id;
               new_e->newstate = STATE_READY;
               new_e->oldstate = STATE_BLOCKED;
               new_e->event_create_time = CURRENT_TIME;
               eventqs.put_event(new_e); // same here


               CALL_SCHEDULER = true;
               break;}
          case STATE_PREMPT:
               {
               //      if (VERBOSE){cout<<CURRENT_TIME<<" "<<evt->process_id<<" "<<timeinprevstate<<": "
               // <<STATES[evt->oldstate]<<" -> "<<STATES[evt->newstate]<<endl;}

               proc->finishing_time = CURRENT_TIME;
               proc->dynamic_priority--;
               scheduler->add_process(proc);
               // CURRENT_RUNNING_PROCESS->burst_remaining -= timeinprevstate;
               // proc->ready_cpu_waiting_start = CURRENT_TIME;
               
               CURRENT_RUNNING_PROCESS = nullptr;

               CALL_SCHEDULER = true;
               break;}

          case STATE_DONE:
               {if (VERBOSE){cout<<CURRENT_TIME<<" "<<evt->process_id<<" "<<timeinprevstate<<": "
               <<STATES[evt->oldstate]<<" -> "<<STATES[evt->newstate]<<endl;}

               proc->finishing_time = CURRENT_TIME;
               CURRENT_RUNNING_PROCESS = nullptr;


               CALL_SCHEDULER = true;
               break;}
          }

          delete evt;

          if (CALL_SCHEDULER)
          {
               if (eventqs.get_next_event_time() == CURRENT_TIME){ continue;}

               CALL_SCHEDULER = false;
               if (CURRENT_RUNNING_PROCESS == nullptr)
               {
                    CURRENT_RUNNING_PROCESS = scheduler->get_next_process();
                    if (CURRENT_RUNNING_PROCESS == nullptr){ continue;}
                    
                    Event *new_e = new Event();//processes[i].arrival_time , i, 1, 0); // transition to ready (1)
                    new_e->timestamp = CURRENT_TIME;
                    new_e->process_id = CURRENT_RUNNING_PROCESS->process_id;
                    new_e->newstate = STATE_RUNNING;
                    new_e->oldstate = STATE_READY;
                    new_e->event_create_time = CURRENT_TIME;
                    proc->ready_cpu_waiting_start = CURRENT_TIME;

                    eventqs.put_event(new_e); // same here
               }
          }
     }

     cout<<printing<<endl;
     for (size_t i = 0; i < processes.size(); i++)
     {
          printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
          i, processes[i].arrival_time, processes[i].total_cpu, processes[i].cb, processes[i].io,
          processes[i].PRIO, processes[i].finishing_time, processes[i].finishing_time -processes[i].arrival_time,
          processes[i].full_io_time, processes[i].cpu_waiting);
	     //   prid,
	     //   arrival, totaltime, cpuburst, ioburst, static_prio,
	     //   state_ts, // last time stamp
	     //   state_ts - arrival,
	     //   iowaittime,
	     //   cpuwaittime);
          maxfintime = max(processes[i].finishing_time, maxfintime);
          total_cpu += processes[i].total_cpu;
          total_cpu_waiting += processes[i].cpu_waiting;
          total_turnaround +=  processes[i].finishing_time -processes[i].arrival_time;
     }


     double cpu_util = ((double)total_cpu/(double)maxfintime)*100;
	double io_util  = ((double)total_io/(double)maxfintime)*100;
	double avg_turnaround = ((double)total_turnaround/(double)processes.size());
	double avg_waittime = ((double)total_cpu_waiting/(double)processes.size());
	double throughput = ((double)processes.size()*100/(double)maxfintime) ;





     printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", maxfintime, cpu_util, io_util,
                                                            avg_turnaround, avg_waittime, throughput);

     return 0;
}

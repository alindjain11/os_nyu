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



class IO
{
public:
     int time_step;
     int track;
     int arrival_time;
     int start_time;
     int end_time;
     int req_no;

     IO(int time, int trk)
     {
          this->time_step = time;
          this->track = trk;
     }
};

deque <IO *> IODQ;
// deque <IO *> comp;

class IOScheduler
{
       // dequeue or Vector
  public:
     std::deque <IO *> IOQueue;
     // string SchedType; // this will be FIFO


     virtual void addToQ(IO *) = 0;
     virtual IO *get_next_operation() = 0;
     virtual bool all_empty() = 0;

     
};


class FIFO : public IOScheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:
  string SchedType = "FIFO";

  void addToQ(IO *p)
  {
       IOQueue.push_back(p);
  }

  IO *get_next_operation()
  {
       IO *p;
       if (IOQueue.empty()){return nullptr;}
       else 
       {
          p = IOQueue.front();
          IOQueue.pop_front();
          return p;
       }
  }

  bool all_empty()
  {
       return IOQueue.empty();
  }

};

const char * filename;
ifstream filebuff;
string buffer;
int CURRENT_TIME = 0;
int CURRENT_TRACK = 0;
int direction = 0;
int tot_movement = 0;
IO *ACTIVE_IO;


class SSTF : public IOScheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:
  string SchedType = "SSTF";

  void addToQ(IO *p)
  {
       IOQueue.push_back(p);
  }

  IO *get_next_operation()
  {
     //   IO *p;
       if (IOQueue.empty()){return nullptr;}
       else 
       {  
          int position = 0;
          int min_distance = 1000000;
          for (int i = 0; i < IOQueue.size(); i++)
          {
               IO *p = IOQueue[i];
               int distance = abs(CURRENT_TRACK - p->track);
               if(distance<min_distance)
               {
                    min_distance = distance;
                    position = i;
               }
               
          }
          IO *p = IOQueue[position];
          IOQueue.erase(IOQueue.begin() + position);
          // IOQueue.pop_front();
          return p;
       }
  }

  bool all_empty()
  {
       return IOQueue.empty();
  }

};

class LOOK : public IOScheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:
  string SchedType = "LOOK";

  void addToQ(IO *p)
  {
       IOQueue.push_back(p);
  }

  IO *get_next_operation()
  {
     //   IO *p;
       if (IOQueue.empty()){return nullptr;}
       else 
       {  
          int position = 0;
          int min_distance = 1000000;
          for (int i = 0; i < IOQueue.size(); i++)
          {
               IO *p = IOQueue[i];
               // int distance = abs(CURRENT_TRACK - p->track);
               int distance = p->track - CURRENT_TRACK;
               if (direction == 1 && distance >= 0)
               {
                    if(distance<min_distance)
                    {
                         min_distance = distance;
                         position = i;
                    }
               }
               else if (direction == -1 && distance <= 0)
               {
                    distance = distance * -1;
                    if(distance<min_distance)
                    {
                         min_distance = distance;
                         position = i;
                    }
               }
               
          }
          if (min_distance == 1000000)
          {
               if (direction == 1)
               {
                    direction = -1;
               }
               else{ direction = 1;}

               return this->get_next_operation();
          }

          IO *p = IOQueue[position];
          IOQueue.erase(IOQueue.begin() + position);
          // IOQueue.pop_front();
          return p;
       }
  }

  bool all_empty()
  {
       return IOQueue.empty();
  }

};

class CLOOK : public IOScheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:
  string SchedType = "CLOOK";

  void addToQ(IO *p)
  {
       IOQueue.push_back(p);
  }

  IO *get_next_operation()
  {
     //   IO *p;
       if (IOQueue.empty()){return nullptr;}
       else 
       {  
          int position = 0;
          int min_distance = 1000000;
          for (int i = 0; i < IOQueue.size(); i++)
          {
               IO *p = IOQueue[i];
               // int distance = abs(CURRENT_TRACK - p->track);
               int distance = p->track - CURRENT_TRACK;
               if (distance >= 0)
               {
                    if(distance<min_distance)
                    {
                         min_distance = distance;
                         position = i;
                    }
               }
               // else if (direction == -1 && distance <= 0)
               // {
               //      distance = distance * -1;
               //      if(distance<min_distance)
               //      {
               //           min_distance = distance;
               //           position = i;
               //      }
               // }
               
          }
          if (min_distance == 1000000)
          {    
               for (int i = 0; i < IOQueue.size(); i++)
               {
                    IO *p = IOQueue[i];
                    
                    if (p->track <= CURRENT_TRACK)
                    {
                         int distance = p->track;
                         if (distance < min_distance)
                         {
                              min_distance = distance;
                              position = i;
                         }
                    }
               }
               // CURRENT_TRACK = 0;
               // this->get_next_operation();
               // direction = 1;

          }

          IO *p = IOQueue[position];
          IOQueue.erase(IOQueue.begin() + position);
          // IOQueue.pop_front();
          return p;
       }
  }

  bool all_empty()
  {
       return IOQueue.empty();
  }

};

class FLOOK : public IOScheduler
{
     // deque <Process *> runQ;  // dequeue or Vector
  public:
     // deque <IO *> *activeQueue = new deque <IO *>(0);
     // deque <IO *> *addQueue = new deque <IO *>(0);
     deque <IO *> activeQueue;
     deque <IO *> addQueue ;
     void swapQueues()
     {
          deque<IO *> temp = activeQueue;
          activeQueue = addQueue;
          addQueue = temp;
     }
  string SchedType = "FLOOK";

  void addToQ(IO *p)
  {  
     //   cout<<"ADD"<<endl;
       addQueue.push_back(p);
     //   if(activeQueue.empty()){swapQueues();}

  }

  IO *get_next_operation()
  {
     //   IO *p;
     // cout<<"GET"<<endl;
     if (activeQueue.empty()){swapQueues();}

       if (activeQueue.empty()){return nullptr;}
       else 
       {  
          int position = 0;
          int min_distance = 1000000;
          for (int i = 0; i < activeQueue.size(); i++)
          {
               IO *p = activeQueue.at(i);
               // int distance = abs(CURRENT_TRACK - p->track);
               int distance = p->track - CURRENT_TRACK;
               if (direction == 1 && distance >= 0)
               {
                    if(distance<min_distance)
                    {
                         min_distance = distance;
                         position = i;
                    }
               }
               else if (direction == -1 && distance <= 0)
               {
                    distance = distance * -1;
                    if(distance<min_distance)
                    {
                         min_distance = distance;
                         position = i;
                    }
               }
               
          }
          if (min_distance == 1000000)
          {
               if (direction == 1)
               {
                    direction = -1;
               }
               else{ direction = 1;}

               return this->get_next_operation();
          }

          IO *p = activeQueue.at(position);
          activeQueue.erase(activeQueue.begin() + position);
          // IOQueue.pop_front();
          return p;
       }
  }

  bool all_empty()
  {  
     //   cout<<"EMPTY"<<endl;
     // cout<<"ACTIVE "<<activeQueue->empty()<<endl;
     // cout<<"ADD "<<addQueue.empty()<<endl;
       if (activeQueue.empty() && addQueue.empty()){;return true;}
       else
       {
          //   swapQueues();
          // cout<<"NOT EMPTY"<<endl;
            return false;
       }
  }

};


string get_buffer()
{
  if(buffer[0] == '#')
  {
    getline(filebuff,buffer);
    return get_buffer();
  }
  return buffer;
}

int main(int argc, char *argv[])
{    
     IOScheduler *scheduler;

     bool VERBOSE = false;
     bool QFLAG = false;
     bool FFLAG = false;

     bool CALL_SCHEDULER = true;


     int opt; 
     string algorithm;
      
    // put ':' in the starting of the 
    // string so that program can  
    //distinguish between '?' and ':'  
    while((opt = getopt(argc, argv, "vqfs:")) != -1)  
    {  
        switch(opt)  
        {  
          case 'v':
               VERBOSE = true;
          // printf("option verbose: %c\n", opt);

               break;  
          case 'q':  
               QFLAG = true;
          case 'f':
               FFLAG = true;
          
          case 's':
                    algorithm = string(optarg);
                    if (algorithm[0] == 'i'){scheduler = new FIFO();break;}
                    if (algorithm[0] == 'j'){scheduler = new SSTF();break;}
                    if (algorithm[0] == 's'){scheduler = new LOOK();break;}
                    if (algorithm[0] == 'c'){scheduler = new CLOOK();break;}
                    if (algorithm[0] == 'f'){scheduler = new FLOOK();break;}
                    
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
      

     filebuff.open(argv[optind]);
     int number_of_req = 0;
     while(getline(filebuff, buffer))
     {
          string sq = get_buffer();
          // cout<<sq<<endl;
          stringstream ss1(sq);
          int time;
          int trk;
          ss1 >> time >> trk;

          IO *ioreq =   new IO(time, trk);
          ioreq->req_no = number_of_req;
          IODQ.push_back(ioreq);
          number_of_req++;
          
     }
     // cout<<IODQ.size()<<endl;
     // cout<<IODQ.back()->req_no<<endl;
     // for (int i = 0; i<IODQ.size();i++)
     // {
     //      IO *p = IODQ[i];
     //      cout<<p->req_no<<" "<<p->time_step<<" "<<p->track<<endl;
     // }
     // exit(0);

     // VERBOSE = true;
     // scheduler = new FIFO();
     // scheduler = new SSTF();

     int sizze = IODQ.size();

     deque <IO *> comp = IODQ;

     while(CALL_SCHEDULER)
     {
          if (!IODQ.empty())
          {
               if (IODQ.front()->time_step == CURRENT_TIME)
               {
                    IO *request = IODQ.front();
                    IODQ.pop_front();
                    // cout<<"SIZE "<<IODQ.size()<<endl;
                    scheduler->addToQ(request);
                    if (VERBOSE){printf("%5d: %5d adds %5d\n", CURRENT_TIME, request->req_no, request->track);}

               }
          }

          if ((ACTIVE_IO != nullptr) && (CURRENT_TRACK == ACTIVE_IO->track))
          {
               ACTIVE_IO->end_time = CURRENT_TIME;
               // ACTIVE_IO = nullptr;
               
               // comp.push_back(ACTIVE_IO);
               ACTIVE_IO = nullptr;
          }

          if ((ACTIVE_IO != nullptr) && (CURRENT_TRACK != ACTIVE_IO->track))
          {
               if (CURRENT_TRACK > ACTIVE_IO->track)
               {
                    CURRENT_TRACK--;
                    direction = -1;  //Down
               }
               else
               {
                    CURRENT_TRACK++;
                    direction = 1;  // UP
               }
               tot_movement++;
          }

          if ((ACTIVE_IO == nullptr) && (!scheduler->all_empty()))
          {
               ACTIVE_IO = scheduler->get_next_operation();
               ACTIVE_IO->start_time = CURRENT_TIME;
               continue;
               
          }


          if ((IODQ.empty()) && (scheduler->all_empty()) && (ACTIVE_IO == nullptr) )
          {
               CALL_SCHEDULER = false;
               continue;
               // break;
          }

          // cout<<CURRENT_TIME<<endl;
          // cout<<IODQ.empty()<<"  "<< scheduler->all_empty()<< "  "<< (ACTIVE_IO == nullptr)<<endl;
          CURRENT_TIME++;
          
     }

     // cout<<"EXIT SIM "<<comp.size()<<endl;
     
     double turnaround = 0;
     double total_wait_time = 0;
     int max_waittime = 0;
     int waittime = 0;
     for (int i = 0; i < comp.size(); i++)
     {
          IO *io = comp[i];
          printf("%5d: %5d %5d %5d\n",i,io->time_step,io->start_time,io->end_time);
          turnaround += io->end_time - io->time_step;
          waittime = io->start_time - io->time_step;
          total_wait_time += waittime;
          max_waittime = max(max_waittime, waittime);
     }

     double avg_turnaround = turnaround/comp.size();
     double avg_waittime = total_wait_time/comp.size();
     printf("SUM: %d %d %.2lf %.2lf %d\n", CURRENT_TIME, tot_movement, avg_turnaround, avg_waittime, max_waittime);
     // cout<<algorithm;
}


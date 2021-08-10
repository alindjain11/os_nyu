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
char operation;
int vpage=0;


struct frame {
  int pid = -1;
  int vpage = -1;

  // other stuff you probably want to add
  bool mapped = false;
  int index;
  unsigned int age : 32; // idk why this is 32 bit might not need to be
  unsigned int timestamp = 0;

};

frame *FRAMETABLE;
int MAX_FRAMES;
deque <int> *freepool;
unsigned long instructions = 0;


struct pte { // needs to be 32 bits
  unsigned int present : 1;
  unsigned int referenced : 1;
  unsigned int modified : 1;
  unsigned int writeProtect : 1;
  unsigned int pagedOut : 1;
  unsigned int frameIndex : 7;
  unsigned int isFileMapped : 1;
  unsigned int padding : 19;
};

struct VMA {
  int startingVirtualPage; // example: 15
  int endingVirtualPage; // example: 63
  bool writeProtected; // example: True
  bool fileMapped; // example: False
};

const int PT_SIZE = 64;

class Process {
  public:
  int pid;
  deque<VMA> vmas;

  pte pageTable[PT_SIZE]; // PT_SIZE = 64

  unsigned long unmaps = 0;
  unsigned long maps = 0;
  unsigned long ins = 0;
  unsigned long outs = 0;
  unsigned long fins = 0;
  unsigned long fouts = 0;
  unsigned long zeroes = 0;
  unsigned long segv = 0;
  unsigned long segprot = 0;

  Process(int pid)
  {
     this->pid = pid;
     for (int i =0; i<PT_SIZE; i++)
     {
          pageTable[i].present = 0;
          pageTable[i].referenced = 0;
          pageTable[i].modified = 0;
          pageTable[i].writeProtect = 0;
          pageTable[i].pagedOut = 0;
          pageTable[i].frameIndex = 0;
          pageTable[i].isFileMapped = 0;
     }
  }
  void addVMA(VMA vma) { vmas.push_back(vma); }
  VMA *getCorrespondingVMA(int vpage)
  {
       for (int i = 0; i<vmas.size(); i++)
       {
            if(vmas[i].startingVirtualPage <= vpage && vmas[i].endingVirtualPage >= vpage)
            {
                 return &vmas[i];
            }
       }
       return nullptr;
  }
  void initializePageTable() // just calls reset_pte for all entries in pageTable  
  {
       
  }
};


ifstream filebuff;
string buffer;
std::deque <Process> processes;

string get_buffer()
{
  if(buffer[0] == '#')
  {
    getline(filebuff,buffer);
    return get_buffer();
  }
  return buffer;
}

bool get_next_instructions(char *operatio, int *vpag)
{    
     while(getline(filebuff,buffer))
     {
          // if (buffer.empty()){return false;}
     if(buffer[0] == '#'){ continue;}
          char op;
          int vp;
          stringstream sst(buffer);
          sst>>op>>vp;
          *operatio = op;
          *vpag = vp;
          return true;
     }
     return false;
}



struct Token {
     char *token;
    int line , lineofs;
};

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

const char * random_f;
std::deque <int> randoms;
static int ofs = 0;

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
     return (randoms[ofs++] % burst);
}


class Pager
{
public:

     virtual frame *select_victim_frame() = 0;

};

class FIFO : public Pager
{
public:
     int hand;
     FIFO(){int hand = 0;}

     frame * select_victim_frame()
     {
          frame* victim = &FRAMETABLE[hand];
          hand = (hand+1)%MAX_FRAMES;
          // cout<<"VICTIM FRAME"<<endl;
          return victim;
     }

};


class Clock : public Pager
{
public:
     int hand;
     Clock()
     {hand = 0;}

     frame * select_victim_frame()
     {
          while (true)
          {
               if(processes[FRAMETABLE[hand].pid].pageTable[FRAMETABLE[hand].vpage].referenced == 0 )
               {
                    frame *f = &FRAMETABLE[hand];
                    hand = (hand + 1)%MAX_FRAMES;
                    return f;
               }
               else
               {
                    processes[FRAMETABLE[hand].pid].pageTable[FRAMETABLE[hand].vpage].referenced = 0;
                    hand = (hand + 1)%MAX_FRAMES;
               }
          }
          
     }
};



class NRU : public Pager
{
public:
     int hand;
     int last_reset;
     NRU()
     {
          hand = 0;
          last_reset = 0;
     }

     const int interval = 50;
     int threshold = 49;

     bool exceedsThreshold(int instrCount)
     {
     if (instrCount >= this->threshold)
     {
          this->threshold = instrCount + this->interval;
          return true;
     }
     return false;
     }

     frame * select_victim_frame()
     {
          // deque <int> Class;
          frame *Class[4];
          bool reset = false;
          // int victim = -1;

          for (int i = 0; i<4;i++)
          {
               Class[i] = NULL;
          }

          // if (last_reset - instructions >= 50){reset = true; last_reset = instructions;}
          // if (instructions - last_reset >= 50){reset = true; last_reset = instructions;}
          reset = exceedsThreshold(instructions);
          
          for (int i = 0; i<MAX_FRAMES; i++)
          {
               int r = processes[FRAMETABLE[hand].pid].pageTable[FRAMETABLE[hand].vpage].referenced;
               int m = processes[FRAMETABLE[hand].pid].pageTable[FRAMETABLE[hand].vpage].modified;
               int class_index = 2*r + m;
               
               if (Class[class_index] == NULL)
               {
                    Class[class_index] = &FRAMETABLE[hand];
                    // if((class_index == 0 && !reset))
                    // {
                    //      victim = hand;
                    //      hand = (hand +1)%MAX_FRAMES;
                    //      return &FRAMETABLE[victim];
                    // } 
               }

               if (reset){processes[FRAMETABLE[hand].pid].pageTable[FRAMETABLE[hand].vpage].referenced = 0;}
               else if(class_index == 0){break;}
               
               hand = (hand + 1)%MAX_FRAMES;
          }

          // int victim;
          frame *victim = NULL;
          for (int i =0; i<4;i++)
          {
               if(Class[i]!= NULL)
               {
                    hand = (Class[i]->index +1) % MAX_FRAMES;
                    victim = Class[i];
                    // hand = (victim + 1)%MAX_FRAMES;
                    break;
               }
          }
          return victim;
     }
};

class Random : public Pager
{
public:
     Random(){}
     frame * select_victim_frame()
     {
          int victim = myrandom(MAX_FRAMES);
          frame *f = &FRAMETABLE[victim];
          return f;
     }

};

class WorkingSet : public Pager
{
public:

     int hand;
     WorkingSet(){hand = 0;}


     frame * select_victim_frame()
     {
          // int victim = hand;
          frame *oldestFrame = nullptr;
          for (int i = 0; i<MAX_FRAMES; i++)
          {
               frame *fte = &FRAMETABLE[hand];
               int time = instructions - fte->timestamp;
               
               // processes[fte->pid].pageTable[fte->vpage].referenced
               if (processes[fte->pid].pageTable[fte->vpage].referenced)
               {
                    fte->timestamp =  instructions;
                    processes[fte->pid].pageTable[fte->vpage].referenced = 0;
                    if (oldestFrame == nullptr) {oldestFrame = fte;}
               }
               else
               {
                    if (time>=50)
                    {    
                         oldestFrame = fte;
                         // victim = hand;
                         break;
                    }
                    else
                    {
                         if (oldestFrame == nullptr ||fte->timestamp < oldestFrame->timestamp){oldestFrame = fte;}
                    }
               }
               hand = (hand +1)%MAX_FRAMES;
               
          }
          // hand = (victim + 1)%MAX_FRAMES;
          hand = (oldestFrame->index + 1)%MAX_FRAMES;
          // return &FRAMETABLE[victim];
          return oldestFrame;
     }

};

class Ageing : public Pager
{
public:
     int hand;
     Ageing(){hand = 0;}

     frame * select_victim_frame()
     {
          frame *frameWithLowestAge = nullptr;
          for (int i = 0; i<MAX_FRAMES; i++)
          {
               frame *fte = &FRAMETABLE[hand];
               FRAMETABLE[hand].age = FRAMETABLE[hand].age >> 1;
               
               if (processes[fte->pid].pageTable[fte->vpage].referenced)
               {
                    FRAMETABLE[hand].age = (FRAMETABLE[hand].age | 0x80000000);
                    processes[fte->pid].pageTable[fte->vpage].referenced = 0;

               }
               if (frameWithLowestAge == nullptr || fte->age < frameWithLowestAge->age)
               {
                    frameWithLowestAge = fte;
               }
               hand = (hand +1)%MAX_FRAMES;
               
          }
          hand = (frameWithLowestAge->index +1)%MAX_FRAMES;
          return frameWithLowestAge;
     }
};




Pager * THE_PAGER;

frame *allocate_frame_from_freelist()
{
     if (!freepool->size()){return nullptr;}
     else
     {
          int i = freepool->front();
          freepool->pop_front();
          
          return &FRAMETABLE[i];
     }
}

frame *get_frame()
{
     frame *fte = allocate_frame_from_freelist();
     if (fte == nullptr){fte = THE_PAGER->select_victim_frame();}
     return fte;
}

Process *CURRENT_PROCESS = nullptr;
unsigned long context_switch = 0;
unsigned long long cost = 0;
unsigned long total_process_exits = 0;


int main(int argc, char *argv[])
{ 

     bool VERBOSE = false;
     bool FRAMETABLE_PRINT = false;
     bool PAGETBALE_PRINT = false;
     bool SUMMARY = false;
     
     int opt;
     string options;
     string algorithm;

     while((opt = getopt(argc,argv,"f:o:a:")) != -1)
     {
          switch(opt)
          {
               case 'f':
                    MAX_FRAMES = stoi(string(optarg));
                    break;
               
               case 'o':
                    options = string(optarg);
                    for (int i = 0; i<options.size();i++)
                    {
                         if (options[i] == 'O'){VERBOSE = true;}
                         if (options[i] == 'P'){PAGETBALE_PRINT = true;}
                         if (options[i] == 'F'){FRAMETABLE_PRINT = true;}
                         if (options[i] == 'S'){SUMMARY = true;}                
                    }
                    break;
               case 'a':
                    algorithm = string(optarg);
                    if (algorithm[0] == 'f'){THE_PAGER = new FIFO();break;}
                    if (algorithm[0] == 'e'){THE_PAGER = new NRU();break;}
                    if (algorithm[0] == 'c'){THE_PAGER = new Clock();break;}
                    if (algorithm[0] == 'r'){THE_PAGER = new Random();break;}
                    if (algorithm[0] == 'w'){THE_PAGER = new WorkingSet();break;}
                    if (algorithm[0] == 'a'){THE_PAGER = new Ageing();break;}
                    break;
                     

               default: exit(0);

          }
     }
     // cout<<MAX_FRAMES<<endl<<VERBOSE<<endl<<PAGETBALE_PRINT<<endl<<FRAMETABLE_PRINT<<endl<<SUMMARY<<endl;
     // exit(0);
     if (optind < argc)
     {    filebuff.open(argv[optind]);
          // filename = argv[optind];
          // printf("filename %s\n", filename);
          random_f = argv[++optind];
          // printf("randfile %s\n", random_f);
     }

     random_file(random_f);
     // cout<<randoms.size();

     // MAX_FRAMES = 16;
     FRAMETABLE = new frame[MAX_FRAMES];
     freepool = new deque<int>;
     for (int i = 0; i<MAX_FRAMES; i++)
     {
          FRAMETABLE[i].index = i;
          FRAMETABLE[i].pid = -1;
          FRAMETABLE[i].vpage = -1;
          FRAMETABLE[i].mapped = 0;
          freepool->push_back(i);
     }



     int numprocess;
     // int numvmas;
     getline(filebuff, buffer);
     
     numprocess = stoi(get_buffer());
     
     for(int i=0; i<numprocess; i++)
     {
          Process p(i);
          // p.pid = i;
          getline(filebuff, buffer);
          int numvmas = stoi(get_buffer());
          // cout<<"VMAS "<<numvmas<<endl;
          for(int i=0; i<numvmas; i++)
          {    getline(filebuff, buffer);
               // cout<<buffer<<endl;
               string last = get_buffer();
               // cout<<last<<endl;
               stringstream ss(last);
               int startingVirtualPage = 0; // example: 15
               int endingVirtualPage = 0; // example: 63
               bool writeProtected = 0; // example: True
               bool fileMapped = 0;

               ss >> startingVirtualPage >> endingVirtualPage >> writeProtected >> fileMapped;
               // cout<<startingVirtualPage <<" "<< endingVirtualPage <<" "<< writeProtected <<" "<< fileMapped<<endl;
               struct VMA vma = {startingVirtualPage, endingVirtualPage, writeProtected, fileMapped};
               p.addVMA(vma);
               // cout<<startingVirtualPage <<" "<< endingVirtualPage <<" "<< writeProtected <<" "<< fileMapped<<endl;
               
          }
          // cout<<"VMAS "<<p.vmas.size()<<endl;
          processes.push_back(p);
          
          
     }
     
     for(instructions; get_next_instructions(&operation,&vpage); instructions++)
     {    
          if (VERBOSE){cout<<instructions<<": ==> "<<operation<<" "<<vpage<<endl;}

          if (operation == 'c')
          {
               CURRENT_PROCESS = &processes[vpage];
               context_switch++;
               continue;
          }
          else if(operation == 'e')
          {
               cout<<"EXIT current process "<<CURRENT_PROCESS->pid<<endl;
               total_process_exits++;
               for (int i = 0; i<PT_SIZE;i++)
               {
                    pte *p = &CURRENT_PROCESS->pageTable[i];
                    if (p->present)
                    {    
                         if (VERBOSE){cout<<" UNMAP "<< CURRENT_PROCESS->pid<<":"<<i<<endl;}
                         CURRENT_PROCESS->unmaps++;

                         if (p->modified && p->isFileMapped)
                         {
                              CURRENT_PROCESS->fouts++;
                              if (VERBOSE){cout<<" FOUT"<<endl;}
                         }

                         FRAMETABLE[p->frameIndex].pid = -1;
                         FRAMETABLE[p->frameIndex].vpage = -1;
                         FRAMETABLE[p->frameIndex].mapped = 0;
                         freepool->push_back(FRAMETABLE[p->frameIndex].index);

                         // p->present = 0;
                         // p->referenced = 0;
                         // p->modified = 0;
                         // p->writeProtect = 0;
                         // p->pagedOut = 0;
                         // // p->frameIndex = 0;
                         // p->isFileMapped = 0;
                         

                    }
                    p->present = 0;
                    p->referenced = 0;
                    p->modified = 0;
                    p->writeProtect = 0;
                    p->pagedOut = 0;
                    // p->frameIndex = 0;
                    p->isFileMapped = 0;
               }
               CURRENT_PROCESS = nullptr;
               continue;
          }
          else
          {
               pte *pt = &CURRENT_PROCESS->pageTable[vpage];
               // cout<<"PRESENT "<<pt->present<<endl;
               // cout<<"SIZE OF PTE "<<sizeof(pte)<<endl;
               if ( !pt->present)
               {
                    // cout<<"NOT PRESENT"<<endl;
                    VMA *vma_t;
                    // vpage = 64;
                    vma_t = CURRENT_PROCESS->getCorrespondingVMA(vpage);
                    // cout<<vma_t->startingVirtualPage<<" "<<vma_t->endingVirtualPage<<endl;
                    if (vma_t == nullptr)
                    {
                         // cout<<"does not exists"<<endl;
                         if (VERBOSE){cout<<" SEGV"<<endl;}
                         CURRENT_PROCESS->segv++;
                         continue;

                    }
                    else
                    {
                         // cout<<"exists"<<endl;
                         pt->isFileMapped = vma_t->fileMapped;
                         pt->writeProtect = vma_t->writeProtected;
                    
                    }
                    frame * frame_t = get_frame();
                    
                    // cout<<"FRAME "<<frame_t->mapped<<endl;
                    // cout<<(frame_t->vpage == -1)<<endl;
                    // exit(0);
                    if(frame_t->mapped)
                    {
                         if (VERBOSE){cout<<" UNMAP "<< frame_t->pid<<":"<<frame_t->vpage<<endl;}
                         processes[frame_t->pid].unmaps++;


                         pte *pt2 = &processes[frame_t->pid].pageTable[frame_t->vpage];
                         if (pt2->modified == 1)
                         {
                              // cout<<"Inside Modified"<<endl;
                              if(pt2->isFileMapped)
                              {
                                   if (VERBOSE){cout<<" FOUT"<<endl;}
                                   processes[frame_t->pid].fouts++;
                              }
                              else
                              {
                                   if (VERBOSE){cout<<" OUT"<<endl;}
                                   processes[frame_t->pid].outs++;
                                   pt2->pagedOut = 1;

                              }
                         }
                         // wipe pt2
                    pt2->frameIndex = 0;
                    pt2->present = 0;
                    pt2->modified = 0;
                    pt2->referenced = 0;
                    } 
                    if (pt->pagedOut)
                    {
                         if (VERBOSE){cout<<" IN"<<endl;}
                         CURRENT_PROCESS->ins++;
                    }
                    else if (pt->isFileMapped)
                    {
                         if (VERBOSE){cout<<" FIN"<<endl;}
                         CURRENT_PROCESS->fins++;
                    }
                    else
                    {
                         if (VERBOSE){cout<<" ZERO"<<endl;}
                         CURRENT_PROCESS->zeroes++;    
                    }
                    // if (pt->isFileMapped)
                    // {
                    //      cout<<" FIN"<<endl;
                    //      CURRENT_PROCESS->fins++;
                    // }
                    // // if (!pt->isFileMapped && pt->pagedOut)
                    // if (pt->pagedOut)
                    // {
                    //      cout<<" IN"<<endl;
                    //      CURRENT_PROCESS->ins++;
                    // }
                    // if (!pt->isFileMapped && !pt->pagedOut)
                    // {
                    //      cout<<" ZERO"<<endl;
                    //      CURRENT_PROCESS->zeroes++;
                    // }
                    pt->frameIndex = frame_t->index;
                    pt->present = 1;

                    frame_t->pid = CURRENT_PROCESS->pid;
                    frame_t->vpage = vpage;
                    frame_t->timestamp = instructions;
                    frame_t->age = 0;

                    frame_t->mapped = 1;
                    if (VERBOSE){cout<<" MAP "<<pt->frameIndex<<endl;}
                    CURRENT_PROCESS->maps++;
                    
                    // pt->referenced = 1;
               } 
               if (operation == 'r')
                    {
                         pt->referenced = 1;
                    }
               else if(operation == 'w')
               // if (operation == 'w')
               {
                    
                    pt->referenced = 1;
                    if(pt->writeProtect)
                    {
                         if (VERBOSE){cout<<" SEGPROT"<<endl;}
                         CURRENT_PROCESS->segprot++;

                    }
                    else{
                         // cout<<"modified"<<endl;
                    pt->modified = 1;}
               }
               // if (operation == '')
          }
     }

     if (PAGETBALE_PRINT)
     {
          for (int i = 0; i<processes.size();i++)
          {
               cout<<"PT["<<i<<"]: ";
               for (int x = 0; x<PT_SIZE;x++)
               {
                    pte *pt_p = &processes[i].pageTable[x];
                    if(pt_p->present)
                    {
                         cout<<x<<":";
                         if (pt_p->referenced)
                         {
                              cout<<"R";
                         }
                         else{cout<<"-";}

                         if(pt_p->modified){cout<<"M";}
                         else{cout<<"-";}

                         if(pt_p->pagedOut){cout<<"S ";}
                         else{cout<<"- ";}

                    }
                    else
                    {
                         if(pt_p->pagedOut)
                         {
                              cout<<"# ";
                         }
                         else{cout<<"* ";}
                    }
               }
               cout<<endl;
          }
     }

     if (FRAMETABLE_PRINT)
     {
          cout<<"FT: ";
          for (int i = 0; i<MAX_FRAMES;i++)
          {
               if (FRAMETABLE[i].mapped)
               {
                    cout<<FRAMETABLE[i].pid<<":"<<FRAMETABLE[i].vpage<<" ";
               }
               else{cout<<"* ";}
          }
          cout<<endl;
     }

     if (SUMMARY)
     {
          for (int i = 0; i< processes.size();i++)
          {
               printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
          i,processes.at(i).unmaps,
          processes.at(i).maps,
          processes.at(i).ins,
          processes.at(i).outs,
          processes.at(i).fins,
          processes.at(i).fouts,
          processes.at(i).zeroes,
          processes.at(i).segv,
          processes.at(i).segprot);

          cost += processes.at(i).unmaps * 400;
          cost += processes.at(i).maps * 300;
          cost += processes.at(i).ins * 3100;
          cost += processes.at(i).outs * 2700;
          cost += processes.at(i).fins * 2800;
          cost += processes.at(i).fouts * 2400;
          cost += processes.at(i).zeroes * 140;
          cost += processes.at(i).segv * 340;
          cost += processes.at(i).segprot * 420;
          }

          unsigned long r_w = instructions - context_switch - total_process_exits;
          cost += context_switch * 130;
          cost += total_process_exits * 1250;
          cost += r_w;
          printf("TOTALCOST %lu %lu %lu %llu %lu\n", instructions, context_switch, total_process_exits, cost, sizeof(pte));
     }

}













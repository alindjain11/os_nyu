#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <string.h>
#include <deque>
#include <iomanip>
#include <ios>

using namespace std;


FILE *infile;

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


void __parseError(const int errcode, const unsigned int lineNo, const unsigned int offset)
{
  static string errstr[] = {

     "NUM_EXPECTED",              // Number expect : 0
     "SYM_EXPECTED",               // Symbol Expected : 1
     "ADDR_EXPECTED",              // Addressing Expected which is A/E/I/R : 2
     "SYM_TOO_LONG",               // Symbol Name is too long : 3
     "TOO_MANY_DEF_IN_MODULE",     // > 16 : 4
     "TOO_MANY_USE_IN_MODULE",     // > 16 : 5
     "TOO_MANY_INSTR",             // total num_instr exceeds memory size (512) : 6
  };
  printf("Parse Error line %u offset %u: %s\n", lineNo, offset, errstr[errcode].c_str());
  exit(1);
}


char *getToken()
{
     static bool newline = 1; // you must read a new line
     char delimeter[] = "\n \t";
     size_t BUFSZ = 1024;
     static char *buffer; // large buffer that must be static because we reuse across calls.
     char *tok = NULL;
     ssize_t sz;

     while (1) {
       while (newline)  {
         sz = getline(&buffer, &BUFSZ, infile);
         if (sz == -1)
              return NULL; // eof
         // now lets seed the tokenizer
         tok = strtok(buffer,delimeter);
         if (tok != NULL) {
              newline = 0;  // next time we call getToken we don't need to call a new line
              return tok;
         }
         // looks like an empty line so read another
      }
      tok = strtok(NULL, delimeter); // note we use NULL to read from the same line
      if (tok == NULL) {
           newline = 1;  // next time we call getToken we don't need to call a new line
           continue;     // forces reading a new line
      }

      return tok;
}}



Token readInt(bool newSection = 0)
{
  Token tok = findtoken();
  const char *token = tok.token;
//   cout<<tok.token<<endl;
  if (token == NULL && newSection == 1) 
  {
    
    return tok;
  }
  if (token == NULL) 
  {
    __parseError(0, tok.line, tok.lineofs);
  }
  for (size_t i = 0; i < strlen(token); i++)
  {
    if (!isdigit(token[i])) 
    {
      __parseError(0, tok.line, tok.lineofs);
    }
  }

  return tok;
}

Token readSymbol()
{
  Token tok = findtoken();
  const char *token = tok.token;

  if (token == NULL || !isalpha(token[0]))
  {
    __parseError(1, tok.line, tok.lineofs);
  }
  if (strlen(token) > 16)
  {
    __parseError(3, tok.line, tok.lineofs);
  }
  if (strlen(token) > 1)
  {
    for (size_t i = 1; i < strlen(token); i++)
    {
      if (!isalnum(token[i]))
      {
        __parseError(1, tok.line, tok.lineofs);
      }
    }
  }

  return tok;
}

Token readIEAR()
{
     Token tok = findtoken();
     // return tok;
     const char *token = tok.token;
     // cout<<token<<endl;
     // string s(token);
     // string s = token;
     // cout<<s<<endl;
     if (token == NULL )//|| s.length() > 1)
     {
     __parseError(2, tok.line, tok.lineofs);
     }

     char iaer = *token;
     if (iaer != 'I' && iaer != 'A' && iaer != 'E' && iaer != 'R' )
     {
     __parseError(2, tok.line, tok.lineofs);
     }
     
     return tok;
}



class Symbol
{
public:
     bool redefine = 0;
     int module = -1;
     bool used = 0;
     string value;

};

class Module
{
public:
     int base_add;
     int count;
     int len = 0;

     deque <Symbol> uselist;
     deque <int> Usedsym;
     Module()
     {
          base_add = 0;
          count = 0;
     }
     void insert(Symbol sym)
     {
          uselist.push_back(sym);
          Usedsym.push_back(0);
     }
};


deque <Symbol> symbols;
deque <int> address_symbols;

int symAddress(Symbol sym){
  for(int i = 0; i<symbols.size();i++){
    if(sym.value==symbols[i].value){
      return address_symbols[i];
    }
  }
  return -1;
}

void pass2();


int main(int argc, char* argv[])
{
  // ifstream infile(argv[1]);e
     infile = fopen(argv[1],"r");
     deque <Module> modules;
     Token toking;
     while (!feof(infile))
     {
          toking = readInt(1);
          if (feof(infile)){break;}
          Module m1;
          int n = modules.size();
          if (n>0)
          {
               int base = modules[n-1].base_add + modules[n-1].count;
               m1.base_add = base;
          }
          int defcount = atoi(toking.token);
          if (defcount > 16)
          {
               __parseError(4, toking.line, toking.lineofs);
          }

          for (int i = 0; i< defcount; i++)
          {
               toking = readSymbol();
               Symbol sym;
               sym.value = toking.token;
               // Symbol symb = Symbol(toking.token);
               // string symbol(toking.token);
               toking = readInt();
               int rel_add = atoi(toking.token);
               int abs_add = rel_add + m1.base_add;
               int address = symAddress(sym);
               for(int i =0; i<symbols.size();i++)
               {
                    if(symbols[i].value == sym.value)
                    {//we already have in sym table
                         Symbol symb = symbols[i];
                         symb.redefine = 1;
                         symbols[i] = symb;
	               }
               }
               if(symAddress(sym)==-1)
               {
                    sym.module = n+1;
                    symbols.push_back(sym);        
                    address_symbols.push_back(abs_add);  
               }
          }
          toking = readInt();
          int usecount = atoi(toking.token);
          if (usecount > 16)
          {
               __parseError(5, toking.line, toking.lineofs);
          }
          for (int i = 0; i<usecount; i++)
          {
               toking = readSymbol();
               Symbol sym;
               sym.value = toking.token;
               m1.insert(sym);
          }
          toking = readInt();
          int code_count = atoi(toking.token);
          if ( code_count + m1.base_add >= 512)
          {
               __parseError(6, toking.line, toking.lineofs);
          }
          m1.count = code_count;
          for (int i = 0; i < code_count; i++)
          {
               toking = readIEAR();
               char address_mode = toking.token[0];
               toking = readInt();
               int instr = atoi(toking.token);

               int opcode = instr/1000;
               int operand = instr % 1000;
          }
          modules.push_back(m1);


          // cout<<toking.token<<endl;

     }
     fclose(infile);

     cout<<"Symbol Table"<<endl;
     for(int i = 0; i<symbols.size();i++){
     Symbol sym = symbols[i];
     int absAddress = address_symbols[i];
     if(sym.redefine){
          cout<<sym.value<<"="<<absAddress<<"  Error: This variable is multiple times defined; first value used"<<endl;
     }
     else{
          cout<<sym.value<<"="<<absAddress<<endl;
     }
     }
     cout<<endl;
     

     infile = fopen(argv[1],"r");
     // cout<<"START"<<endl;
     cout<< "Memory Map"<<endl;
     pass2();

     // exit(1);


}


void pass2()
{
     // infile = fopen(argv[1],"r");
     deque <Module> modules2;
     Token toking;
     int instruction_id = 0;
     while (!feof(infile))
     {
          toking = readInt(1);
          if (feof(infile)){break;}
          Module m1;
          int n = modules2.size();
          if (n>0)
          {
               int base = modules2[n-1].base_add + modules2[n-1].count;
               m1.base_add = base;
          }
          int defcount = atoi(toking.token);
          for (int i = 0; i< defcount; i++)
          {
               toking = readSymbol();
               Symbol sym;
               sym.value = toking.token;
               // Symbol symb = Symbol(toking.token);
               // string symbol(toking.token);
               toking = readInt();
               int rel_add = atoi(toking.token);
               int abs_add = rel_add + m1.base_add;
          }
          toking = readInt();
          int usecount = atoi(toking.token);
          
          for (int i = 0; i<usecount; i++)
          {
               toking = readSymbol();
               Symbol sym;
               sym.value = toking.token;
               m1.insert(sym);
          }
          toking = readInt();
          int code_count = atoi(toking.token);
          
          m1.count = code_count;

          for (int i = 0; i < code_count; i++)
          {
               toking = readIEAR();
               char address_mode = toking.token[0];
               toking = readInt();
               int instr = atoi(toking.token);

               int opcode = instr/1000;
               int operand = instr % 1000;
               string error_message = "";
               
               switch (address_mode)
               {
               case 'R':
               {
                    instr = instr + m1.base_add;
                    if(instr % 1000 >= m1.base_add + m1.count)
                    {
                         error_message = "Error: Relative address exceeds module size; zero used";
                         instr = opcode * 1000 + m1.base_add;
                    }
               break;}
               case 'E':
               {
                    if(operand >= usecount)
                    {
                         error_message = "Error: External address exceeds length of uselist; treated as immediate";
                    }

               else{
               Symbol sym = m1.uselist[operand];
               int address = symAddress(sym);
                    if(address==-1)
                    {  
                    error_message = "Error: " + sym.value + " is not defined; zero used";
                    address = 0;
                    
                    m1.Usedsym[operand] = 1;
                    }
                    else{
                    m1.Usedsym[operand]=1; 
                    for(int i =0 ;i<symbols.size();i++)
                    {
                         if(symbols[i].value==sym.value)
                         {
                         Symbol sym = symbols[i];
                         sym.used = 1;
                         symbols[i] = sym;
                         
                         }
                    }
               
               }
               instr = 1000*opcode+address;
               }
                    break;}
               case 'A':
               {
                    if(operand>=512)
                    {
                         instr = 1000*opcode;
                         error_message = "Error: Absolute address exceeds machine size; zero used";
                    }
               break;}
               case 'I':
               {
                    if(instr >=10000)
                    {
                         instr = 9999;
                         error_message = "Error: Illegal immediate value; treated as 9999"; 
                    }
               break;}
               }
               if(instr>9999){
               instr = 9999;
               error_message = "Error: Illegal opcode; treated as 9999";
               }
               string out_string = to_string(instruction_id);
               
               cout<< setfill('0') << setw(3) <<out_string<<": "<< setfill('0') << setw(4)<<instr<<" "<<error_message<<endl;
               
               
               instruction_id++;
          }
          for(int i =0 ;i<m1.Usedsym.size();i++)
          {
               if(!m1.Usedsym[i])
               {
                    cout<<"Warning: Module "<<n+1<<": "<<m1.uselist[i].value<<" appeared in the uselist "<<
                    "but was not actually used"<<endl;
               }
          }
          modules2.push_back(m1);
     }
          
          // exit(1);
          // cout<<"next"<<endl<<endl;
}

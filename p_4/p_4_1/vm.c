/** 
  Concurrent_Programming    

  @author Tzimotoudis Panagiotis
  @author Tziokas George
*/


/* Starting of executable MagicBeg:
        -> hex -> ( 0xDE 0xAD 0xBE 0xAF )
        -> int -> ( 222 173 190 175 )
   Starting of code MagicBody:
        -> hex -> ( 0xDE 0xAD 0xC0 0xDE )
        -> int -> ( 222 173 192 222 )
   Starting of task MagicTask:
        -> hex -> ( 0xDE 0xAD 0xBA 0xBE )
        -> int -> ( 222 173 186 190 )
   End of code MagicEnd:
        -> hex -> ( 0xFE 0xE1 0xDE 0xAD )
        -> int -> ( 254 225 222 173 )  */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define NumOfReg 32 // pc == reg31, idx == reg0
#define SWITCH 15

#define magicBeg "\xde\xad\xbe\xaf"
#define magicBody "\xde\xad\xc0\xde"
#define magicTask "\xde\xad\xba\xbe"
#define magicEnd "\xfe\xe1\xde\xad"

enum states { READY, BLOCKED, SLEEPING, STOPPED };

typedef struct LocalMemory {
  int id;
  enum states state;
  int reg[NumOfReg-2];  // Because pc and idx are registers too.  reg0: default 0
  int pc;   // pc == reg31
  int idx;  // idx == reg0
  int sem;
  int waket;
  char *localMem;
  struct LocalMemory *nxt;
  struct LocalMemory *prev;
}localMemory;

typedef struct GlobalMemory {
  char *globalMem;
  char *code;
  localMemory *tasks;
  int cur;
}globalMemory;

char *read_binary ( const char *file_name );

char *check_if_executable ( char *decimal_file );

globalMemory *decode_the_executable ( char *decimal_file );

localMemory *switch_task ( localMemory *head, globalMemory *globalMem );

void instruction_set ( localMemory *curr_task, globalMemory *globalMem );

int main ( int argc, char *argv[] ) {
  globalMemory *GlobalMem;
  localMemory *curr_task,*curr;
  char *executable_file;
  int regX,regY,addr,val,switchTask=0,yield=0,offset,i;

  executable_file = read_binary( argv[1] );
  if ( executable_file == NULL ) {
    return 1;
  }
  executable_file = check_if_executable ( executable_file );
  if ( executable_file == NULL ) {
    return 1;
  }

  GlobalMem = decode_the_executable ( executable_file );
  if ( GlobalMem == NULL ) {
    return 0;
  }

  curr_task = GlobalMem->tasks;
  GlobalMem->cur = curr_task->id;

  instruction_set ( curr_task, GlobalMem );

  return 0;
}

char *read_binary ( const char *file_name ) {
  FILE *input_file;
  char ch,*binary_file;
  int file_size,fseek_check,number,counter=0;

  input_file = fopen ( file_name, "rb" );

  if ( input_file == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  fseek_check = fseek ( input_file, 0L, SEEK_END );
  if ( fseek_check == -1 ){
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  file_size = ftell ( input_file );
  if ( file_size == -1 ){
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }
  fseek_check = fseek ( input_file, 0L, SEEK_SET );
  if ( fseek_check == -1 ){
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  binary_file = ( char * ) malloc ( sizeof ( char ) * file_size );
  if ( binary_file == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  while ( fread ( &ch, sizeof(char), 1, input_file ) != 0 ) {
    binary_file [ counter ] = ch;
    counter++;
  }
  return binary_file;
}

char *check_if_executable ( char *binary_file ) {

  if ( memcmp ( magicBeg, binary_file, 4*sizeof(char) ) == 0 ) {
    printf( "It's an executable file\n" );
    return binary_file;
  }else {
    printf( "Your file isn't an executable binary file...!!\n" );
    return NULL;
  }

  return NULL;
}

globalMemory *decode_the_executable ( char *executable ) {
  globalMemory *GlobalMem;
  localMemory *curr,*head;
  int NumOfBodies,TotCodeSize,i,j,first,curr_position_on_file;
  int NumOfTasks,codeSize,*startingCode,curr_code_size=0;
  int *bodyLocalSize,bodytask,k;
  char hex[5],hex2[3];

  curr_position_on_file = 4;

  GlobalMem = ( globalMemory * ) malloc ( sizeof ( globalMemory ) );

  // GlobalSize == Size of global memory
  GlobalMem->globalMem = ( char * ) malloc ( sizeof ( char ) * executable[curr_position_on_file] );
  curr_position_on_file++;

  // NumOfBodies == Number of task body procedures
  NumOfBodies = executable[curr_position_on_file];
  curr_position_on_file++;

  startingCode = ( int * ) malloc ( sizeof ( int ) * NumOfBodies );
  bodyLocalSize = ( int * ) malloc ( sizeof ( int ) * NumOfBodies );
  // TotCodeSize == Total code size of all bodies ( in bytes )
  sprintf ( hex, "%x", executable[curr_position_on_file] );
  curr_position_on_file++;
  sprintf ( hex2, "%x", executable[curr_position_on_file] );
  curr_position_on_file++;
  strcat ( hex, hex2 );
  TotCodeSize = ( int ) strtol ( hex, NULL, 16 );
  GlobalMem->code = ( char * ) malloc ( sizeof ( char ) * TotCodeSize );

  // NumOfTasks == Number of tasks
  NumOfTasks = executable[curr_position_on_file];

  for ( i=0,first=1; i<NumOfTasks; i++ ) {
    curr = ( localMemory * ) malloc ( sizeof ( localMemory ) );
    curr->id = i + 1;
    curr->state = 0;
    for ( j=0; j<NumOfReg-2; j++ ) {
      curr->reg[j] = j;
    }
    curr->pc = 0;
    curr->idx = 0;
    curr->sem = -1;
    curr->waket = -1;
    curr->localMem = NULL;
    if ( first ) {
      curr->nxt = curr;
      curr->prev = curr;
      head = curr;
      first = 0;
    }else {
      if ( ( head->nxt ) == head ) {
        head->nxt = curr;
        head->prev = curr;
        curr->nxt = head;
        curr->prev = head;
      }else {
        head->prev->nxt = curr;
        curr->prev = head->prev;
        curr->nxt = head;
        head->prev = curr;
      }
    }
  }
  GlobalMem->tasks = head;
  curr_position_on_file++;

  // GlobalInit == Initial values for global memory
  for ( i=0; i<executable[4]; i++ ) {
    GlobalMem->globalMem[i] = executable[curr_position_on_file];
    curr_position_on_file++;
  }

  // MagicBody == Start of code
  // LocalSize == Size of local memory for this body
  // bodyLocalSize == Size of local memory for each body
  for ( i=0; i<NumOfBodies; i++ ) {
    if ( memcmp ( magicBody, &executable[curr_position_on_file], 4*sizeof(char) ) == 0 ) {
      curr_position_on_file = curr_position_on_file + 4;
    }else {
      printf( "Your file isn't an executable binary file...!!\n" );
      for ( j=0; j<NumOfTasks; j++ ) {
        if ( j < NumOfTasks-1 ) {
          curr = head->nxt;
          free( head );
          head = curr;
        }else {
          free( head );
        }
      }
      free( GlobalMem->globalMem );
      free( GlobalMem->code );
      free( GlobalMem );

      return NULL;
    }

    startingCode[i] = curr_code_size;
    bodyLocalSize[i] = executable[curr_position_on_file];
    curr_position_on_file++;

    codeSize = executable[curr_position_on_file];
    curr_position_on_file++;

    for ( j=0,k=curr_code_size; j<codeSize; j++,k++ ) {
      GlobalMem->code[k] = executable[curr_position_on_file];
      curr_position_on_file++;
    }

    curr_code_size += codeSize;
  }

  for ( i=0,curr=GlobalMem->tasks; i<NumOfTasks; i++,curr=curr->nxt ) {
    if ( memcmp ( magicTask, &executable[curr_position_on_file], 4*sizeof(char) ) == 0 ) {
      curr_position_on_file = curr_position_on_file + 4;
    }else {
      printf( "Your file isn't an executable binary file...!!\n" );
      for ( j=0; j<NumOfTasks; j++ ) {
        if ( j < NumOfTasks-1 ) {
          curr = head->nxt;
          free( head );
          head = curr;
        }else {
          free( head );
        }
      }
      free( GlobalMem->globalMem );
      free( GlobalMem->code );
      free( GlobalMem );

      return NULL;
    }
    bodytask = executable[curr_position_on_file];
    curr_position_on_file++;

    curr->pc = startingCode[bodytask-1];
    curr->localMem = ( char * ) malloc ( sizeof ( char ) * bodyLocalSize[bodytask-1] );
    curr->localMem[bodyLocalSize[bodytask-1] - 1] = executable[curr_position_on_file];
    curr_position_on_file++;

  }

  if ( memcmp ( magicEnd, &executable[curr_position_on_file], 4*sizeof(char) ) == 0 ) {
    curr_position_on_file = curr_position_on_file + 4;
  }else {
    printf( "Your file isn't an executable binary file...!!\n" );
    for ( j=0; j<NumOfTasks; j++ ) {
      if ( j < NumOfTasks-1 ) {
        curr = head->nxt;
        free( head->localMem );
        free( head );
        head = curr;
      }else {
        free( head->localMem );
        free( head );
      }
    }
    free( GlobalMem->globalMem );
    free( GlobalMem->code );
    free( GlobalMem );

    return NULL;
  }

  free( startingCode );
  free( bodyLocalSize );

  return GlobalMem;
}

localMemory *switch_task ( localMemory *head, globalMemory *globalMem ) {
  localMemory *curr;
  time_t currTime;

  for ( curr=head->nxt; curr->nxt!=head; curr=curr->nxt ) {
    if ( curr->state == 0 || curr->state == 2 ) {
      if ( curr->state == 0 ) {
        sleep(1);
        globalMem->cur = curr->id;
        return curr;
      }else {
        currTime = time ( NULL );
        if ( currTime > curr->waket ) {
          curr->state = READY;
          globalMem->cur = curr->id;
          return curr;
        }
      }
    }
  }

  if ( head->state == 0 ) {
    return head;
  }

  return NULL;
}

void instruction_set ( localMemory *curr_task, globalMemory *GlobalMem ) {
  int regX,regY,addr,val,switchTask=0,yield=0,offset,i,idx;
  localMemory *curr;
  time_t currTime;
  char string[100];
  
  while ( 1 ) {
    if ( switchTask == SWITCH || yield == 1 ) {
      curr_task = switch_task ( curr_task, GlobalMem );
      switchTask = 0;
      yield = 0;

      if ( curr_task == NULL ) {
        break;
      }
    }
    
    switch ( GlobalMem->code [ curr_task->pc ] ) {
      case 0x01:  // LLOAD: 0x01, reg, addr == reg=localMem[addr];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->localMem[addr];
        curr_task->pc += 3;
        break;
      case 0x02:  // LLOADi: 0x02, reg, addr == reg=localMem[addr+idx];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        curr_task->reg[regX] = curr_task->localMem[addr+idx];
        curr_task->pc += 3;
        break;
      case 0x03:  // GLOAD: 0x03, reg, addr == reg=globalMem[addr];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = GlobalMem->globalMem[addr];
        curr_task->pc += 3;
        break;
      case 0x04:  // GLOADi: 0x04, reg, addr == reg=globalMem[addr+idx];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        curr_task->reg[regX] = GlobalMem->globalMem[addr+idx];
        curr_task->pc += 3;
        break;
      case 0x05:  // LSTORE: 0x05, reg, addr == localMem[addr]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        curr_task->localMem[addr] = curr_task->reg[regX];
        curr_task->pc += 3;
        break;
      case 0x06:  // LSTOREi: 0x06, reg, addr == localMem[addr+idx]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        curr_task->localMem[addr+idx] = curr_task->reg[regX];
        curr_task->pc += 3;
        break;
      case 0x07: // GSTORE: 0x07, reg, addr == globalMem[addr]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        GlobalMem->globalMem[addr] = curr_task->reg[regX];
        curr_task->pc += 3;
        break;
      case 0x08: // GSTOREi: 0x08, reg, addr == globalMem[addr+idx]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        GlobalMem->globalMem[addr+idx] = curr_task->reg[regX];
        curr_task->pc += 3;
        break;
      case 0x09:  // SET: 0x09, reg, val == reg=val;
        regX = GlobalMem->code[curr_task->pc + 1];
        val = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = val;
        curr_task->pc += 3;
        break;
      case 0x0a:  // ADD: 0x0a, reg1, reg2 == reg1=reg1+reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] += curr_task->reg[regY];
        curr_task->pc += 3;
        break;
      case 0x0b:  // SUB: 0x0b, reg1, reg2 == reg1=reg1-reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] -= curr_task->reg[regY];
        curr_task->pc += 3;
        break;
      case 0x0c:  // MUL: 0x0c, reg1, reg2 == reg1=reg1*reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->reg[regX] * curr_task->reg[regY];
        curr_task->pc += 3;
        break;
      case 0x0d:  // DIV: 0x0d, reg1, reg2 == reg1=reg1/reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->reg[regX] / curr_task->reg[regY];
        curr_task->pc += 3;
        break;
      case 0x0e:  // MOD: 0x0e, reg1, reg2 == reg1=reg1%reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->reg[regX] % curr_task->reg[regY];
        curr_task->pc += 3;
        break;
      case 0x0f:  // BRGZ: 0x0f, reg1, offset == if(reg>0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( curr_task->reg[regX] > 0 ) {
          curr_task->pc += offset*3;
        }else {
          curr_task->pc += 3;
        }
        break;
            case 0x10:  // BRGEZ: 0x10, reg1, offset == if(reg>=0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX >= 0 ) {
          curr_task->pc += offset*3;
        }else {
          curr_task->pc += 3;
        }
        break;
      case 0x11:  // BRLZ: 0x11, reg1, offset == if(reg<0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX < 0 ) {
          curr_task->pc += offset*3;
        }else {
          curr_task->pc += 3;
        }
        break;
      case 0x12:  // BRLEZ: 0x12, reg1, offset == if(reg<=0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX <= 0 ) {
          curr_task->pc += offset*3;
        }else {
          curr_task->pc += 3;
        }
        break;
      case 0x13:  // BREZ: 0x13, reg1, offset == if(reg==0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX == 0 ) {
          curr_task->pc += offset*3;
        }else {
          curr_task->pc += 3;
        }
        break;
      case 0x14:  // BRA: 0x14, reg1, offset == pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        curr_task->pc += offset*3;
        break;
      case 0x15:  // DOWN: 0x15, -, addr == globalMem[addr]--;
      //                        if(globalMem[addr]<0){state=1; sem=addr;}
        addr = GlobalMem->code[curr_task->pc + 2];
        GlobalMem->globalMem[addr]--;
        if ( GlobalMem->globalMem[addr] < 0 ) {
          curr_task->state = BLOCKED;
          curr_task->sem = addr;
          yield = 1;
        }
        curr_task->pc += 3;
        break;
      case 0x16:  // UP: 0x16, -, addr == globalMem[addr]++;
      //                        if(globalMem[addr]<=0){t=findBlocked(addr); t.state=0; t.sem=-1;}
        addr = GlobalMem->code[curr_task->pc + 2];
        GlobalMem->globalMem[addr]++;
        if ( GlobalMem->globalMem[addr] <= 0 ) {
          for ( curr=curr_task; curr->nxt!=curr_task; curr=curr->nxt) {
            if( curr->state == 1 && curr->sem == addr ) {
              break;
            }
          }
          curr->state = READY;
          curr->sem = -1;
        }
        curr_task->pc += 3;
        break;
      case 0x17:  // YIELD: 0x17, -, - == switch to another task
        yield = 1;
        curr_task->pc += 3;
        break;
      case 0x18:  // SLEEP: 0x18, reg, - == t.state=2; t.waket=curTime()+reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        curr_task->state = SLEEPING;
        currTime = time( NULL );
        curr_task->waket = currTime + regX;
        curr_task->pc += 3;
        break;
      case 0x19:  // PRINT: 0x19, -, addr == printf("%d, %s\n",id,globalMem[addr]);
        i=0;
        while ( 1 ) {
          string[i] = GlobalMem->globalMem[addr+i];
          i++;
          if ( string[i-1] == 0x00 ) {
            string[i] = '\0';
            break;
          }
        }
        printf ( "%d: %s", curr_task->id, string );
        curr_task->pc += 3;
        break;
      case 0x1a:  // EXIT: 0x1a, -, - == state=3;
        curr_task->state = STOPPED;
        yield = 1;
        break;
      default:  // DEFAULT: yield 1 == this command isn't correct.Do nothing
        printf ( "Wrong Instruction. Exiting with failure...\n" );
        break;
    }
    switchTask++;
  }
}

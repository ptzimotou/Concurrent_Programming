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
#include <pthread.h>
#include <time.h>
#include <sched.h>

#define NumOfReg 32 // pc == reg31, idx == reg0
#define SWITCH 15
#define CPUS 4

#define magicBeg "\xde\xad\xbe\xaf"
#define magicBody "\xde\xad\xc0\xde"
#define magicTask "\xde\xad\xba\xbe"
#define magicEnd "\xfe\xe1\xde\xad"

enum states { READY=0, BLOCKED, SLEEPING, STOPPED };

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

typedef struct pthread_args {
  localMemory *local;
  globalMemory *global;
  struct pthread_args *cpus_list;
  struct pthread_args *nxt;
}args;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *down_up_conflict_mtxs; // By convention addresses of mutexes will be stored
                                        // at the beginning of the global memory.

char *read_binary ( const char *file_name );

char *check_if_executable ( char *decimal_file );

globalMemory *decode_the_executable ( char *decimal_file );

localMemory *switch_task ( localMemory *head, globalMemory *globalMem );

void *instruction_set ( void *arg );

args *tasks_per_cpu ( localMemory *head, int NumOfTasks , globalMemory *Global );

int main ( int argc, char *argv[] ) {
  globalMemory *GlobalMem;
  localMemory *curr_task,*curr,*tasksPerCpu;
  args *arg;
  char *executable_file;
  int regX,regY,addr,val,switchTask=0,yield=0,offset,i,curr_cpus,numOfTasks,pthreadCheck;
  pthread_t thread[CPUS];


  executable_file = read_binary( argv[1] );
  if ( executable_file == NULL ) {
    return 1;
  }
  executable_file = check_if_executable ( executable_file );
  if ( executable_file == NULL ) {
    free ( executable_file );
    return 1;
  }

  GlobalMem = decode_the_executable ( executable_file );
  if ( GlobalMem == NULL ) {
    return 0;
  }

  numOfTasks = GlobalMem->tasks->prev->id;
  arg = tasks_per_cpu ( GlobalMem->tasks, numOfTasks , GlobalMem);
  if ( arg == NULL ) {
    printf ( "An error occured. Exiting with failure...\n" );
    return 1;
  }

  curr_cpus = CPUS;
  if ( curr_cpus > numOfTasks ) {
    curr_cpus = numOfTasks;
  }

  for ( i=0; i<curr_cpus; i++ ) {
    pthreadCheck = pthread_mutex_lock ( &mtx );
    if ( pthreadCheck != 0 ) {
      printf ( "---->%s<----\n", strerror(errno) );
      return 1;
    }

    pthreadCheck = pthread_create ( &thread[i], NULL, instruction_set, (void *)arg );
    if ( pthreadCheck != 0 ) {
      printf ( "---->%s<----\n", strerror(errno) );
      return 1;
    }
    arg = arg->nxt;
  }

  for ( i=0; i<curr_cpus; i++ ) {
    pthreadCheck = pthread_join ( thread[i], NULL );
    if ( pthreadCheck != 0 ) {
      printf ( "---->%s<----\n", strerror(errno) );
      return 1;
    }
  }

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
  int NumOfTasks,codeSize,*startingCode,curr_code_size=0,mutex_counter=0,mutex_exist;
  int *bodyLocalSize,bodytask,k,l,pthreadCheck;
  char hex[5],hex2[3],*mutexes_array;

  curr_position_on_file = 4;

  GlobalMem = ( globalMemory * ) malloc ( sizeof ( globalMemory ) );
  if ( GlobalMem == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  // GlobalSize == Size of global memory
  GlobalMem->globalMem = ( char * ) malloc ( sizeof ( char ) * (executable[curr_position_on_file]) );
  if ( GlobalMem->globalMem == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  curr_position_on_file++;

  // NumOfBodies == Number of task body procedures
  NumOfBodies = executable[curr_position_on_file];
  curr_position_on_file++;

  startingCode = ( int * ) malloc ( sizeof ( int ) * NumOfBodies );
  if ( startingCode == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  bodyLocalSize = ( int * ) malloc ( sizeof ( int ) * NumOfBodies );
  if ( bodyLocalSize == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  // TotCodeSize == Total code size of all bodies ( in bytes )
  sprintf ( hex, "%x", executable[curr_position_on_file] );
  curr_position_on_file++;
  sprintf ( hex2, "%x", executable[curr_position_on_file] );
  curr_position_on_file++;
  strcat ( hex, hex2 );
  TotCodeSize = ( int ) strtol ( hex, NULL, 16 );

  GlobalMem->code = ( char * ) malloc ( sizeof ( char ) * TotCodeSize );
  if ( GlobalMem->code == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  // NumOfTasks == Number of tasks
  NumOfTasks = executable[curr_position_on_file];

  for ( i=0,first=1; i<NumOfTasks; i++ ) {
    curr = ( localMemory * ) malloc ( sizeof ( localMemory ) );
    if ( curr == NULL ) {
      printf ( "---->%s<----\n", strerror(errno) );
      return NULL;
    }

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

    mutexes_array = ( char * ) malloc ( sizeof ( char ) );
    for ( j=0,k=curr_code_size; j<codeSize; j++,k++ ) {
      GlobalMem->code[k] = executable[curr_position_on_file];
      //find all the different "mutexes" on the code.
      if ( ( GlobalMem->code[k] == 0x15 && ( j % 3 ) == 0 ) || ( GlobalMem->code[k] == 0x16 && ( j % 3 ) == 0 ) ) {
        mutex_exist = 0;
        for ( l=0; l<mutex_counter; l++ ) {
          if ( mutexes_array[l] == executable[curr_position_on_file+2] ) {
            mutex_exist = 1;
            break;
          }
        }

        if ( mutex_exist == 0 ) {
          mutexes_array = ( char * ) realloc ( mutexes_array, sizeof ( char ) * ( mutex_counter + 1 ) );
          if ( mutexes_array == NULL ) {
            printf ( "---->%s<----\n", strerror(errno) );
            return NULL;
          }

          mutexes_array[mutex_counter+1] = executable[curr_position_on_file+2];
          mutex_counter++;
        }
      }

      curr_position_on_file++;
    }

    curr_code_size += codeSize;
  }

  down_up_conflict_mtxs = ( pthread_mutex_t * ) malloc ( sizeof ( pthread_mutex_t ) * strlen ( mutexes_array ) );
  if ( down_up_conflict_mtxs == NULL ) {
    printf ( "---->%s<----\n", strerror(errno) );
    return NULL;
  }

  for ( i=0; i<mutex_counter; i++ ) {
    pthreadCheck = pthread_mutex_init ( &down_up_conflict_mtxs[i], NULL );
    if ( pthreadCheck != 0 ) {
      printf ( "---->%s<----\n", strerror(errno) );
      return NULL;
    }
  }

  free( mutexes_array );
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
    if ( curr->localMem == NULL ) {
      printf ( "---->%s<----\n", strerror(errno) );
      return NULL;
    }

    curr->localMem[bodyLocalSize[bodytask-1] - 1] = executable[curr_position_on_file]; //
    curr_position_on_file++;
  }
  if ( memcmp ( magicEnd, &executable[curr_position_on_file], 4*sizeof(char) ) == 0 ) {
    curr_position_on_file = curr_position_on_file + 4;
  }else {
    printf( "Your file isn't an executable binarys file...!!\n" );
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

  if ( head->nxt == head ) {
    curr=head;
    if ( curr->state == 0 || curr->state == 2 ) {
      if ( curr->state == 0 ) {
        globalMem->cur = curr->id;
        return curr;
      }else {
        currTime = time ( NULL );
        if ( currTime > curr->waket ) {
          curr->state = READY;
          globalMem->cur = curr->id;
          curr->waket = -1;
          return curr;
        }
      }
    }
  }else {
    for ( curr=head->nxt; curr->nxt!=head; curr=curr->nxt ) {
      if ( curr->state == 0 || curr->state == 2 ) {
        if ( curr->state == 0 ) {
          globalMem->cur = curr->id;
          return curr;
        }else {
          currTime = time ( NULL );
          if ( currTime > curr->waket ) {
            curr->state = READY;
            globalMem->cur = curr->id;
            curr->waket = -1;
            return curr;
          }
        }
      }
    }
  }
  if ( head->state == 0 ) {
    return head;
  }
  if (head->state == STOPPED ) {
    return NULL;
  }

  return ((localMemory *)0x01);
}

void *instruction_set ( void *arg ) {
  int regX,regY,addr,val,idx,switchTask=1,yield=1,offset,i,pthreadCheck,check;
  localMemory *curr,*curr_task,*curr_task_prev;
  globalMemory *GlobalMem;
  args *curr_cpu,*cpu_list;
  time_t currTime;
  char string[100];

  curr_task = ((args *)arg)->local;
  GlobalMem = ((args *)arg)->global;
  cpu_list = ((args *)arg)->cpus_list;
  pthreadCheck = pthread_mutex_unlock ( &mtx );
  if ( pthreadCheck != 0 ) {
    printf ( "---->%s<----\n", strerror(errno) );
    pthread_exit ( NULL );
  }

  while ( 1 ) {
    if ( switchTask == SWITCH || yield == 1 ) {
      curr_task_prev = curr_task;
      curr_task = (localMemory *)0x01;
      while ( curr_task == (localMemory *)0x01 ) {
        curr_task = switch_task ( curr_task_prev, GlobalMem );
        switchTask = 0;
        yield = 0;

        if ( curr_task == NULL ) {
          pthread_exit ( NULL );
        }
      }
    }

    switch ( GlobalMem->code [ curr_task->pc ] ) {
      case 0x01:  // LLOAD: 0x01, reg, addr == reg=localMem[addr];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->localMem[addr];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x02:  // LLOADi: 0x02, reg, addr == reg=localMem[addr+idx];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        curr_task->reg[regX] = curr_task->localMem[addr+idx];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x03:  // GLOAD: 0x03, reg, addr == reg=globalMem[addr];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = GlobalMem->globalMem[addr];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x04:  // GLOADi: 0x04, reg, addr == reg=globalMem[addr+idx];
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        curr_task->reg[regX] = GlobalMem->globalMem[addr+idx];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x05:  // LSTORE: 0x05, reg, addr == localMem[addr]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        curr_task->localMem[addr] = curr_task->reg[regX];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x06:  // LSTOREi: 0x06, reg, addr == localMem[addr+idx]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        curr_task->localMem[addr+idx] = curr_task->reg[regX];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x07: // GSTORE: 0x07, reg, addr == globalMem[addr]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        GlobalMem->globalMem[addr] = curr_task->reg[regX];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x08: // GSTOREi: 0x08, reg, addr == globalMem[addr+idx]=reg;
        regX = GlobalMem->code[curr_task->pc + 1];
        addr = GlobalMem->code[curr_task->pc + 2];
        idx = curr_task->idx;
        GlobalMem->globalMem[addr+idx] = curr_task->reg[regX];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x09:  // SET: 0x09, reg, val == reg=val;
        regX = GlobalMem->code[curr_task->pc + 1];
        val = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = val;
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x0a:  // ADD: 0x0a, reg1, reg2 == reg1=reg1+reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] += curr_task->reg[regY];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x0b:  // SUB: 0x0b, reg1, reg2 == reg1=reg1-reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] -= curr_task->reg[regY];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x0c:  // MUL: 0x0c, reg1, reg2 == reg1=reg1*reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->reg[regX] * curr_task->reg[regY];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x0d:  // DIV: 0x0d, reg1, reg2 == reg1=reg1/reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->reg[regX] / curr_task->reg[regY];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x0e:  // MOD: 0x0e, reg1, reg2 == reg1=reg1%reg2;
        regX = GlobalMem->code[curr_task->pc + 1];
        regY = GlobalMem->code[curr_task->pc + 2];
        curr_task->reg[regX] = curr_task->reg[regX] % curr_task->reg[regY];
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x0f:  // BRGZ: 0x0f, reg1, offset == if(reg>0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( curr_task->reg[regX] > 0 ) {
          curr_task->pc = curr_task->pc + ( offset*3 );
        }else {
          curr_task->pc = curr_task->pc + 3;
        }
        break;
      case 0x10:  // BRGEZ: 0x10, reg1, offset == if(reg>=0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX >= 0 ) {
          curr_task->pc = curr_task->pc + ( offset*3 );
        }else {
          curr_task->pc = curr_task->pc + 3;
        }
        break;
      case 0x11:  // BRLZ: 0x11, reg1, offset == if(reg<0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX < 0 ) {
          curr_task->pc = curr_task->pc + ( offset*3 );
        }else {
          curr_task->pc = curr_task->pc + 3;
        }
        break;
      case 0x12:  // BRLEZ: 0x12, reg1, offset == if(reg<=0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX <= 0 ) {
          curr_task->pc = curr_task->pc + ( offset*3 );
        }else {
          curr_task->pc = curr_task->pc + 3;
        }
        break;
      case 0x13:  // BREZ: 0x13, reg1, offset == if(reg==0) pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        if ( regX == 0 ) {
          curr_task->pc = curr_task->pc + ( offset*3 );
        }else {
          curr_task->pc = curr_task->pc + 3;
        }
        break;
      case 0x14:  // BRA: 0x14, reg1, offset == pc=pc+offset;
        regX = GlobalMem->code[curr_task->pc + 1];
        offset = GlobalMem->code[curr_task->pc + 2];
        curr_task->pc = curr_task->pc + ( offset*3 );
        break;
      case 0x15:  // DOWN: 0x15, -, addr == globalMem[addr]--;
      //                        if(globalMem[addr]<0){state=1; sem=addr;}

        addr = GlobalMem->code[curr_task->pc + 2];
        pthreadCheck = pthread_mutex_lock ( &down_up_conflict_mtxs[addr] );
        if ( pthreadCheck != 0 ) {
          printf ( "---->%s<----\n", strerror(errno) );
          pthread_exit ( NULL );
        }

        GlobalMem->globalMem[addr]--;
        if ( GlobalMem->globalMem[addr] < 0 ) {
          curr_task->state = BLOCKED;
          curr_task->sem = addr;
          yield = 1;
        }
        pthreadCheck = pthread_mutex_unlock ( &down_up_conflict_mtxs[addr] );
        if ( pthreadCheck != 0 ) {
          printf ( "---->%s<----\n", strerror(errno) );
          pthread_exit ( NULL );
        }

        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x16:  // UP: 0x16, -, addr == globalMem[addr]++;
      //                        if(globalMem[addr]<=0){t=findBlocked(addr); t.state=0; t.sem=-1;}
        addr = GlobalMem->code[curr_task->pc + 2];
        pthreadCheck = pthread_mutex_lock ( &down_up_conflict_mtxs[addr] );
        if ( pthreadCheck != 0 ) {
          printf ( "---->%s<----\n", strerror(errno) );
          pthread_exit ( NULL );
        }
        GlobalMem->globalMem[addr]++;

        if ( GlobalMem->globalMem[addr] <= 0 ) {
          check=0;
          for ( curr_cpu=cpu_list; (curr_cpu!=NULL && check==0); curr_cpu=curr_cpu->nxt ) {
            for ( curr=curr_cpu->local; 1; curr=curr->nxt) {
              if( curr->state == 1 && curr->sem == addr ) {
                check=1;
                curr->state = READY;
                curr->sem = -1;
                yield = 1;
                break;
              }
              if ( curr->nxt==curr_cpu->local ) {
                break;
              }
            }
            
          }
        }
        pthreadCheck = pthread_mutex_unlock ( &down_up_conflict_mtxs[addr] );
        if ( pthreadCheck != 0 ) {
          printf ( "---->%s<----\n", strerror(errno) );
          pthread_exit ( NULL );
        }

        curr_task->pc = curr_task->pc + 3;
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
        printf("%d currTime: %ld, sleep: %d\n", curr_task->id, currTime, curr_task->waket);
        curr_task->pc = curr_task->pc + 3;
        yield = 1;
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
        printf ( "%d: %s", curr_task->id, &GlobalMem->globalMem[addr] );
        curr_task->pc = curr_task->pc + 3;
        break;
      case 0x1a:  // EXIT: 0x1a, -, - == state=3;
        curr_task->state = STOPPED;
        yield = 1;
        break;
      default:  // DEFAULT: yield 1 == this command isn't correct.Do nothing
        printf ( "Wrong Instruction. Exiting with failure...\n" );
        pthread_exit ( NULL );
        break;
    }
    switchTask++;
  }

  pthread_exit ( NULL );
}

// split list of tasks
args *tasks_per_cpu ( localMemory *head, int NumOfTasks , globalMemory *Global) {
    localMemory *headTable[CPUS],*curr;
    int tasksPerCpu[CPUS],i,j,curr_cpus;
    args *head_arg,*curr_arg;

    curr_cpus = CPUS;
    if ( curr_cpus > NumOfTasks ) {
      curr_cpus = NumOfTasks;
    }

    // Calculate the number of tasks/cpu
    for ( i=0; i<CPUS; i++ ) {
      tasksPerCpu[i] = NumOfTasks / CPUS;
      if ( ( NumOfTasks % CPUS ) != 0 ) {
        tasksPerCpu[i]++;
        NumOfTasks--;
      }
    }

    head_arg = NULL;
    for ( i=0; i<curr_cpus; i++ ) {
      curr_arg = ( args * ) malloc ( sizeof ( args ) );
      if ( curr_arg == NULL ) {
        printf ( "---->%s<----\n", strerror(errno) );
        return NULL;
      }

      for( j=0,curr=head; j<(tasksPerCpu[i]-1); j++,curr=curr->nxt ) {

      }

      head = curr->nxt;

      curr_arg->local = head;
      curr_arg->global = Global;

      curr_arg->nxt = head_arg;
      head_arg=curr_arg;
    }

    for( j=0,curr_arg=head_arg; j<curr_cpus; j++,curr_arg=curr_arg->nxt ) {
      curr_arg->cpus_list = head_arg;
    }

    return head_arg;
}

/** 
  Concurrent_Programming    

  @author Tzimotoudis Panagiotis
  @author Tziokas George
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LINELEN 150
#define LABEL_LEN 35

typedef struct label_list {
  char *label;
  int beg_end;  // intialize to 0
  struct label_list *nxt;
}labelL;

char *getLine ( char *filename ) {
  char *line;
  static FILE *input;
  static int firstTime = 1;

  if ( firstTime == 1 ) {
    input = fopen ( filename, "r" );
    if ( input == NULL ) {
      printf ( "ERROR: Could Not Open File\n");
      exit ( 1 );
    }
    firstTime = 0;
  }

  line = ( char * ) malloc ( sizeof ( char ) * LINELEN );
  if ( line == NULL ) {
    printf ( "ERROR: Memory Allocation Error\n" );
    exit ( 1 );
  }

  if ( fscanf ( input, "%150[^\n]\n", line ) == EOF ) {
    fclose ( input );
    firstTime = 1;
    return NULL;
  }

  return ( line );
}

char *findLabel ( char *line_with_label ) {
  char *label;
  int i,j;

  label = ( char *) malloc (sizeof(char) * LABEL_LEN );
  for ( i =0 ; i < LINELEN; i++ ) {
    if ( line_with_label[i] == '(' ) {
      i++;
      for ( j = 0; j < LABEL_LEN; j++ ) {
        if ( line_with_label[i] != ' ' && line_with_label[i] != ')' ) {
          label[j] = line_with_label[i];
          i++;
        }else if ( line_with_label[i] == ')' ) {
          return ( label );
        }else {
          i++;
          j--;
        }
      }
    }
  }

  return NULL;
}

int check_for_beg_end ( labelL *root ) {
  labelL *curr;

  for ( curr = root; curr != NULL; curr = curr->nxt ) {
    if ( curr->beg_end == 1 ) {
      printf("ERROR: Syntax Error.There is a synch_beg without a synch_end in label %s\n", curr->label);
      return 1;
    }else {
      return 0;
    }
  }

  return 0;
}

int checkBeg ( labelL *root, char *label ) {
  labelL *curr;

  for ( curr = root; curr != NULL; curr = curr->nxt ) {
    if ( strcmp( curr->label, label ) == 0 ){
      if ( curr->beg_end == 0 ) {
        curr->beg_end = 1;
        return 0;
      }else {
        printf("ERROR: You can not call synch_beg twice inside a monitor with label %s\n", curr->label);
        return 1;
      }
    }
  }

  printf("ERROR: There is no declaration of label: %s\n", label);
  return 1;
}

int checkEnd ( labelL *root, char *label ) {
  labelL *curr;

  for ( curr = root; curr != NULL; curr = curr->nxt ) {
    if ( strcmp( curr->label, label ) == 0 ) {
      if ( curr->beg_end == 1 ) {
        curr->beg_end = 0;
        return 0;
      }else {
        printf("ERROR: You call synch_beg without call synch_beg before in label %s\n", curr->label);
        return 1;
      }
    }
  }

  printf("ERROR: There is no declaration of label: %s\n", label);
  return 1;
}

int checkWait ( labelL *root, char *label ) {
  labelL *curr;

  for ( curr = root; curr != NULL; curr = curr->nxt ) {
    if ( strcmp( curr->label, label ) == 0 ) {
      if ( curr->beg_end == 1 ) {
        return 0;
      }else {
        printf("ERROR: You can not call synch_wait outside from monitor\n");
        return 1;
      }
    }
  }

  printf("ERROR: There is no declaration of label: %s\n", label);
  return 1;
}

int checkNotify ( labelL *root, char *label ) {
  labelL *curr;
  int label_exists=0,labelOn=0;

  for ( curr = root; curr != NULL; curr = curr->nxt ) {
    if ( curr->beg_end == 1 ) {
      labelOn = 1;
    }
    if ( strcmp( curr->label, label ) == 0 ) {
      label_exists = 1;
    }
  }

  if ( labelOn == 1 && label_exists == 1 ) {
    return 0;
  }else if ( labelOn == 0 && label_exists == 1) {
    printf("ERROR: Syntax Error.You are using synch_notify || synch_notifyall with label \"%s\" inappropriate.\n", curr->label);
    return 1;
  }

  if ( label_exists == 0 ) {
    printf("ERROR: There is no declaration of label: %s\n", label);
    return 1;
  }
  return 1;
}

int main ( int argc, char *argv[] ) {
  char *line,*curr_label;
  int first_label=1,beg_end_check,check_beg,check_end,check_wait,check_notify;  // initialize to 1
  labelL *root=NULL,*curr;
  FILE *output;

  output = fopen ( argv[2], "a" );
  if ( output == NULL ) {
    printf ( "ERROR: Could Not Open File\n");
    exit ( 1 );
  }

  do {
    line = getLine ( argv[1] );
    if ( line != NULL ) {

      if ( strncmp ( line, "synch_beg", strlen("synch_beg") ) == 0 ) {
        curr_label = findLabel ( line );
        check_beg = checkBeg ( root , curr_label );
        if ( check_beg == 1 ) {
          fclose ( output );
          unlink ( argv[2] );
          return 1;
        }else {
          fputs("pthread_mutex_lock ( &mtx_", output);
          fputs( curr_label, output );
          fputs( " );\n", output );
        }
      }else if ( strncmp ( line, "synch_end", strlen("synch_end") ) == 0 ) {
        curr_label = findLabel ( line );
        check_end = checkEnd ( root , curr_label );
        if ( check_end == 1 ) {
          fclose ( output );
          unlink ( argv[2] );
          return 1;
        }else {
          fputs("pthread_mutex_unlock ( &mtx_", output);
          fputs( curr_label, output );
          fputs( " );\n", output );
        }
      }else if ( strncmp ( line, "synch_wait", strlen("synch_wait") ) == 0 ) {
        curr_label = findLabel ( line );
        check_wait = checkWait ( root, curr_label );
        if ( check_wait == 1 ) {
          fclose ( output );
          unlink ( argv[2] );
          return 1;
        }else {
          fputs( "waitL *curr;\n", output );
          fputs( "curr = ( waitL *) malloc ( sizeof ( waitL ) );\n", output );
          fputs( "curr->nxt = NULL;\n", output );
          fputs( "pthread_mutex_init ( &curr->wait_mtx, NULL );\n", output );
          fputs( "pthread_mutex_lock ( &curr->wait_mtx );\n", output);
          fputs( "if ( end_", output );
          fputs( curr_label, output );
          fputs( " == NULL ) {\n", output );
          fputs( "\tend_", output );
          fputs( curr_label, output );
          fputs( " = curr;\n", output );
          fputs( "\troot_", output );
          fputs( curr_label, output );
          fputs( " = end_", output );
          fputs( curr_label, output );
          fputs( ";\n", output );
          fputs( "}else {\n", output );
          fputs( "\tend_", output );
          fputs( curr_label, output );
          fputs( "->nxt = curr;\n", output );
          fputs( "\tend_", output );
          fputs( curr_label, output );
          fputs( " = curr;\n}\n", output );
          fputs( "pthread_mutex_unlock ( &mtx_", output );
          fputs( curr_label, output );
          fputs( " );\n", output );
          fputs( "pthread_mutex_lock ( &curr->wait_mtx );\n", output );
          fputs( "pthread_mutex_destroy ( &curr->wait_mtx );\n", output );
          fputs( "free ( curr );\n", output );
          fputs( "pthread_mutex_lock ( &mtx_", output );
          fputs( curr_label, output );
          fputs( " );\n", output );
        }
      }else if ( strncmp ( line, "synch_notifyall", strlen("synch_notifyall") ) == 0 ) {
        curr_label = findLabel ( line );
        check_notify = checkNotify ( root , curr_label );
        if ( check_notify == 1 ) {
          fclose ( output );
          unlink ( argv[2] );
          return 1;
        }else {
          fputs( "while ( root_", output );
          fputs( curr_label, output );
          fputs( " != NULL ) {\n", output );
          fputs( "\twaitL *temp;\n", output );
          fputs( "\ttemp = ( waitL *) malloc ( sizeof ( waitL ) );\n", output );
          fputs( "\ttemp = root_", output );
          fputs( curr_label, output );
          fputs( ";\n", output );
          fputs( "\troot_", output );
          fputs( curr_label, output );
          fputs( " = root_", output );
          fputs( curr_label, output );
          fputs( "->nxt;\n", output );
          fputs( "\tif ( root_", output );
          fputs( curr_label, output );
          fputs( " == NULL ) {\n", output );
          fputs( "\t\tend_", output );
          fputs( curr_label, output );
          fputs( " = root_", output );
          fputs( curr_label, output );
          fputs( ";\n", output );
          fputs( "\t}\n", output );
          fputs( "\tpthread_mutex_unlock ( &temp->wait_mtx );\n", output );
          fputs( "}\n", output );
        }
      }else if ( strncmp ( line, "synch_notify", strlen("synch_notify") ) == 0 ) {
        curr_label = findLabel ( line );
        check_notify = checkNotify ( root , curr_label );
        if ( check_notify == 1 ) {
          fclose ( output );
          unlink ( argv[2] );
          return 1;
        }else {
          fputs( "if ( root_", output );
          fputs( curr_label, output );
          fputs( " != NULL ) {\n", output );
          fputs( "\twaitL *temp;\n", output );
          fputs( "\ttemp = ( waitL *) malloc ( sizeof ( waitL ) );\n", output );
          fputs( "\ttemp = root_", output );
          fputs( curr_label, output );
          fputs( ";\n", output );
          fputs( "\troot_", output );
          fputs( curr_label, output );
          fputs( " = root_", output );
          fputs( curr_label, output );
          fputs( "->nxt;\n", output );
          fputs( "\tif ( root_", output );
          fputs( curr_label, output );
          fputs( " == NULL ) {\n", output );
          fputs( "\t\tend_", output );
          fputs( curr_label, output );
          fputs( " = root_", output );
          fputs( curr_label, output );
          fputs( ";\n", output );
          fputs( "\t}\n", output );
          fputs( "\tpthread_mutex_unlock ( &temp->wait_mtx );\n", output );
          fputs( "}\n", output );
        }
      }else if ( strncmp ( line, "synch", strlen("synch") ) == 0 ) {
        if ( first_label == 1 ) {
          fputs( "typedef struct list {\n", output);
          fputs( "pthread_mutex_t wait_mtx;\n", output );
          fputs( "struct list *nxt;\n", output );
          fputs( "}waitL;\n", output );
          first_label = 0;
        }
        curr = ( labelL *) malloc ( sizeof ( labelL ) );
        curr->nxt = root;
        curr->beg_end = 0;
        curr_label = findLabel ( line );
        curr->label = curr_label;
        fputs( "waitL *root_", output );
        fputs( curr_label, output );
        fputs( ";\n", output );
        fputs( "waitL *end_", output );
        fputs( curr_label, output );
        fputs( ";\n", output );
        fputs( "pthread_mutex_t mtx_", output );
        fputs( curr_label, output );
        fputs( " = PTHREAD_MUTEX_INITIALIZER;\n", output );
        root = curr;
      }else if ( strncmp ( line, "pthread_exit", strlen("pthread_exit") ) == 0 ) {
        beg_end_check = check_for_beg_end ( root );
        if ( beg_end_check == 1 ) {
          fclose ( output );
          unlink ( argv[2] );
          return 1;
        }else {
          fputs ( line, output );
          fputs ( "\n", output );
        }
      }else {
        fputs( line, output );
        fputs( "\n", output );
      }
    }

  }while ( line != NULL );
  return 0;
}

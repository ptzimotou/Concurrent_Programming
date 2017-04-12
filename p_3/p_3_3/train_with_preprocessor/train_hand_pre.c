/** 
  Concurrent_Programming    

  @author Tzimotoudis Panagiotis
  @author Tziokas George
*/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#define MaxPassengers 15

int on_train; // current number of passengers on train
int start_trip; // flag for train to start
pthread_mutex_t pass_mtx;

typedef struct list {
  pthread_mutex_t wait_mtx;
  struct list *nxt;
}waitL;

waitL *root_train=NULL;
waitL *end_train=NULL;
pthread_mutex_t mtx_train = PTHREAD_MUTEX_INITIALIZER;

waitL *root_passengers=NULL;
waitL *end_passengers=NULL;
pthread_mutex_t mtx_passengers = PTHREAD_MUTEX_INITIALIZER;

void *Train ( void *arg ) {

  while ( 1 ) {
    pthread_mutex_lock ( &mtx_train );

    while ( start_trip == 0 ) {
      waitL *curr;
      curr = (waitL *)malloc(sizeof(waitL));
      curr->nxt = NULL;
      pthread_mutex_init ( &curr->wait_mtx, NULL );
      pthread_mutex_lock ( &curr->wait_mtx );
      if ( end_train == NULL ) {
        end_train = curr;
        root_train = end_train;
      }else {
        end_train->nxt = curr;
        end_train = curr;
      }
      pthread_mutex_unlock ( &mtx_train );
      pthread_mutex_lock ( &curr->wait_mtx );
      pthread_mutex_destroy ( &curr->wait_mtx );
      free(curr);
      pthread_mutex_lock ( &mtx_train );

    }
    start_trip = 0;
    printf("Starting...!!!\n");

    // On Trip
    printf("On Trip\n");
    sleep(6);
    printf("The trip is finished...\n");

    if ( root_passengers != NULL ) {
      waitL *temp;
      temp = (waitL *)malloc(sizeof(waitL));
      temp = root_passengers;
      root_passengers = root_passengers->nxt;
      if ( root_passengers == NULL ) {
        end_passengers = root_passengers;
      }
      pthread_mutex_unlock( &temp->wait_mtx );
      //free ( temp );
    }

    pthread_mutex_unlock ( &mtx_train );
  }
  pthread_exit ( NULL );
}

void *Passengers ( void *arg ) {
  pthread_mutex_lock ( &pass_mtx ); // lock here if the train is full

  pthread_mutex_lock ( &mtx_passengers );

  if ( on_train < MaxPassengers - 1 ) { // if take the pass but you are not the last then go to wake
    on_train++;
    pthread_mutex_unlock ( &pass_mtx );

    waitL *curr;
    curr = (waitL *)malloc(sizeof(waitL));
    curr->nxt = NULL;
    pthread_mutex_init ( &curr->wait_mtx,NULL );
    pthread_mutex_lock ( &curr->wait_mtx );
    if ( end_passengers == NULL ) {
      end_passengers = curr;
      root_passengers = end_passengers;
    }else {
      end_passengers->nxt = curr;
      end_passengers = curr;
    }
    pthread_mutex_unlock ( &mtx_passengers );
    pthread_mutex_lock ( &curr->wait_mtx );
    pthread_mutex_destroy ( &curr->wait_mtx );
    free(curr);
    printf("malakas\n");
    pthread_mutex_lock ( &mtx_passengers );

  }else if ( on_train == MaxPassengers - 1 ) {  // if you are the last wake up the train and go to wake
    on_train++;
    start_trip = 1;
    printf("On train: %d\n", on_train);


    if ( root_train != NULL ) {
      waitL *temp;
      temp = (waitL *)malloc(sizeof(waitL));
      temp = root_train;
      root_train = root_train->nxt;
      if ( root_train == NULL ) {
        end_train = root_train;
      }
      pthread_mutex_unlock( &temp->wait_mtx );
      //free ( temp );
    }

    waitL *curr;
    curr = (waitL *)malloc(sizeof(waitL));
    curr->nxt = NULL;
    pthread_mutex_init ( &curr->wait_mtx,NULL );
    pthread_mutex_lock ( &curr->wait_mtx );
    if ( end_passengers == NULL ) {
      end_passengers = curr;
      root_passengers = end_passengers;
    }else {
      end_passengers->nxt = curr;
      end_passengers = curr;
    }
    pthread_mutex_unlock ( &mtx_passengers );
    pthread_mutex_lock ( &curr->wait_mtx );
    pthread_mutex_destroy ( &curr->wait_mtx );
    free(curr);
    pthread_mutex_lock ( &mtx_passengers );
  }
  printf("%p\n", root_passengers);
  printf("Yeahhhhh..It's awesomeeee.....\n");
  on_train--;
  if ( on_train > 0 ) { // if there are passengers on train wake up the next passenger

    if ( root_passengers != NULL ) {
      waitL *temp;
      temp = (waitL *)malloc(sizeof(waitL));
      temp = root_passengers;
      root_passengers = root_passengers->nxt;
      if ( root_passengers == NULL ) {
        end_passengers = root_passengers;
      }
      printf("Yeah %d, %p, %p\n", on_train,temp, root_passengers);

      pthread_mutex_unlock( &temp->wait_mtx );
      //free ( temp );
    }
    //pthread_mutex_unlock ( &mtx_passengers );
  }else { // if you are the final passenger wake up the next passenger who is locked on pass_mtx
    pthread_mutex_unlock ( &pass_mtx );
    //pthread_mutex_unlock ( &mtx_passengers );
  }
  pthread_mutex_unlock ( &mtx_passengers );

  pthread_exit ( NULL );
}

// initialize the variables and "create" the train
int init () {

  pthread_t thread;

  on_train = 0;
  start_trip = 0;

  if ( pthread_mutex_init ( &pass_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }

  pthread_create ( &thread, NULL, Train, NULL );

  return (0);
}

int main ( int argc, char *argv[] ) {
  int check_init,NumOfPassengers;
  pthread_t thread;

  check_init = init();
  if ( check_init == 1 ) {
    printf("Error in init.\n");
    return 1;
  }

  while ( 1 ) {
    printf("Enter the number of the passengers\n");
    scanf(" %d", &NumOfPassengers );

    while ( NumOfPassengers > 0 ) {
      pthread_create ( &thread, NULL, Passengers, NULL );
      NumOfPassengers--;
    }
  }
  return 0;
}

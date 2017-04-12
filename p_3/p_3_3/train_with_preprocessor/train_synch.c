/** 
  Concurrent_Programming    

  @author Tzimotoudis Panagiotis
  @author Tziokas George
*/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#define MaxPassengers 100

int on_train; // current number of passengers on train
int start_trip; // flag for train to start
int threads_exist;
int main_flag;
pthread_mutex_t threads_exist_mtx;
pthread_mutex_t mtx;
pthread_mutex_t pass_mtx;

synch ( Train );
synch ( Passengers );

void *Train ( void *arg ) {

  while ( 1 ) {
    synch_beg ( Train );
    while ( start_trip == 0 ) {
      synch_wait ( Train );
    }
    start_trip = 0;
    printf("Starting...!!!\n");


    // On Trip
    printf("On Trip\n");
    sleep(1);
    printf("The trip is finished...\n");

    synch_notify ( Passengers );
    synch_end ( Train );
  }
  pthread_exit ( NULL );
}

void *Passengers ( void *arg ) {
  pthread_mutex_lock ( &mtx );
  threads_exist++;
  if ( threads_exist == 1 && main_flag == 1 ) {
    main_flag = 0;
  }
  pthread_mutex_unlock ( &mtx );

  pthread_mutex_lock ( &pass_mtx ); // lock here if the train is full

  synch_beg ( Passengers );
  if ( on_train < MaxPassengers - 1 ) { // if take the pass but you are not the last then go to wake
    on_train++;
    pthread_mutex_unlock ( &pass_mtx );

    synch_wait ( Passengers );
  }else if ( on_train == MaxPassengers - 1 ) {  // if you are the last wake up the train and go to wake
    on_train++;
    start_trip = 1;
    synch_notify ( Train );
    synch_wait ( Passengers );
  }

  printf("Yeahhhhh..It's awesomeeee.....\n");
  on_train--;
  if ( on_train > 0 ) { // if there are passengers on train wake up the next passenger
    synch_notify ( Passengers );
  }else { // if you are the final passenger wake up the next passenger who is locked on pass_mtx
    pthread_mutex_unlock ( &pass_mtx );
  }

  pthread_mutex_lock ( &mtx );
  threads_exist--;
  if ( threads_exist == 0 && main_flag == 1 ) {
    pthread_mutex_unlock ( &threads_exist_mtx );
  }else if ( threads_exist == 0 && main_flag == 0 ) {
    main_flag = 1;
  }
  pthread_mutex_unlock ( &mtx );

  synch_end ( Passengers );

  pthread_exit ( NULL );
}

// initialize the variables and "create" the train
int init () {
  pthread_t thread;

  on_train = 0;
  start_trip = 0;
  threads_exist = 0;
  main_flag = 0;

  if ( pthread_mutex_init ( &mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }

  if ( pthread_mutex_init ( &threads_exist_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }
  pthread_mutex_lock ( &threads_exist_mtx );

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
    if ( NumOfPassengers == 0 && main_flag == 0 ) {
      main_flag = 1;
      pthread_mutex_lock ( &threads_exist_mtx );
      break;
    }else if ( NumOfPassengers == 0 && main_flag == 1 ) {
      break;
    }
    while ( NumOfPassengers > 0 ) {
      pthread_create ( &thread, NULL, Passengers, NULL );
      NumOfPassengers--;
    }
  }

  pthread_mutex_destroy ( &mtx );
  pthread_mutex_destroy ( &threads_exist_mtx );
  pthread_mutex_destroy ( &pass_mtx );

  return 0;
}

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

int counter;
int count;
pthread_mutex_t train_mtx;
pthread_mutex_t enter_passenger_mtx;
pthread_mutex_t *exit_passenger_mtx;
pthread_mutex_t counter_mtx;
pthread_mutex_t end_mtx;

void StartTtrip () {
	pthread_mutex_lock ( &train_mtx ) ;
	printf("Starting...!!!\n");
}

void EndTrip () {
  
  counter = 0;
  while ( count > 0 ) {
    pthread_mutex_unlock ( &exit_passenger_mtx[count] );
    count--;
  }
  if ( MaxPassengers % 2 == 0 ) {
    counter++;
  }
  pthread_mutex_lock ( &end_mtx );
  pthread_mutex_unlock ( &enter_passenger_mtx );
   
}

void *Train ( void *arg ) {

	while ( 1 ) {
		StartTtrip ();
		// On Trip
		printf("On Trip\n");
		//sleep(6);
		
		EndTrip ();
		printf("The trip is finished...\n");
		printf("Preaparing for the next trip....Let's fun..!!\n");
	}
	return 0;
}

void *Passengers ( void *arg ) {
  
  int i;
  int control_num = 2;
  
  pthread_mutex_lock ( &enter_passenger_mtx );
  if ( count < MaxPassengers - 1 ) {
    count++;
    i=count;
    pthread_mutex_unlock ( &enter_passenger_mtx );
  }else {
    count++;
    i = count;
    pthread_mutex_unlock ( &train_mtx );
  }
  
  pthread_mutex_lock  ( &exit_passenger_mtx[i] );
  printf("Yeahhhhh..It's awesomeeee.....\n");
  pthread_mutex_lock ( &counter_mtx );
  counter++;
  control_num = MaxPassengers % 2;
  if ( counter == MaxPassengers && control_num == 1) {
    pthread_mutex_unlock ( &end_mtx );
  }else if ( control_num == 0 ) {
      if (  counter == MaxPassengers + 1 ) {
        pthread_mutex_unlock ( &end_mtx );
      }
  }
  pthread_mutex_unlock ( &counter_mtx );
  return(0);
}

int init () {
  
  int i;
  pthread_t thread;
  
  count = 0;
  
  if ( pthread_mutex_init ( &train_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }else {
    pthread_mutex_lock( &train_mtx );
  }
  
  if ( pthread_mutex_init ( &end_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }else {
    pthread_mutex_lock( &end_mtx );
  }
    
  if ( pthread_mutex_init ( &enter_passenger_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }
  
    
  if ( pthread_mutex_init ( &counter_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }
  
  exit_passenger_mtx = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*(MaxPassengers+1));
  
  for(i=0; i<MaxPassengers+1; i++){
    if ( pthread_mutex_init ( &exit_passenger_mtx[i], NULL ) != 0 ) {
      printf("\n create_mutex init failed\n");
      return 1;
    }else {
      pthread_mutex_lock( &exit_passenger_mtx[i]);
    }
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

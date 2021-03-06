/** 
  Concurrent_Programming    

  @author Tzimotoudis Panagiotis
  @author Tziokas George
*/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define MaxCarsOnBridge 5
#define Control 15

int waitCars[2];
int direction;
int cars_on_bridge;
int change_dir_counter;
int threads_exist;
int main_flag;
pthread_mutex_t mtx;
pthread_mutex_t flag_mtx;
pthread_mutex_t threads_exist_mtx;
pthread_cond_t direction_cond[2];


void enter_bridge ( int dir ) {
  cars_on_bridge++;
  direction = dir;
  waitCars[dir]--;
  change_dir_counter++;
  if ( waitCars[dir] > 0 && change_dir_counter < Control && cars_on_bridge < MaxCarsOnBridge ) {
    pthread_cond_signal ( &direction_cond[dir] );
  }

}

void exit_bridge ( int dir ) {
  int ndir;

  cars_on_bridge--;
  if ( waitCars[dir] > 0 && change_dir_counter < Control ) {
    pthread_cond_signal ( &direction_cond[dir] );
  }else if ( cars_on_bridge == 0 ) {
    change_dir_counter = 0;
    ndir = dir^1;
    if ( waitCars[ndir] > 0 ) {
      pthread_cond_signal ( &direction_cond[ndir] );
    }else if ( waitCars[dir] > 0 ) {
      pthread_cond_signal ( &direction_cond[dir] );
    }
  }
}

void *car ( void *arg ) {
  int dir;

  pthread_mutex_lock ( &flag_mtx );
  threads_exist++;
  if ( threads_exist == 1 && main_flag == 1 ) {
    main_flag = 0;
  }
  pthread_mutex_unlock ( &flag_mtx );

  dir = *(int *)arg;
  pthread_mutex_lock ( &mtx );
  waitCars[dir]++;

  while ( cars_on_bridge > 0 ) {
    if (  direction == dir && change_dir_counter < Control && cars_on_bridge < MaxCarsOnBridge  ) {
      break;
    }else {
      pthread_cond_wait ( &direction_cond[dir], &mtx );
    }
  }

  enter_bridge ( dir );
  pthread_mutex_unlock ( &mtx );
  //on bridge
  sleep(1);
  printf("On bridge with dir: %d,cars_on_bridge: %d\n", dir, cars_on_bridge);

  pthread_mutex_lock ( &mtx );
  exit_bridge ( dir );
  pthread_mutex_unlock ( &mtx );

  pthread_mutex_lock ( &flag_mtx );
  threads_exist--;
  if ( threads_exist == 0 && main_flag == 1 ) {
    pthread_mutex_unlock ( &threads_exist_mtx );
  }else if ( threads_exist == 0 && main_flag == 0 ) {
    main_flag = 1;
  }
  pthread_mutex_unlock ( &flag_mtx );

  return 0;
}

int init () {
  int i;

  threads_exist = 0;
  main_flag = 0;

  if ( pthread_mutex_init ( &flag_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }

  if ( pthread_mutex_init ( &threads_exist_mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }
  pthread_mutex_lock ( &threads_exist_mtx );

  for ( i = 0; i < 2; i++ ) {
    waitCars[i] = 0;
  }
  cars_on_bridge = 0;
  change_dir_counter = 0;

  for ( i = 0; i < 2; i++ ) {
    if ( pthread_cond_init ( &direction_cond[i], NULL ) != 0 ) {
      printf("\n create_mutex init failed\n");
      return 1;
    }
  }

  if ( pthread_mutex_init ( &mtx, NULL ) != 0 ) {
    printf("\n create_mutex init failed\n");
    return 1;
  }

  return 0;
}

int main ( int argc, char *argv[] ) {
  pthread_t thread;
  int check_init,left_dir,right_dir;
  char ch[2];

  left_dir = 1;
  right_dir = 0;

  check_init = init();
  if ( check_init == 1 ) {
    printf("Init Stop With Error.\n");
    return 1;
  }

  ch[1] = '\0';
  do {
    ch[0] = getchar();
    if ( ch[0] == '1' ){
      pthread_create ( &thread, NULL, car, (void *)&left_dir );
    }else if ( ch[0] == '0' ) {
      pthread_create ( &thread, NULL, car, (void *)&right_dir );
    }else if ( ch[0] == '2' && main_flag == 0 ) {
      main_flag = 1;
      pthread_mutex_lock ( &threads_exist_mtx );
      break;
    }else if ( ch[0] == '2' && main_flag == 1 ) {
      break;
    }
  }while ( 1 );

  pthread_mutex_destroy ( &mtx );
  pthread_mutex_destroy ( &flag_mtx );
  pthread_mutex_destroy ( &threads_exist_mtx );
  pthread_cond_destroy ( &direction_cond[0] );
  pthread_cond_destroy ( &direction_cond[1] );
  
  return 0;
}

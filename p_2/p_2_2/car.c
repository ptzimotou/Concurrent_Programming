/** 
	Concurrent_Programming		

	@author Tzimotoudis Panagiotis
	@author Tziokas George
*/

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define MaxCarsOnBridge 100
#define Control 50

int waitCars[2];
int NumOfCars;
int counter;
pthread_mutex_t waitCars_mtx;
pthread_mutex_t enter_bridge_mtx[2];

void enter_bridge ( int dir ) {
	NumOfCars++;
	waitCars[dir]--;
	counter++;
	if ( waitCars[dir] > 0 && counter < Control && NumOfCars < MaxCarsOnBridge ) {
		pthread_mutex_unlock ( &enter_bridge_mtx[dir] );
	}
}

void exit_bridge ( int dir ) {
	int ndir;
	
	NumOfCars--;
	if ( waitCars[dir] > 0 && counter < Control ) {
		pthread_mutex_unlock ( &enter_bridge_mtx[dir] );
	}else if ( NumOfCars == 0 ) {
		counter = 0;
		ndir = dir^1;
		if ( waitCars[ndir] > 0 ) {
			pthread_mutex_unlock( &enter_bridge_mtx[ndir] );
		}else if ( waitCars[dir] > 0 ) {
			pthread_mutex_unlock ( &enter_bridge_mtx[dir] );
		}
	}
}

void *car ( void *arg ) {
	int dir;
	
	dir = *(int *)arg;
	pthread_mutex_lock ( &waitCars_mtx );
	waitCars[dir]++;
	if ( NumOfCars > 0 ) {
		pthread_mutex_unlock ( &waitCars_mtx );
		pthread_mutex_lock ( &enter_bridge_mtx[dir] );
	}
	
	enter_bridge ( dir );
	pthread_mutex_unlock ( &waitCars_mtx );
	//on bridge
	sleep(1);
	printf("On bridge with dir: %d,NumOfCars: %d\n", dir, NumOfCars);
	pthread_mutex_lock ( &waitCars_mtx );
	exit_bridge ( dir );
	pthread_mutex_unlock ( &waitCars_mtx );
	
	return 0;
}

int init () {
	int i;
	
	for ( i = 0; i < 2; i++ ) {
		waitCars[i] = 0;
	}
	NumOfCars = 0;
	counter = 0;
	
	for ( i = 0; i < 2; i++ ) {
		if ( pthread_mutex_init ( &enter_bridge_mtx[i], NULL ) != 0 ) {
			printf("\n create_mutex init failed\n");
			return 1;
		}else {
			pthread_mutex_lock ( &enter_bridge_mtx[i] );
		}
	}
	
	if ( pthread_mutex_init ( &waitCars_mtx, NULL ) != 0 ) {
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
		}
	}while ( 1 );

	return 0;
}
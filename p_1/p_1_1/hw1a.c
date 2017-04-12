/** 
	Concurrent_Programming		

	@author Tzimotoudis Panagiotis
	@author Tziokas George
*/

/*  This programm take as a parameter the name and the path of a text-file and formats it.
	Firstly it delete all the whitespace and '/n' characters from the text, secodly transform 
	all the lowercase characters to uppercase, and finally put every 30 characters a '/n' 
	character and print it to user or into a new file-text.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

#define N 10

typedef struct share_buf {
	int in,out;		// "pointers" to control input and output from the "buffer"
	char buffer[N];	//	Global "buffer"
	int flag;		//	Flag to "tell" to the next thread that the previous finished his job and put all the charactes to the buffer
}buf;

buf *buf_p1, *buf_p2;
int termination = 1;	// flag to "tell" main that all threads do their jobs

// initialize the "Buffer"
void buf_init ( ) {
	buf_p1 = ( buf *)malloc ( sizeof ( buf ) );
	buf_p2 = ( buf *)malloc ( sizeof ( buf ) );

	/* initialize first "buffer" with code_name buf_p1 */
	buf_p1->in = 0;
	buf_p1->out = 0;
	buf_p1->flag = 0;

	/* initialize the second "buffer" with code_name buf_p2 */
	buf_p2->in = 0;
	buf_p2->out = 0;
	buf_p2->flag = 0;

}

// Function put. This function put one character to the first available position of "buffer" array
int buf_put ( char ch , buf *buf_p ) {

	// Put the characters only if there is a "free" position on the array 
	if ( ( ( buf_p->in + 1 ) % N ) != buf_p->out ) {
		buf_p->buffer[buf_p->in] = ch;
		buf_p->in = ( buf_p->in + 1 ) % N;
		return 0;
	}else{
		return 1;
	}
}

// Function get. This function get one character from the first available position of "buffer" array
int buf_get ( char *ch , buf *buf_p ) {

	// Get a character only if there is a character in the next available position that is not readed again before
	if ( buf_p->in != buf_p->out ) {
		*ch = buf_p->buffer[buf_p->out];
		buf_p->out = ( buf_p->out + 1 ) % N;
		return 0;
	}else if ( buf_p->in == buf_p->out && buf_p->flag == 1 ) {
		return 2 ;	// Return 2 if read all the available characters and no character is going to put in the array in the future
	}else {
		return 1;
	}
}

// Set flag 1
void buf_setEOI ( buf *buf_p ) {
	buf_p->flag = 1;
}

// Function  that removes the whitespaces and '\n' characters
void *remove_characters ( void *fd_arg ) {
	int fd,i=1,k,put_check=1;
	char tmp[2];
	ssize_t check;

	fd = *(int *)fd_arg;
	tmp[1] = ' ';	// initiallize a 2-character array to help us remove the whitespaces

	while ( 1 ) {
		i++;
		k = i % 2;

		check = read ( fd, &tmp[k], sizeof( char ) );
		if ( check > 0 ) {
			if ( tmp[k] == '\n' ) {
				tmp[k] = ' ';
			}
			if ( tmp[0] != ' ' || tmp[1] != ' ' ){
				while ( put_check != 0 ) {	// lock here until put the character to array
					put_check = buf_put( tmp[k] , buf_p1 );
					/*if ( put_check == 1 ) {
						pthread_yield();
					}*/
				}
				put_check = 1;
			}
		}else {
			buf_setEOI ( buf_p1 );	// Set flag 1 when finish the job
			break;
		}
	}
	return 0;
}

// Function that transform all the lowercase characters to uppercase
void *low_to_upper () {
	int get_check=1,put_check=1;
	char tmp;

	while ( 1 ) {
		while ( get_check == 1 ) {	// lock until he can read a character from the "buffer" array
			get_check = buf_get ( &tmp, buf_p1 );
			/*if ( get_check == 1 ) {
				pthread_yield();
			}*/
		}

		if ( get_check == 2 ) {		// Set flag 1 when read all the available data from the array and p1 finished her job
			buf_setEOI ( buf_p2 );
			return 0;
		}else {
			get_check = 1;
		}

		// Make the transformation
		if ( isalpha ( tmp ) && islower ( tmp ) ) {
			tmp = toupper ( tmp );
		}
		put_check = 1;
		while ( put_check != 0 ) {	// Lock until she can put a character to "buffer" array
			put_check = buf_put( tmp , buf_p2 );
			/*if ( put_check == 1 ) {
				pthread_yield();
			}*/
		}	
	}
}

// Print the document and put a '\n' every 30 characters
void *split_the_document () {
	int get_check=1,ch_counter=0;
	char tmp;

	while ( 1 ) {
		while ( get_check == 1 ) {	// lock until he can read a character from the "buffer" array
			get_check = buf_get ( &tmp, buf_p2 );
			/*if ( get_check == 1 ) {
				pthread_yield();
			}*/
		}
		if ( get_check == 2 ) {
			termination = 0;
			return 0;
		}else {
			get_check = 1;
		}
		get_check = 1;
		printf("%c", tmp);
		ch_counter++;
		ch_counter = ch_counter % 30;
		if ( ch_counter==0 ) {
			printf("\n");
		}
	}
}

int main ( int argc, char *argv[]) {
	int thr_arg_fd,iret1,iret2,iret3;
	pthread_t thread1,thread2,thread3;

	thr_arg_fd = open ( argv[1], O_RDWR | O_CREAT, 0777 );
	if ( thr_arg_fd < 0 ) {
		perror ( "Error Opening File\n" );
		return 1;
	}

	buf_init ( );

	iret1 = pthread_create ( &thread1, NULL, remove_characters, (void*)&thr_arg_fd );
	if( iret1 ){
		fprintf ( stderr, "Error - pthread_create() return code: %d\n", iret1 );
		exit(EXIT_FAILURE);
	}

	iret2 = pthread_create ( &thread2, NULL, low_to_upper, NULL );
	if( iret2 ){
		fprintf ( stderr, "Error - pthread_create() return code: %d\n", iret2 );
		exit(EXIT_FAILURE);
	}

	iret3 = pthread_create ( &thread3, NULL, split_the_document, NULL );
	if( iret3 ){
		fprintf ( stderr, "Error - pthread_create() return code: %d\n", iret3 );
		exit(EXIT_FAILURE);
	}

	while ( termination != 0 ) {}
	printf("\n");
	free ( buf_p1 );
	free ( buf_p2 );

	return 0;
}

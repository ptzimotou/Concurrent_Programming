#include "mandelcalc.h"
#include <stdlib.h>
#include <stdio.h>
#define N 100  // Thread number
/* sub-region parameters */

typedef struct {
  long double reBeg,reStep;  
  long double imBeg,imStep;
  int rePixels,imPixels;
  int maxIterations;
  int *res; /* array where to store result values */
  int *rdy; /* set 1 when done */
  int *jobTaken;  // flag to know if another thread took this specific job
} sliceMPars;


int ticket[N] = { 0 };
int enter[N] = { 0 };
int alreadyTaken = 0;
int calcBlog = 1; // Flag to wait the threads for main to make the jobs

sliceMPars *p;

/* the mandelbrot test */

static int isMandelbrot(double re, double im, int maxIterations) {
  long double zr = 0.0, zi = 0.0;
  long double xsq = 0.0, ysq = 0.0;
  long double cr = re, ci = im;
  int n = 0;

  while(xsq + ysq < 4.0 && n < maxIterations) {
    xsq = zr * zr;
    ysq = zi * zi;
    zi = 2 * zr * zi + ci;
    zr = xsq - ysq + cr;
    n++;
  }
  return(n);

}

/* perform mandelbrot computation for a sub-region */

static void computeMandelbrot(sliceMPars *p) {
  long double re,im; 
  int x,y;

  im = p->imBeg;
  for (y=0; y<p->imPixels; y++) {
    re = p->reBeg;
    for (x=0; x<p->rePixels; x++) {
      p->res[y*p->rePixels+x] = isMandelbrot (re,im,p->maxIterations);
      re = re + p->reStep;
    }
    im = im + p->imStep;
  }

  *(p->rdy)=1;

}

int findMax ( int num_of_threads ) {
  int max=0,i;

  for ( i=0; i<num_of_threads; i++ ) {
    if ( ticket[i] > max ) {
      max = ticket[i];
    }
  }

  return max;
}

// Function to lock threads
void lockThreads ( int id_num,int num_of_threads ) {
  int i;

  enter[id_num] = 1 ;
  ticket[id_num] = findMax ( num_of_threads ) + 1; 
  enter[id_num] = 0 ;
  for ( i=0; i<N; i++ ) {
    while (enter[i]) {
      continue;
    }
    while ( ( ticket[i] != 0 ) && ( (ticket[i] < ticket[id_num]) || ( ticket[i] == ticket[id_num] && i < id_num ) ) ) {
		//pthread_yield();
	} 
  }
}

// Function to unlock threads
void unlockThreads ( int id_num ) {
  ticket[id_num] = 0;
}

// Function that the thread find the next available job to take
int takeAjob ( int id_num, mandelPars *pars, int num_of_threads ) {
  int i,job=-1;


  lockThreads ( id_num, num_of_threads);
  for ( i = alreadyTaken; i<pars->slices; i++) {

    if ( *p[i].jobTaken == 0 ) {
      *p[i].jobTaken = 1;
      alreadyTaken++;
      job = i;
      break;
    }else if ( (i+1) >= pars->slices ) {
      job = -1;
      break;
    }
  }

  unlockThreads ( id_num );
  
  return job;
}

// Main code for threads
void *workerJob ( void *arg ) {
  int job;
  thread_arg *args = (thread_arg*)arg;

  while ( 1 ) {
    while ( calcBlog == 1 ) {
		//pthread_yield();
	}
    job = takeAjob ( args->id_num , args->pars , args->num_of_threads ) ;
    if ( job == -1 ) {
		//pthread_yield();
		continue;
    }
    if ( (job + 1) == args->pars->slices ) {
      calcBlog = 1;
    }
    computeMandelbrot ( &p[job] ) ;
  }
  return 0;
} 
/* perform computation for an entire region */

void calcMandel (mandelPars *pars, int *res, int *rdy, int *jobTaken ) {
  int k;
  long double imBeg;
  long double reStep = (pars->reEnd - pars->reBeg) / pars->rePixels;
  long double imStep = (pars->imEnd - pars->imBeg) / pars->imPixels;
  int sliceImPixels = pars->imPixels / pars->slices;
  int sliceTotPixels = sliceImPixels * pars->rePixels;
  
  /* allocate array for sub-region parameters */
  p = (sliceMPars *)malloc(pars->slices * sizeof(sliceMPars));

  /* fill in paramters for each sub-region */
  alreadyTaken = 0;
  imBeg = pars->imBeg;
  for (k=0; k<pars->slices; k++) {
    p[k].reBeg = pars->reBeg;
    p[k].reStep = reStep;
    p[k].rePixels = pars->rePixels;
    p[k].imBeg = imBeg;
    p[k].imStep = imStep;
    p[k].imPixels = sliceImPixels;
    p[k].maxIterations = pars->maxIterations;
    p[k].res = &res[k*sliceTotPixels];
    p[k].rdy = &rdy[k];
    p[k].jobTaken = &jobTaken[k];
    imBeg =  imBeg + imStep*sliceImPixels;
  }


  calcBlog = 0;
  // Wait until all the threads do their jobs
  for (k=0; k<pars->slices; k++) {
    while ( *p[k].rdy == 0 ) { 
		//pthread_yield();
	}
  }

  /* release parameter array */
  free(p);

}


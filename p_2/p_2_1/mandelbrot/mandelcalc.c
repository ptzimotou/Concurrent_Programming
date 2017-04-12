/*  Tzimotoudis Panagiotis 1669
    Tziokas Giorgos 1592 */

#include "mandelcalc.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
/* sub-region parameters */

int alreadyTaken = 0;
int slices;


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

// Function that the thread find the next available job to take
int takeAjob () {
  int i,job=-1;


  pthread_mutex_lock(&job_mtx);
  for ( i = alreadyTaken; i < slices; i++) {

    if ( *p[i].jobTaken == 0 ) {
      *p[i].jobTaken = 1;
      alreadyTaken++;
      job = i;
      break;
    }
  }

  pthread_mutex_unlock(&job_mtx);
  
  return job;
}

// Main code for threads
void *workerJob ( void *arg ) {
  int job,pid;
  pid = *(int *)arg;

  while ( 1 ) {
    pthread_mutex_lock(&worker_mtx[pid]);
    job = takeAjob () ;
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
  

  /* fill in paramters for each sub-region */
  alreadyTaken = 0;
  slices = pars->slices;
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

  /* release parameter array */
  //free(p);

}


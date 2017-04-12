/*  Tzimotoudis Panagiotis 1669
    Tziokas Giorgos 1592 */

#include <pthread.h>

typedef struct {
  long double reBeg,reEnd;  /* range of real values */
  long double imBeg,imEnd;  /* range of imaginary values */
  int rePixels,imPixels;    /* number of pixels per range */
  int maxIterations;        /* iteration cut off value */
  int slices;               /* number of sub-computations */
} mandelPars;

/* assume imPixels % slices == 0 */

typedef struct {
  long double reBeg,reStep;  
  long double imBeg,imStep;
  int rePixels,imPixels;
  int maxIterations;
  int *res; /* array where to store result values */
  int *rdy; /* set 1 when done */
  int *jobTaken;  // flag to know if another thread took this specific job
} sliceMPars;

sliceMPars *p;
pthread_mutex_t *worker_mtx;
pthread_mutex_t job_mtx;
extern void calcMandel(mandelPars *pars, int *res, int *rdy, int *jobTaken );
extern void *workerJob ( void *arg);	// Thread function
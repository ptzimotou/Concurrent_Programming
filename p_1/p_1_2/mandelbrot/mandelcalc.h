typedef struct {
  long double reBeg,reEnd;  /* range of real values */
  long double imBeg,imEnd;  /* range of imaginary values */
  int rePixels,imPixels;    /* number of pixels per range */
  int maxIterations;        /* iteration cut off value */
  int slices;               /* number of sub-computations */
} mandelPars;

/* assume imPixels % slices == 0 */

// Struct for list that we want to give in pthread as a parameter
typedef struct thread_arguments {
  int id_num;
  mandelPars *pars;
  int num_of_threads;
  struct thread_arguments *nxt;
}thread_arg;

extern void calcMandel(mandelPars *pars, int *res, int *rdy, int *jobTaken );
extern void *workerJob ( void *arg);	// Thread function
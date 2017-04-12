typedef struct list {
  pthread_mutex_t wait_mtx;
  struct list *nxt;
}wait;

synch ( Label ){
  wait *root_label;
  wait *end_label;
  pthread_mutex_t mtx_label = PTHREAD_MUTEX_INITIALIZER;
}

synch_beg ( Label ) {
  pthread_mutex_lock ( &mtx_label );
}

synch_wait ( Label ) {
  wait *curr;
  curr = (wait *)malloc(sizeof(wait));
  curr->nxt = NULL;
  pthread_mutex_init( &curr->wait_mtx, NULL );
  pthread_mutex_lock ( &wait_mtx );
  if ( end_label == NULL ) {
    end_label = curr;
    root_label = end_label;
  }else {
    end_label->nxt = curr;
    end_label = curr;
  }
  pthread_mutex_unlock ( &mtx_label );
  pthread_mutex_lock ( &curr->wait_mtx );
  pthread_mutex_destroy ( &curr->wait_mtx );
  free(curr);
  pthread_mutex_lock ( &mtx_label );
}

synch_notify ( Label ) {

  if ( root_label != NULL ) {
    wait *temp;
    temp = (wait *)malloc(sizeof(wait));
    temp = root_label;
    root_label = root_label->nxt;
    if ( root_label == NULL ) {
      end_label = root_label;
    }
    pthread_mutex_unlock( &temp->wait_mtx );
    //free ( temp );
  }
}

synch_notifyall ( Label ) {

  while ( root_label != NULL ) {
    wait *temp;
    temp = (wait *)malloc(sizeof(wait));
    temp = root_label;
    root_label = root_label->nxt;
    if ( root_label == NULL ) {
      end_label = root_label;
    }
    pthread_mutex_unlock( &temp->wait_mtx );
    free ( temp );
  }
}

synch_end ( Label ) {
  pthread_mutex_unlock ( &mtx_label );
}

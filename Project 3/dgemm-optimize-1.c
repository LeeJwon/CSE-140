#define BLOCKSIZE 32
// -------------------------function 1---------------------------------
//blocking/tiling
/*
int min(int x, int y){
    if( x < y){
        return x;
    }
    else{
        return y;
    }
}
void do_block(int m, int n, int si, int sk, int sj, float *A, float *C){
    int temp1 = min(si+BLOCKSIZE, m);
    int temp2 = min(sk+BLOCKSIZE, n);
    int temp3 = min(sj+BLOCKSIZE, m);
    for(int i = si; i < temp1; i++){
        for(int k = sk; k < temp2; k++){
            for(int j = sj; j < temp3; j++){
                C[i+j*m] += A[i+k*m] * A[j+k*m];
            }
        }
    }
}
void dgemm( int m, int n, float *A, float *C ){
    for(int si = 0; si < m; si += BLOCKSIZE){
        for(int sk = 0; sk < n; sk += BLOCKSIZE){
            for(int sj = 0; sj < m; sj += BLOCKSIZE){
                    do_block(m, n, si, sk, sj, A, C);
            }
        }
    }
}
*/

// -------------------------function 2---------------------------------
//unrolling
/*
void dgemm( int m, int n, float *A, float *C )
{
  int j;
  for( int i = 0; i < m; i++ ){
    for( int k = 0; k < n; k++ ){ 
      for(j = 0; j + 4 - 1 < m; j += 4){
	    C[i+j*m] += A[i+k*m] * A[j+k*m];
        C[i+(j+1)*m] += A[i+k*m] * A[j+1+k*m];
        C[i+(j+2)*m] += A[i+k*m] * A[j+2+k*m];
        C[i+(j+3)*m] += A[i+k*m] * A[j+3+k*m];
      }
      while(j < m){
          C[i+j*m] += A[i+k*m] * A[j+k*m];
          j++;
      }
    }
  }
}
*/


// -------------------------function 3---------------------------------
//Pre-Fetching
void dgemm( int m, int n, float *A, float *C )
{
    for( int i = 0; i < m; i++ ){
        for( int k = 0; k < n; k++ ) {
            for( int j = 0; j < m; j++ ) {
                __builtin_prefetch(&A[(j+8)+k*m]);
                __builtin_prefetch(&A[(j+7)+k*m]);
                __builtin_prefetch(&A[(j+6)+k*m]);
	            C[i+j*m] += A[i+k*m] * A[j+k*m];
            }
        }
    }
}

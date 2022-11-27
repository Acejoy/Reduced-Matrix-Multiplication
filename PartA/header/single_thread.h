// Optimize this function
#include <immintrin.h>

void singleThread(int N, int *matA, int *matB, int *output)
{ 

  assert( N>=4 and N == ( N &~ (N-1)));
  
  __m256i vec_prod = _mm256_setzero_si256();
  __m256i vec_mat_row1 = _mm256_setzero_si256();
  __m256i vec_mat_row2 = _mm256_setzero_si256();
  __m256i vec_mat_col1 = _mm256_setzero_si256();
  __m256i vec_mat_col2 = _mm256_setzero_si256();
  
  for (int k = 0; k < N; k+=8) {
    for (int colB = 0; colB < N; colB+=2) {
      
      vec_mat_col1= _mm256_setr_epi32(matB[k*N+colB], matB[(k+1)*N+colB], matB[(k+2)*N+colB], matB[(k+3)*N+colB], matB[(k+4)*N+colB], matB[(k+5)*N+colB], matB[(k+6)*N+colB], matB[(k+7)*N+colB]);
      vec_mat_col2= _mm256_setr_epi32(matB[k*N+(colB+1)], matB[(k+1)*N+(colB+1)], matB[(k+2)*N+(colB+1)], matB[(k+3)*N+(colB+1)], matB[(k+4)*N+(colB+1)], matB[(k+5)*N+(colB+1)], matB[(k+6)*N+(colB+1)], matB[(k+7)*N+(colB+1)]);

      for (int rowA = 0; rowA < N; rowA+=2) {

        // for(int ii=0; ii<N;)
        vec_mat_row1 = _mm256_loadu_si256((__m256i *)&matA[rowA*N+k]);
        vec_mat_row2 = _mm256_loadu_si256((__m256i *)&matA[(rowA+1)*N+k]);

        vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row1, vec_mat_col1));
        vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row1, vec_mat_col2));
        vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row2, vec_mat_col1));
        vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row2, vec_mat_col2));

        int rowC = rowA>>1;
        int colC = colB>>1;
        int indexC = rowC * (N>>1) + colC;

        output[indexC]+= _mm256_extract_epi32(vec_prod, 0)+_mm256_extract_epi32(vec_prod, 1)+_mm256_extract_epi32(vec_prod, 2)+_mm256_extract_epi32(vec_prod, 3)+_mm256_extract_epi32(vec_prod, 4)+_mm256_extract_epi32(vec_prod, 5)+_mm256_extract_epi32(vec_prod, 6)+_mm256_extract_epi32(vec_prod, 7);

        vec_prod = _mm256_setzero_si256();
      }
      
      
    }
  }
}

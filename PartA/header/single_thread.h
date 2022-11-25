// Optimize this function

void singleThread(int N, int *matA, int *matB, int *output)
{ 

  int block =16;

  assert( N>=4 and N == ( N &~ (N-1)));
  // for(int rowA = 0; rowA < N; rowA +=2) {
  //   for(int colB = 0; colB < N; colB += 2){
  //     int sum = 0;
  //     for(int iter = 0; iter < N; iter++) 
  //     {
  //       sum += matA[rowA * N + iter] * matB[iter * N + colB];
  //       sum += matA[(rowA+1) * N + iter] * matB[iter * N + colB];
  //       sum += matA[rowA * N + iter] * matB[iter * N + (colB+1)];
  //       sum += matA[(rowA+1) * N + iter] * matB[iter * N + (colB+1)];
  //     }

  //     // compute output indices
  //     int rowC = rowA>>1;
  //     int colC = colB>>1;
  //     int indexC = rowC * (N>>1) + colC;
  //     output[indexC] = sum;
  //   }
  // }

  for(int iter = 0; iter< N; iter+=block){
    for(int rowA = 0; rowA < N; rowA+=block){
      for(int colB = 0; colB < N; colB+=block){
        for(int rowrr = rowA; rowrr < rowA + block; rowrr+=2){
          for(int colcc = colB; colcc < colB + block; colcc+=2){

            int sum=0;
            for(int iterii = iter; iterii < iter + block; iterii++){
              sum += matA[rowrr * N + iterii] * matB[iterii * N + colcc];
              sum += matA[(rowrr+1) * N + iterii] * matB[iterii * N + colcc];
              sum += matA[rowrr * N + iterii] * matB[iterii * N + (colcc+1)];
              sum += matA[(rowrr+1) * N + iterii] * matB[iterii * N + (colcc+1)];
            }

            int rowC = rowrr>>1;
            int colC = colcc>>1;
            int indexC = rowC * (N>>1) + colC;
            output[indexC] = sum;

          }
        }
      }
    }
  }
  
}

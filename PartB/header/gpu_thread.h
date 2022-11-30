

#include<iostream>
using namespace std;
__global__ void Multiplication(int N, int *matA, int *matB, int *output)
{
	int row_start=(blockIdx.y * blockDim.y + threadIdx.y)*2;
	int row_end= row_start+2;
	int col_start=(blockIdx.x * blockDim.x + threadIdx.x)*2;
	int col_end=col_start+2;

	
	
	int sum=0;
	
		for(int i=row_start;i<row_end;i++)
		{
			for(int j=col_start;j<col_end;j++)
			{
				for(int k=0;k<N;++k)
				{
					int element1=matA[i*N+k];
					int element2=matB[k*N+j];
					sum=sum+element1*element2;
				}
				
			}
			
		}
		output[(blockIdx.y * blockDim.y + threadIdx.y)*N/2+ (blockIdx.x * blockDim.x + threadIdx.x)]=sum;
	


} 

void gpuThread(int N, int *matA, int *matB, int *output)
{
	int blocksize = 4;// 8,16,32
	
	dim3 dimGrid((N>>1)/blocksize,(N>>1)/blocksize);
	dim3 dimBlock(blocksize,blocksize);
	
	int size=N*N*sizeof(int);
	int size1=(N/2)*(N/2)*sizeof(int);
	int *A,*B,*C;
	
	
	cudaMalloc((void**)&A,size);
	cudaMemcpy(A,matA,size,cudaMemcpyHostToDevice);
	
	cudaMalloc((void**)&B,size);
	cudaMemcpy(B,matB,size,cudaMemcpyHostToDevice);
	
	cudaMalloc((void**)&C,size1);
	cudaMemcpy(C,output,size1,cudaMemcpyHostToDevice);
	
	cudaEvent_t start, stop;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);
	cudaEventRecord(start);
	Multiplication<<<dimGrid,dimBlock>>>(N,A,B,C);
	cudaEventRecord(stop);
	cudaDeviceSynchronize();
	
	cudaMemcpy(output,C,size1,cudaMemcpyDeviceToHost);
	
	float time= 0;
	cudaEventElapsedTime(&time, start, stop);
	
	printf("%f ms\n",time);
	
	cudaFree(A);
	cudaFree(B);
	cudaFree(C);
	
}

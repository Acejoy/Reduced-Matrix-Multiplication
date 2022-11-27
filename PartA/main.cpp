#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <fstream>
#include <assert.h>
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <inttypes.h>

using namespace std;

#define TIME_NOW std::chrono::high_resolution_clock::now()
#define TIME_DIFF(gran, start, end) std::chrono::duration_cast<gran>(end - start).count()

#include "single_thread.h"
#include "multi_thread.h"

struct perf_entry{
    struct perf_event_attr pea;
    long long count;
    int fd;
};

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                    group_fd, flags);
    return ret;
}

// Used to cross-check answer. DO NOT MODIFY!
void reference(int N, int *matA, int *matB, int *output)
{
  // enforce N to be power of 2 and greater than 2
  assert( N>=4 and N == ( N &~ (N-1)));
  for(int rowA = 0; rowA < N; rowA +=2) {
    for(int colB = 0; colB < N; colB += 2){
      int sum = 0;
      for(int iter = 0; iter < N; iter++) 
      {
        sum += matA[rowA * N + iter] * matB[iter * N + colB];
        sum += matA[(rowA+1) * N + iter] * matB[iter * N + colB];
        sum += matA[rowA * N + iter] * matB[iter * N + (colB+1)];
        sum += matA[(rowA+1) * N + iter] * matB[iter * N + (colB+1)];
      }

      // compute output indices
      int rowC = rowA>>1;
      int colC = colB>>1;
      int indexC = rowC * (N>>1) + colC;
      output[indexC] = sum;
    }
  }
}

int main(int argc, char *argv[])
{
  int m=5;
  struct perf_entry pArr[5];

  // Input size of square matrices
  ofstream output_file; 
  string OUTPUT_FILE_NAME = "TimeTaken.txt";  
  output_file.open(OUTPUT_FILE_NAME, ios::app); 
  

  int N;
  string file_name; 
  if (argc < 2) 
    file_name = "data/input_8192.in"; 
  else 
    file_name = argv[1]; 
  ifstream input_file; 
  input_file.open(file_name); 
  input_file >> N;
  cout << "Input matrix of size " << N << "\n";

  output_file<<"Size:"<<N<<'\n';

  for(int i=0; i<m; i++){
      memset(&pArr[i].pea, 0, sizeof(struct perf_event_attr));     
  }

  pArr[0].pea.type = PERF_TYPE_HW_CACHE;
  pArr[1].pea.type = PERF_TYPE_HW_CACHE;
  pArr[2].pea.type = PERF_TYPE_HW_CACHE;
  pArr[3].pea.type = PERF_TYPE_HW_CACHE;
  pArr[4].pea.type = PERF_TYPE_SOFTWARE;

  
  for(int i=0; i<m; i++){
      pArr[i].pea.size = sizeof(struct perf_event_attr);
  }

  pArr[0].pea.config = PERF_COUNT_HW_CACHE_L1D |
              PERF_COUNT_HW_CACHE_OP_READ << 8 |
              PERF_COUNT_HW_CACHE_RESULT_MISS << 16;
      
  pArr[1].pea.config = PERF_COUNT_HW_CACHE_LL |
              PERF_COUNT_HW_CACHE_OP_READ << 8 |
              PERF_COUNT_HW_CACHE_RESULT_MISS << 16;
  
  pArr[2].pea.config = PERF_COUNT_HW_CACHE_LL |
              PERF_COUNT_HW_CACHE_OP_WRITE << 8 |
              PERF_COUNT_HW_CACHE_RESULT_MISS << 16;

  pArr[3].pea.config = PERF_COUNT_HW_CACHE_DTLB |
              PERF_COUNT_HW_CACHE_OP_READ << 8 |
              PERF_COUNT_HW_CACHE_RESULT_MISS << 16;
  
  pArr[4].pea.config = PERF_COUNT_SW_PAGE_FAULTS ;



  for(int i=0; i<m; i++){
      pArr[i].pea.disabled = 1;
      pArr[i].pea.exclude_kernel = 1;
      pArr[i].pea.exclude_hv = 1;

      pArr[i].fd = perf_event_open(&pArr[i].pea, 0, -1, -1, 0);

      if (pArr[i].fd == -1) {
          fprintf(stderr, "%d, Error opening leader %llx\n", i, pArr[i].pea.config);
          exit(EXIT_FAILURE);
      }
  }

    
  // Input matrix A
  int *matA = new int[N * N];
  for(int i = 0; i < N; ++i)
    for(int j = 0; j < N; ++j)
      input_file >> matA[i * N + j];

  // Input matrix B
  int *matB = new int[N * N];
  for(int i = 0; i < N; ++i)
    for(int j = 0; j < N; ++j)
      input_file >> matB[i * N + j];

  // Untimed, warmup caches and TLB
  int *output_reference = new int[(N>>1)*(N>>1)];
  reference(N, matA, matB, output_reference);

  // Execute reference program
  output_file<<"Reference:";
  auto begin = TIME_NOW;

  for(int i=0; i<m; i++){
      ioctl(pArr[i].fd, PERF_EVENT_IOC_RESET, 0);
      ioctl(pArr[i].fd, PERF_EVENT_IOC_ENABLE, 0);
  }

  reference(N, matA, matB, output_reference); 
  
  for(int i=0; i<m; i++){
      ioctl(pArr[i].fd, PERF_EVENT_IOC_DISABLE, 0);
      read(pArr[i].fd, &pArr[i].count, sizeof(long long));          
  }

  auto end = TIME_NOW;
  cout << "Time:" << 
    (double)TIME_DIFF(std::chrono::microseconds, begin, end) / 1000.0 << " ms\n";    
  cout<<"L1 Read misses:"<<pArr[0].count<<'\n';
  cout<<"LL Read misses:"<<pArr[1].count<<'\n';
  cout<<"LL Write misses:"<<pArr[2].count<<'\n';
  cout<<"TLB misses:"<<pArr[3].count<<'\n';
  cout<<"Page Faults:"<<pArr[4].count<<'\n';

  output_file<<(double)TIME_DIFF(std::chrono::microseconds, begin, end) / 1000.0 << "\t";
  output_file<<pArr[0].count<<'\t';
  output_file<<pArr[1].count<<'\t';
  output_file<<pArr[2].count<<'\t';
  output_file<<pArr[3].count<<'\t';
  output_file<<pArr[4].count<<'\n';

  /* if(N<=8){ */
  // cerr << "matA: " << endl;
  // for(int i=0; i<(N); i++){
  //   for(int j=0; j<N; j++){
  //     cerr << matA[i*N+j] << "\t";
  //   }
  //   cerr << endl;
  // }
  // cerr << "matB: " << endl;
  // for(int i=0; i<(N); i++){
  //   for(int j=0; j<N; j++){
  //     cerr << matB[i*N+j] << "\t";
  //   }
  //   cerr << endl;
  // }
  // cerr << "output_reference: " << endl;
  // for(int i=0; i<(N>>1); i++){
  //   for(int j=0; j<(N>>1); j++){
  //     cerr << output_reference[i*(N>>1)+j] << "\t";
  //   }
  //   cerr << endl;
  // }
  /* } */

  // Execute single thread
  output_file<<"Single-Thread:";

  int *output_single = new int[(N>>1)*(N>>1)];
  begin = TIME_NOW;

  for(int i=0; i<m; i++){
      ioctl(pArr[i].fd, PERF_EVENT_IOC_RESET, 0);
      ioctl(pArr[i].fd, PERF_EVENT_IOC_ENABLE, 0);
  }

  singleThread(N, matA, matB, output_single);

  for(int i=0; i<m; i++){
      ioctl(pArr[i].fd, PERF_EVENT_IOC_DISABLE, 0);
      read(pArr[i].fd, &pArr[i].count, sizeof(long long));          
  }

  end = TIME_NOW;
  cout << "Time:" << 
    (double)TIME_DIFF(std::chrono::microseconds, begin, end) / 1000.0 << " ms\n";    
  cout<<"L1 Read misses:"<<pArr[0].count<<'\n';
  cout<<"LL Read misses:"<<pArr[1].count<<'\n';
  cout<<"LL Write misses:"<<pArr[2].count<<'\n';
  cout<<"TLB misses:"<<pArr[3].count<<'\n';
  cout<<"Page Faults:"<<pArr[4].count<<'\n';

  output_file<<(double)TIME_DIFF(std::chrono::microseconds, begin, end) / 1000.0 << "\t";
  output_file<<pArr[0].count<<'\t';
  output_file<<pArr[1].count<<'\t';
  output_file<<pArr[2].count<<'\t';
  output_file<<pArr[3].count<<'\t';
  output_file<<pArr[4].count<<'\n';


  for(int i = 0; i < ((N>>1)*(N>>1)); ++i)
    if(output_single[i] != output_reference[i]) {
      cout << "Mismatch at " << i << "\n";
      exit(0);
    }

  // Execute multi-thread
  output_file<<"Multi-Thread:";

  int *output_multi = new int[(N>>1)*(N>>1)];
  begin = TIME_NOW;

  for(int i=0; i<m; i++){
      ioctl(pArr[i].fd, PERF_EVENT_IOC_RESET, 0);
      ioctl(pArr[i].fd, PERF_EVENT_IOC_ENABLE, 0);
  }

  multiThread(N, matA, matB, output_multi);

  for(int i=0; i<m; i++){
      ioctl(pArr[i].fd, PERF_EVENT_IOC_DISABLE, 0);
      read(pArr[i].fd, &pArr[i].count, sizeof(long long));    
      close(pArr[i].fd);
  }


  end = TIME_NOW;
  cout << "Time:" << 
    (double)TIME_DIFF(std::chrono::microseconds, begin, end) / 1000.0 << " ms\n";    
  cout<<"L1 Read misses:"<<pArr[0].count<<'\n';
  cout<<"LL Read misses:"<<pArr[1].count<<'\n';
  cout<<"LL Write misses:"<<pArr[2].count<<'\n';
  cout<<"TLB misses:"<<pArr[3].count<<'\n';
  cout<<"Page Faults:"<<pArr[4].count<<'\n';

  output_file<<(double)TIME_DIFF(std::chrono::microseconds, begin, end) / 1000.0 << "\t";
  output_file<<pArr[0].count<<'\t';
  output_file<<pArr[1].count<<'\t';
  output_file<<pArr[2].count<<'\t';
  output_file<<pArr[3].count<<'\t';
  output_file<<pArr[4].count<<'\n';


// cerr << "output_multi: " << endl;
// for(int i=0; i<(N>>1); i++){
//   for(int j=0; j<(N>>1); j++){
//     cerr << output_multi[i*(N>>1)+j] << "\t";
//   }
//   cerr << endl;
// }
  

  for(int i = 0; i < ((N>>1)*(N>>1)); ++i)
    if(output_multi[i] != output_reference[i]) {
      cout << "Mismatch at " << i << "\n";
      exit(0);
    }

  input_file.close(); 
  output_file.close(); 
  return 0; 
}

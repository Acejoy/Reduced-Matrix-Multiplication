#include <pthread.h>
#include <immintrin.h>
// Create other necessary functions here
int g_N;
int *g_matA;
int *g_matB;
int *g_opMat;


// struct thread_arg
// {
//     int N;
//     int *matA;
//     int *matB;
//     int *opMat;
//     int *threadIdx;
// };

void *reducedMM(void *args)
{    
    int rowsperThread = g_N / 8;
    int colsperThread = g_N / 8;
    int threadIdx=(long)args;

    // cout<<threadIdx<<":\t";

    assert(g_N >= 4 and g_N == (g_N & ~(g_N - 1)));
    __m256i vec_prod = _mm256_setzero_si256();
    __m256i vec_mat_row1 = _mm256_setzero_si256();
    __m256i vec_mat_row2 = _mm256_setzero_si256();
    __m256i vec_mat_col1 = _mm256_setzero_si256();
    __m256i vec_mat_col2 = _mm256_setzero_si256();

    for (int k = 0; k < g_N; k += 8)
    {   
        // cout<<"Hello1";
        for (int colB = colsperThread*threadIdx ;colB < colsperThread*(threadIdx+1); colB += 2)
        {
            // cout<<"Hello2";
            vec_mat_col1 = _mm256_setr_epi32(g_matB[k * g_N + colB], g_matB[(k + 1) * g_N + colB], g_matB[(k + 2) * g_N + colB], g_matB[(k + 3) * g_N + colB], g_matB[(k + 4) * g_N + colB], g_matB[(k + 5) * g_N + colB], g_matB[(k + 6) * g_N + colB], g_matB[(k + 7) * g_N + colB]);
            vec_mat_col2 = _mm256_setr_epi32(g_matB[k * g_N + (colB + 1)], g_matB[(k + 1) * g_N + (colB + 1)], g_matB[(k + 2) * g_N + (colB + 1)], g_matB[(k + 3) * g_N + (colB + 1)], g_matB[(k + 4) * g_N + (colB + 1)], g_matB[(k + 5) * g_N + (colB + 1)], g_matB[(k + 6) * g_N + (colB + 1)], g_matB[(k + 7) * g_N + (colB + 1)]);

            for (int rowA = 0; rowA < g_N; rowA += 2)
            {

                // for(int ii=0; ii<g_N;)
                vec_mat_row1 = _mm256_loadu_si256((__m256i *)&g_matA[rowA * g_N + k]);
                vec_mat_row2 = _mm256_loadu_si256((__m256i *)&g_matA[(rowA + 1) * g_N + k]);

                vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row1, vec_mat_col1));
                vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row1, vec_mat_col2));
                vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row2, vec_mat_col1));
                vec_prod = _mm256_add_epi32(vec_prod, _mm256_mullo_epi32(vec_mat_row2, vec_mat_col2));

                int rowC = rowA >> 1;
                int colC = colB >> 1;
                int indexC = rowC * (g_N >> 1) + colC;

                // cout<<threadIdx<<' '<<rowA<<' '<<colB<<" entered at "<<rowC<<' '<<colC<<'\n';

                g_opMat[indexC] += _mm256_extract_epi32(vec_prod, 0) + _mm256_extract_epi32(vec_prod, 1) + _mm256_extract_epi32(vec_prod, 2) + _mm256_extract_epi32(vec_prod, 3) + _mm256_extract_epi32(vec_prod, 4) + _mm256_extract_epi32(vec_prod, 5) + _mm256_extract_epi32(vec_prod, 6) + _mm256_extract_epi32(vec_prod, 7);

                vec_prod = _mm256_setzero_si256();
            }
        }
    }

    // cout<<'\n';
    pthread_exit(NULL);
}

// Fill in this function
void multiThread(int N, int *matA, int *matB, int *output)
{
    int num_threads = 8;
    pthread_t threads[num_threads];
    //threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));

    g_N=N;
    g_matA=matA;
    g_matB=matB;
    g_opMat=output;
    

    int index[num_threads]={0};

    for(int i=0; i<num_threads; i++){
        index[i]=i;
    }


    for (int i = 0; i < num_threads; i++)
    {   
        int p=i;
        // cout<<i<<"#\n";
        pthread_create(&threads[i], NULL, reducedMM, (void *)i);
    }

    for (int i = 0; i < num_threads; i++)
    {
        // void *retval;
        pthread_join(threads[i], NULL);
    }
}

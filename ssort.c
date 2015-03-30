/* Parallel sample sort
*/
#include <stdio.h>
//#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"

static int compare(const void *a, const void *b)
{
    int *da = (int *)a;
    int *db = (int *)b;

    if (*da > *db)
        return 1;
    else if (*da < *db)
        return -1;
    else
        return 0;
}

int main( int argc, char *argv[])
{

    int rank, P;
    int i, N;
    int *vec;
    int root = 0;
    timestamp_type time0, time1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    /* Number of random numbers per processor (this should be increased
     * for actual tests or could be passed in through the command line */
    N = 100;
    if (argc > 1)
        int N = atoi(argv[1])/P;
    printf("%d processors with %d numbers each.\n", P, N);

    MPI_Barrier(MPI_COMM_WORLD);
    get_timestamp(&time0);

    vec = calloc(N, sizeof(int));
    /* seed random number generator differently on every core */
    //time_t timer = time(NULL);
    srand((unsigned int) 393919 + rank);//(unsigned int) (rank + 3939190));

    /* fill vector with random integers */
    for (i = 0; i < N; ++i)
        vec[i] = rand();
    //printf("rank: %d, first entry: %d\n", rank, vec[0]);

    /* sort locally */
    qsort(vec, N, sizeof(int), compare);

    /* randomly sample s entries from vector or select local splitters,
     * i.e., every N/ssize-th entry of the sorted vector */
    int ssize = 10;
    int *local = malloc(ssize*sizeof(int));
    //printf("N/s = %d\n", N/ssize);

    int ind = 0;
    for (i = 0; i < ssize; ++i) {
        local[i] = vec[ind];
        ind = ind + N/ssize;
    }

    /* every processor communicates the selected entries
     * to the root processor; use for instance an MPI_Gather */ 
    int *gather = NULL;
    if (rank == 0)
        gather = malloc(P*ssize*sizeof(int));
    
    MPI_Gather(local, ssize, MPI_INT, gather, ssize, MPI_INT, root, MPI_COMM_WORLD);

    /* root processor does a sort, determinates splitters that
     * split the data into P buckets of approximately the same size */
    int *splitters = malloc(P*sizeof(int));
    if (rank == 0) {
        qsort(gather, P*ssize, sizeof(int), compare); 
        for (i = 0; i < P; ++i)
            splitters[i] = gather[i*ssize];
            //printf("%d ", splitters[i]);
    }

    /* root process broadcasts splitters */
    MPI_Bcast(splitters, P, MPI_INT, root, MPI_COMM_WORLD);

    /* every processor uses the obtained splitters to decide
     * which integers need to be sent to which other processor (local bins) */
    int *sdispls = malloc((P+1)*sizeof(int));
    sdispls[0] = 0;
    sdispls[P] = N;
    int p = 1;
    for (i = 0; i < N; ++i) {
        if ((p < P) && (vec[i] > splitters[p])) {
            sdispls[p] = i;
            ++p;
        }
    }

    /* determine, send and recieve the counts for the number of integers
     * that each processor needs to send to each other processor */
    int *scounts = malloc(P*sizeof(int));
    for (i = 0; i < P; ++i)
        scounts[i] = sdispls[i+1]-sdispls[i];
    
    int *rdispls = malloc(P*sizeof(int));
    int *rcounts = malloc(P*sizeof(int));
    MPI_Alltoall(scounts, 1, MPI_INT, rcounts, 1, MPI_INT, MPI_COMM_WORLD);
    
    rdispls[0] = 0;
    for (i = 1; i < P; i++)
        rdispls[i] = rdispls[i-1] + rcounts[i-1];
   
////////////PRINT COUNTS/////////////////////////////////////////////////
/* 
    char buffer[256];
    int end = snprintf(buffer, 256*sizeof(char), "Process %d sending: ", rank);
    for (i = 0; i < P; ++i)
        end += snprintf(buffer + end, (256-end)*sizeof(char), " %d", scounts[i]);
    snprintf(buffer+end, (256-end)*sizeof(char),"\n");
    puts(buffer);
        
    char rbuffer[256];
    end = snprintf(rbuffer, 256*sizeof(char), "Process %d receiving:", rank);
    for (i = 0; i < P; ++i)
        end += snprintf(rbuffer + end, (256-end)*sizeof(char), " %d", rcounts[i]);
    snprintf(rbuffer+end, (256-end)*sizeof(char),"\n");
    puts(rbuffer);
*/
//////////////////////////////////////////////////////////////////////////
  
    /* send and receive: either you use MPI_AlltoallV, or
     * (and that might be easier), use an MPI_Alltoall to share
     * with every processor how many integers it should expect,
     * and then use MPI_Send and MPI_Recv to exchange the data */

    int total = 0;
    for (i = 0; i < P; i++)
        total += rcounts[i];
    int *recvbuf = malloc(total*sizeof(int));

    MPI_Alltoallv(vec, scounts, sdispls, MPI_INT, recvbuf, rcounts, rdispls, MPI_INT, MPI_COMM_WORLD);

    /* do a local sort */

    qsort(recvbuf, total, sizeof(int), compare);

    MPI_Barrier(MPI_COMM_WORLD);
    get_timestamp(&time1);
    double diff = timestamp_diff_in_seconds(time0, time1);
    if (rank == 0)
        printf("Sorted %d numbers on %d processors, %f seconds\n", P*N, P, diff);

    /* every processor writes its result to a file */

    char filename[50];
    sprintf(filename, "rank%d.txt", rank);
    FILE *file = fopen(filename, "w");

    for (i = 0; i < total; i++)
        fprintf(file, "%d\n", recvbuf[i]);

    fclose(file);
    
    free(vec);
    free(local);
    free(gather);
    free(splitters);
    free(scounts);
    free(sdispls);
    free(rcounts);
    free(rdispls);
    free(recvbuf);
    MPI_Finalize();
    return 0;
}

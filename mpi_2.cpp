#include "mpi.h"
#include <cstdio>
#include <cstdlib>
#include<ctime>
#include<cstring>
#include<limits.h>

/* Rank of the master task */
#define MASTER 0
const int INF = 0X7FFFFFFF;

void generate_rand_array(int * array, int size);
void swap(int* array, int posl, int posr);
void odd_sort(int *array, int size, bool &swapped);
void even_sort(int *array, int size, bool &swapped);
void compare_edge(int *array, int size, int id, int ntasks, bool &swap);



int main(int argc, char *argv[]) {
    double start, end;
    int ntasks;                            /* total number of tasks in partition */
    int rank;                              /* task identifier */
    int len;                               /* length of hostname */
    char hostname[MPI_MAX_PROCESSOR_NAME]; /* hostname */
    MPI_Init(&argc, &argv);
    /* how many processes */
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    /* who am I */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(hostname, &len);

    int size = atoi(argv[1]);
    int pad = (ntasks - size%ntasks)%ntasks;
    int p_size = (size+pad)/ntasks;
    int *arr = new int[size+pad];
    int *p_arr = new int[p_size];

    /* task with rank MASTER does this part */
    if (rank == MASTER) {
        printf("Using %d tasks to sort %d numbers...\n", ntasks, size);

        generate_rand_array(arr, size);

        /* print the first 20 elements in unsorted array */
        printf("The first 20 elements in the unsorted array: ");
        for (int i = 0; i < 20; i++) {
            printf("%d ", arr[i]);
        }
        printf("\n");

        start = MPI_Wtime();
        for (int i = size; i < size + pad; i++) {
            arr[i] = INT_MAX;
        }
    }

    /* Distribute the numbers */
    MPI_Scatter(arr, p_size, MPI_INT, p_arr, p_size, MPI_INT, MASTER, MPI_COMM_WORLD);
    
    bool global_swapped = true;
    bool swapped;
    int iter = 1;
    odd_sort(p_arr, p_size, swapped);
    MPI_Barrier(MPI_COMM_WORLD);
    compare_edge(p_arr, p_size, rank, ntasks, swapped);

    while (global_swapped ) {
        iter+=1;
        swapped = false;
        if (iter % 2 == 1) {
            /* Odd sort local elements in each process */
            odd_sort(p_arr, p_size, swapped);
            MPI_Barrier(MPI_COMM_WORLD);
            /* Sort on left nodes between processes */
            compare_edge(p_arr, p_size, rank, ntasks, swapped);
        } else {
            even_sort(p_arr, p_size, swapped);
            MPI_Barrier(MPI_COMM_WORLD);
            compare_edge(p_arr, p_size, rank, ntasks, swapped);
        }
        
        MPI_Allreduce(&swapped, &global_swapped, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD); 
    }

    /* Add up the numbers */
    MPI_Gather(p_arr, p_size, MPI_INT, arr, p_size, MPI_INT, MASTER, MPI_COMM_WORLD);

    end = MPI_Wtime();
    
    if (rank == MASTER) {
        printf("The first 20 elements in the sorted array: ");
            for (int i = 0; i < 20; i++) {
                printf("%d ", arr[i]);
            }
        printf("\n");
        printf("Runtime: %f\n", end-start);
    }
    
    delete[] p_arr;
    delete[] arr;
    MPI_Finalize();
    return 0;
}


void generate_rand_array(int * array, int size) {
    srand((int)time(0));
    for (int i = 0; i < size; i++) {
        array[i] = rand() % size;
    }
    return;
}

void swap(int* array, int posl, int posr) {
    int temp = array[posl];
    array[posl] = array[posr];
    array[posr] = temp;
}

void odd_sort(int *array, int size, bool &swapped) {
    for (int i = 0; i < size-1; i+=2) {
        if (array[i] > array[i+1]){
            swap(array, i, i+1);
            swapped = true;
        }
    }
    return;
}

void even_sort(int *array, int size, bool &swapped) {
    for (int i = 1; i < size-1; i+=2) {
        if (array[i] > array[i+1]){
            swap(array, i, i+1);
            swapped = true;
        }
    }
    return;
}

void compare_edge(int *array, int size, int id, int ntasks, bool &swapped) {
    MPI_Status status;
    int min = array[0];
    int max = array[size-1];
    int left = INT_MIN;
	int right = INT_MAX;


    /* method 1 */
    /* id: 0,1,2,3,...,ntasks */
    // if (id < ntasks-1)  {
    //     MPI_Send(&max, 1, MPI_INT, id + 1, 1, MPI_COMM_WORLD);
    // }
    // if (id >= 1) {
    //     MPI_Recv(&left, 1, MPI_INT, id - 1, 1, MPI_COMM_WORLD, &status);
    //     if (left > min) {
    //         int temp = array[0];
    //         array[0] = left;
    //         left = temp;
    //         swapped = true;
    //     }
    //     MPI_Send(&left, 1, MPI_INT, id - 1, 2, MPI_COMM_WORLD);
    // }

    // if (id < ntasks-1)  {
    //     MPI_Recv(&right, 1, MPI_INT, id + 1, 2, MPI_COMM_WORLD, &status);
    //     array[size-1] = right;
    // }

    /* method 2 */
    if (id < ntasks-1)  {
        MPI_Send(&max, 1, MPI_INT, id + 1, 1, MPI_COMM_WORLD);
    }
    if (id > 0)  {
        MPI_Send(&min, 1, MPI_INT,id - 1, 2, MPI_COMM_WORLD);
    }
    if (id < ntasks-1)  {
        MPI_Recv(&right, 1, MPI_INT, id + 1, 2, MPI_COMM_WORLD,&status);
    }
    if (id > 0)  {
        MPI_Recv(&left, 1, MPI_INT, id - 1, 1, MPI_COMM_WORLD,&status);
    }

	if(left > min){
		array[0] = left;
	}
	if(right < max){
		array[size - 1] = right;
	}
}
    





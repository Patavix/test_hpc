#include<iostream>
#include<algorithm>
#include<random>
#include<time.h>
#include<mpi.h>
 
#define N 16		//待排序的整型数量
 
using namespace std;
 
/*该函数用来获得某个阶段，某个进程的通信伙伴*/
int Get_Partner(int my_rank, int phase) {
	// 偶通信阶段，偶数为通信双方的较小进程
	if (phase % 2 == 0) {
		if (my_rank % 2 == 0) {
			return my_rank - 1;
		}
		else {
			return my_rank + 1;
		}
	}
	// 奇通信阶段，奇数为通信双方的较小进程
	else {
		if (my_rank % 2 == 0) {
			return my_rank + 1;
		}
		else {
			return my_rank - 1;
		}
	}
}
 
/*这个函数用来产生随机数，并分发至各个进程*/
void Get_Input(int A[], int local_n, int my_rank) {
	int* a = NULL;
	// 主进程动态开辟内存，产生随机数，分发至各个进程
	if (my_rank == 0) {
		a = new int[N];
		srand((int)time(0));
		for (int i = 0; i < N; i++) {
			a[i] = rand() % 1000;
		}
		MPI_Scatter(a, local_n, MPI_INT, A, local_n, MPI_INT, 0, MPI_COMM_WORLD);
		delete[] a;
	}
	// 其余进程接收随机数
	else {
		MPI_Scatter(a, local_n, MPI_INT, A, local_n, MPI_INT, 0, MPI_COMM_WORLD);
	}
}
 
/*该函数用来合并两个进程的数据，并取较小的一半数据*/
void Merge_Low(int A[], int B[], int local_n) {
	int* a = new int[local_n];		// 临时数组，倒腾较小的数据
	int p_a = 0, p_b = 0, i = 0;	//分别为A,B,a三个数组的指针
 
	// 这里不同担心数组越界，因为三个数组的大小是相等的
	while (i < local_n) {
		if (A[p_a] < B[p_b]) {
			a[i++] = A[p_a++];
		}
		else {
			a[i++] = B[p_b++];
		}
	}
	// 倒腾一下
	for (i = 0; i < local_n; i++) {
		A[i] = a[i];
	}
	delete[] a;
}
 
/*该函数用来合并两个进程的数据，并取较大的一半数据，与前面的Merge_Low函数类似*/
void Merge_High(int A[], int B[], int local_n) {
	int p_a = local_n - 1, p_b = local_n - 1, i = local_n - 1;
	int* a = new int[local_n];
	// 注意取最大值需要从后往前面取
	while (i >= 0) {
		if (A[p_a] > B[p_b]) {
			a[i--] = A[p_a--];
		}
		else {
			a[i--] = B[p_b--];
		}
	}
	for (i = 0; i < local_n; i++) {
		A[i] = a[i];
	}
	delete[] a;
}
 
/*这个函数用来输出排序后的数组*/
void Print_Sorted_Vector(int A[], int local_n, int my_rank) {
	int* a = NULL;
	// 0号进程接收各个进程的A的分量，并输出至控制台
	if (my_rank == 0) {
		a = new int[N];
		MPI_Gather(A, local_n, MPI_INT, a, local_n, MPI_INT, 0, MPI_COMM_WORLD);
		for (int i = 0; i < N; i++) {
			cout << a[i] << "\t";
			if (i % 4 == 3) {
				cout << endl;
			}
		}
		cout << endl;
		delete[] a;
	}
	// 其余进程将y分量发送至0号进程
	else {
		MPI_Gather(A, local_n, MPI_INT, a, local_n, MPI_INT, 0, MPI_COMM_WORLD);
	}
}
 
int main() {
	int local_n;	// 各个进程中数组的大小
	int* A, * B;	// A为进程中保存的数据，B为进程通信中获得的数据
	int comm_sz, my_rank;
 
	MPI_Init(NULL, NULL);
 
	// 获得进程数和当前进程的编号
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
 
	// 计算每个进程应当获得的数据量，动态开辟空间 
	local_n = N / comm_sz;
	A = new int[local_n];
	B = new int[local_n];
 
	// 随机产生数据并分发至各个进程
	Get_Input(A, local_n, my_rank);
 
	// 先有序化本地数据
	sort(A, A + local_n);
 
	// 定理：如果p个进程运行奇偶排序算法，那么p个阶段后，输入列表有序
	for (int i = 0; i < comm_sz; i++) {
		// 获得本次交换数据的进程号
		int partner = Get_Partner(my_rank, i);
		// 如果本次数据交换的进程号有效
		if (partner != -1 && partner != comm_sz) {
			// 与对方进程交换数据
			MPI_Sendrecv(A, local_n, MPI_INT, partner, 0, B, local_n, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
			// 如果本进程号大于目标进程，就应该取值较大的部分
			if (my_rank > partner) {
				Merge_High(A, B, local_n);
			}
			// 否则应该取值较小的部分
			else {
				Merge_Low(A, B, local_n);
			}
		}
	}
 
	// 打印排序后的数组
	Print_Sorted_Vector(A, local_n, my_rank);
 
	MPI_Finalize();
	return 0;
}
#define SIZE 1024
int __attribute__((aligned(4096))) A[SIZE] = { 0 };
int __attribute__((aligned(4096))) B[SIZE] = { 0 };
int __attribute__((aligned(4096))) C[SIZE] = { 0 };
int __attribute__((aligned(4096))) D[SIZE] = { 0 };
int __attribute__((aligned(4096))) E[SIZE] = { 0 };
int __attribute__((aligned(4096))) F[SIZE] = { 0 };
int __attribute__((aligned(4096))) G[SIZE] = { 0 };
int sum = 0;
int __attribute__((aligned(4096))) H[SIZE] = { 0 };
int __attribute__((aligned(4096))) I[SIZE] = { 0 };
int __attribute__((aligned(4096))) J[SIZE] = { 0 };
int __attribute__((aligned(4096))) K[SIZE] = { 0 };
int __attribute__((aligned(4096))) L[SIZE] = { 0 };
int __attribute__((aligned(4096))) M[SIZE] = { 0 };
int __attribute__((aligned(4096))) N[SIZE] = { 0 };
int __attribute__((aligned(4096))) O[SIZE] = { 0 };
int __attribute__((aligned(4096))) P[SIZE] = { 0 };
int __attribute__((aligned(4096))) Q[SIZE] = { 0 };
int __attribute__((aligned(4096))) R[SIZE] = { 0 };
int __attribute__((aligned(4096))) T[SIZE] = { 0 };
int __attribute__((aligned(4096))) U[SIZE] = { 0 };
int __attribute__((aligned(4096))) V[SIZE] = { 0 };
int __attribute__((aligned(4096))) W[SIZE] = { 0 };
int __attribute__((aligned(4096))) X[SIZE] = { 0 };
int __attribute__((aligned(4096))) Y[SIZE] = { 0 };
int __attribute__((aligned(4096))) Z[SIZE] = { 0 };
int _start() {
  for (int i = 0; i < SIZE; i++) A[i] = 2;
  for (int i = 0; i < SIZE; i++)
    sum += A[i];
  return sum;
}

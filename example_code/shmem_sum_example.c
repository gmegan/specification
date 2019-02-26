#include <stdio.h>
#include <stdlib.h>
#include <shmem.h>

#define N 10

int main(void)
{

   shmem_init();
   int me = shmem_my_pe();
   int npes = shmem_n_pes();

   int *values = shmem_malloc(N * sizeof(int));
   int *sums = shmem_malloc(N * sizeof(int));

   for (int i=0; i < N; i++) {
      values[i] = me + 1;
   }

   /* Wait for all PEs to initialize the values array: */
   shmem_sync(SHMEM_TEAM_WORLD);

   shmem_sum_to_all(sums, values, N, SHMEM_TEAM_WORLD);

   /* Check the result */
   for (int i = 0; i < N; i++) {
      if (sums[i] != npes * (npes + 1) / 2) {
         shmem_global_exit(1);
      }
   }

   shmem_free(values);
   shmem_free(sums);
   shmem_finalize();
   return 0;
}

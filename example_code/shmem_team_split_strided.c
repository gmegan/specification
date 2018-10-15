#include <stdio.h>
#include <shmem.h>

static long dest_e, src_e, dest_3, src_3;

long put_right(long *dest, long *src, int me, int npes, shmem_ctx_t ctx) {
  *src = (long)me;
  shmem_ctx_long_put(ctx, dest, src, 1, (me + 1) % npes);
  shmem_ctx_quiet(ctx);
  return (me == 0) ? (long)(npes-1) : long(me-1);
}

void team_put_ring(long *dest, long *src, shmem_team_t T) {
  shmem_ctx_t ctx;
  assert(shmem_team_create_ctx(T, 0, &ctx) == 0);
  
  long expect = put_right(dest, src, shmem_team_mype(T), shmem_team_npes(T), ctx);
  shmem_team_sync(T);
  assert(dest == expect);
}

int main(void) 
{
  static long src, dest;
  shmem_init();
  
  shmem_team_config_t conf;
  conf.num_threads = 1;
  long cmask = SHMEM_TEAM_NUM_THREADS;

  shmem_team_t even_team;
  shmem_team_split_strided(SHMEM_TEAM_WORLD, 0, 2, shmem_n_pes() / 2, &conf, cmask, &even_team);

  // Synchronization required between two splits into overlapping teams
  // E.g: PE 6 should not receive any traffic from PE 3 about the second split while it
  //      is still communicating for the first split. So PE 3 must wait until the first
  //      split is done, even though it is not involved in that operation
  shmem_team_sync(SHMEM_TEAM_WORLD);
  
  shmem_team_t threes_team;
  shmem_team_split_strided(SHMEM_TEAM_WORLD, 1, 3, shmem_n_pes() / 3, &conf, cmask, &threes_team);  

  if (even_team != SHMEM_TEAM_NULL) {
    team_put_ring(&dest_e, &src_e, even_team);
  }

  // No synchronization required between operations using two different teams
  // even when teams have overlapping membership, since we are using distinct src/dest buffers
  // E.g: PE 6 will not have trouble getting data from PE 3 in the second ring even
  //      if it is still getting data from PE 4 in the first ring. The teams data
  //      are distinct as are src and dest buffers
  
  if (threes_team != SHMEM_TEAM_NULL) {
    team_put_ring(&dest_3, &src_3, threes_team);
  }

  shmem_finalize();
}

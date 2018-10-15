#include <stdio.h>
#include <shmem.h>

// Return PE number to use in broadcast on team T or -1 if this isn't possible
int team_broadcast_access_id(int global_pe, shmem_team_t T) {
   int team_id = shmem_team_translate_pe(SHMEM_TEAM_WORLD, global_pe, T);
   if (team_id < 0) return -1;
   shmem_team_config_t conf;
   shmem_team_get_config(T, &conf);
   if (conf.disable_collectives != 0) return -1;
   return team_id;
}

// Return PE number to use in rma access using context ctx or -1 if this isn't possible
int team_rma_access_id(int global_pe, shmem_ctx_t ctx) {
   shmem_team_t T;
   shmem_ctx_get_team(ctx, &T);
   return shmem_team_translate_pe(SHMEM_TEAM_WORLD, global_pe, T);
}

int getit(long *dest, const long* src, int global_pe, shmem_team_t T, shmem_ctx_t ctx)
{
  int team_root_pe = team_broadcast_id(global_pe, T);
  if (team_root_pe >= 0) {
    //Use broadcast to get the data
    shmem_broadcast64(dest, src, 1, team_root_pe, T);
    return 1;
  }

  team_root_pe = team_rma_id(global_pe, ctx);
  if (team_root_pe >= 0) {
    //Use p2p to get the data
    shmem_ctx_long_get(ctx, dest, src, 1, team_root_pe);
    return 1;
  }

  return 0;
}

int main(void) 
{
  static long src, dest;
  shmem_init();
  
  shmem_team_config_t conf;

  conf.num_threads = 1;
  long cmask = SHMEM_TEAM_NOCOLLECTIVE | SHMEM_TEAM_NUM_THREADS;

  conf.disable_collectives = 1;
  shmem_team_t even_team;
  shmem_ctx_t even_ctx;
  shmem_team_split_strided(SHMEM_TEAM_WORLD, 0, 2, shmem_n_pes() / 2, &conf, cmask, &even_team);
  shmem_team_create_ctx(even_team, 0, &even_ctx);
  
  conf.disable_collectives = 0;
  shmem_team_t odd_team;
  shmem_ctx_t odd_ctx;
  shmem_team_split_strided(SHMEM_TEAM_WORLD, 1, 2, shmem_n_pes() / 2, &conf, cmask, &odd_team);
  shmem_team_create_ctx(odd_team, 0, &odd_ctx);

  int me = shmem_my_pe();
  src = dest = me;
  if (getit(&dest, &src, 4, even_team, even_ctx))
    printf ("%d: %ld\n", me, dest); //Expect dest = 4 if npes >= 5
  else
    printf ("%d: Could not get data from global pe 4 in even team\n", me);

  src = dest = me;
  if (getit(&dest, &src, 5, even_team, even_ctx))
    printf ("%d: %ld\n", me, dest);
  else
    printf ("%d: Could not get data from global pe 5 in even team\n", me); // Expect this

  src = dest = me;
  if (getit(&dest, &src, 5, odd_team, odd_ctx))
    printf ("%d: %ld\n", me, dest); //Expect dest = 5 if npes >= 6
  else
    printf ("%d: Could not get data from global pe 5 in odd team\n", me);

  shmem_finalize();
}

#include "action.h"
#include "constants.h"
#include "logger.h"
#include "manager.h"
#include <cstdio>
#include <cstring>

int main()
{

  LOG_INIT();

  int T, M, N, V, G, K;

  scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &K);

  REAL_CELL_NUM = V;

  V = V * 69 / 100;

  int period_num = (T - 1) / FRE_PER_SLICING + 1;

  printf("OK\n");

  fflush(stdout);

  Manager MAN(N, V, G, M, K, period_num);

  for (int i = 1; i <= 2; i++)
  {
    PERIOD = 1;
    MAN.update_tag_rank();
    for (TIMESTAMP = 1; TIMESTAMP <= T + EXTRA_TIME; TIMESTAMP++)
    {
      timestamp_action();
      delete_action(MAN);
      write_action(MAN);
      read_action(MAN);

      if(!IS_FIRST && TIMESTAMP%slice_len==0){
        SLICE++;
        MAN.update_tag_list();
        MAN.update_busy_area();
      }

      if (TIMESTAMP % 1800 == 0)
      {
        PERIOD++;
        change_action(MAN);
      }
    }
    MAN.Statistics();
    IS_FIRST = false;
    if (i == 1)
    {
      obj_tag_action(MAN);
      MAN.clear();
    }
  }
  return 0;
}
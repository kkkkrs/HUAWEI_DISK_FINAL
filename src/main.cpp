#include "action.h"
#include "constants.h"
#include "logger.h"
#include "manager.h"
#include <cstdio>
#include <cstring>
#include <iostream>

int main()
{

//  LOG_INIT();

  int T, M, N, V, G, K1, K2;

  scanf("%d%d%d%d%d%d%d", &T, &M, &N, &V, &G, &K1, &K2);

  REAL_CELL_NUM = V;

  V = V * 39 / 100;

  int period_num = (T - 1) / FRE_PER_SLICING + 1;

  printf("OK\n");

  fflush(stdout);

  Manager MAN(N, V, G, M, K1, K2, period_num);

  for (int i = 1; i <= 2; i++)
  {
    PERIOD = 1;
    MAN.update_tag_rank();
    for (TIMESTAMP = 1; TIMESTAMP <= T + EXTRA_TIME; TIMESTAMP++)
    {
      if (IS_FIRST)
        timestamp_action();
      delete_action(MAN);
      write_action(MAN);
      read_action(MAN);

      if (IS_FIRST && TIMESTAMP % forecast_window_len == 0)
      {
        MAN.forecast_tag();
      }

      if (TIMESTAMP % slice_len == 0)
      {
        SLICE++;
      }

      if (TIMESTAMP % 1800 == 0)
      {
        MAN.update_tag_list();
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

  for (TIMESTAMP = 1; TIMESTAMP <= T + EXTRA_TIME; TIMESTAMP++)
  {
    timestamp_action();

    printf("%s", delete_actions[TIMESTAMP].str().c_str());
    printf("%s", write_actions[TIMESTAMP].str().c_str());
    printf("%s", point_actions[TIMESTAMP].str().c_str());
    printf("%s", fin_actions[TIMESTAMP].str().c_str());
    fflush(stdout);

    if (TIMESTAMP + 105 >= MAX_TIME_SLICING)
    {
      printf("0\n");
    }
    else
    {
      printf("%s", busy_actions[TIMESTAMP + 105].str().c_str());
    }

    fflush(stdout);

    if (TIMESTAMP % 1800 == 0)
    {
      printf("%s", gc_actions[TIMESTAMP].str().c_str());
      fflush(stdout);
    }
  }

  return 0;
}
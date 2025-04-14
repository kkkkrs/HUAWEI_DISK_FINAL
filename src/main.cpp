#include "action.h"
#include "constants.h"
#include "logger.h"
#include "manager.h"
#include <cstdio>
#include <cstring>

int main() {

  project_init();

  int T, M, N, V, G, K;

  scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &K);

  REAL_CELL_NUM = V;

  V = V * 69 / 100;

  int period_num = (T - 1) / FRE_PER_SLICING + 1;

  Manager MAN(N, V, G, M, K, period_num);

  printf("OK\n");

  PERIOD = 1;

  fflush(stdout);

  for (int i = 1; i <= T + EXTRA_TIME; i++) {
    TIMESTAMP++;

    // MAN.Statistics();
    timestamp_action();
    delete_action(MAN);
    write_action(MAN);
    read_action(MAN);

    if (TIMESTAMP % 1800 == 0) {
      PERIOD++;
      change_action(MAN);
    }
  }

  return 0;
}

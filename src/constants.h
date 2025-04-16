#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <vector>

#define is_logging (true)

#define run_first (true)

#define run_second (true)

#define REP_NUM (3)
#define EXTRA_TIME (105)
#define FRE_PER_SLICING (1800)
#define MAX_TIME_SLICING (86400 + EXTRA_TIME + 1)

#define MAX_JUMP_TIME_BEFORE_PRE (13) //3-15

#define FIRST_Turn_down (72)

#define FIRST_TAG_AREA (0.32)

#define LEAST_READ_NUM (400) //

#define POINTER_RESTRICTION_RANGE (600) //指针之间的屏蔽范围

#define jump_req_num_threshold (1.3) //jump需要最小的阈值倍数

#define shield_request_time (15) //请求屏蔽范围

#define WINDOW_LEN (30) //15-30

#define forecast_window_len (300)

#define slice_len (900)

extern std::vector<int> DISK_START;

extern std::vector<int> TAG_RANK;

extern int TIMESTAMP; // 仅声明 TIMESTAMP
extern int PERIOD;
extern int SLICE;
extern int REAL_CELL_NUM;
extern double SCORE;
extern bool IS_FIRST;

extern double READ_SCORE;
extern double BUSY_SCORE;
extern int fin_num;
extern int busy_num;


extern std::vector<std::vector<int>> DELETE_DATA;
extern std::vector<std::vector<std::tuple<int, int, int>>> WRITE_DATA;
extern std::vector<std::vector<std::tuple<int, int>>> READ_DATA;

#endif // CONSTANTS_H

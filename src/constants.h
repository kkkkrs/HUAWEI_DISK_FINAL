#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <vector>

extern int TIMESTAMP; // 仅声明 TIMESTAMP

extern int PERIOD;

extern int REAL_CELL_NUM;

extern double SCORE;

extern int shield_request_time;

#define is_logging (true)

#define REP_NUM (3)
#define EXTRA_TIME (105)
#define FRE_PER_SLICING (1800)

#define MAX_JUMP_TIME_BEFORE_PRE (13) //3-15

#define LEAST_READ_NUM (520) //这个参数和token有关 //可以从200试到500

#define POINTER_RESTRICTION_RANGE (500) //指针之间的屏蔽范围 TODO: 修改为动态的值 //这个也要变小,300-900

#define jump_req_num_threshold (1.3) //jump需要最小的阈值倍数 //1.1 - 3?

// #define shield_request_time (15) //这个参数在constants.cpp，大概是从10-30，与上面的参数有关系

#define WINDOW_LEN (30) //15-30

extern std::vector<int> DISK_START;

extern std::vector<int> TAG_RANK;

#endif // CONSTANTS_H

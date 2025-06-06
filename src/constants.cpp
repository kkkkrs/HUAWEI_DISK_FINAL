#include "constants.h"
#include <vector>
#include <tuple>

int TIMESTAMP = 0;

int PERIOD = 0;

double SCORE = 0;

double READ_SCORE = 0;

double BUSY_SCORE = 0;

int fin_num = 0;

int busy_num = 0;

int SLICE = 0;

bool IS_FIRST = true;

int REAL_CELL_NUM;

std::vector<std::vector<int>> DELETE_DATA(MAX_TIME_SLICING);
std::vector<std::vector<std::tuple<int, int, int>>> WRITE_DATA(MAX_TIME_SLICING);
std::vector<std::vector<std::tuple<int, int>>> READ_DATA(MAX_TIME_SLICING);

std::vector<std::ostringstream> delete_actions(MAX_TIME_SLICING);
std::vector<std::ostringstream> write_actions(MAX_TIME_SLICING);
std::vector<std::ostringstream> point_actions(MAX_TIME_SLICING);
std::vector<std::ostringstream> fin_actions(MAX_TIME_SLICING);
std::vector<std::ostringstream> busy_actions(MAX_TIME_SLICING);
std::vector<std::ostringstream> gc_actions(MAX_TIME_SLICING);


std::vector<int> DISK_START;

std::vector<int> TAG_RANK;

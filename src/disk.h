#ifndef DISK_H
#define DISK_H

#include "entity.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

class Cell
{
public:
  Cell(std::unordered_map<int, Request> *request, std::unordered_map<int, Object> *objects)
  {
    this->block_id = 0;
    this->obj_id = 0;
    this->request = request;
    this->objects = objects;
    this->last_interview_time = -100;
    // this->latest_req_id = 0;
  }

  std::unordered_map<int, Request> *request;
  std::unordered_map<int, Object> *objects;
  int obj_id;
  int block_id;
  int last_interview_time;
  // int latest_req_id;
  // int req_num;
};

class Point
{
public:
  Point() {};
  Point(int disk_id, int point_id);

  int disk_id;
  int point_id;
  int position;
  int pre_token;
  int token;
  int last_jump_time;
  std::string actions;
  int since_last_jump;
  int jump_here;
  int reserve_end;
  
};

class Disk
{
public:
  Disk(int disk_id, int cell_num, int token_init,
       std::unordered_map<int, Object> *objects, std::unordered_map<int, Request> *request);

  std::vector<std::pair<int, int>> work(int point_id);
  std::pair<int, int> prosess_single_cell(int point_id, bool is_read);

  std::pair<int, int> find_jump_point(int point_id,bool is_shield_request);

  void refresh();

  std::string find_replacement_indices(const std::string &sequence, int pre_token);

  std::string get_ori_ops(int point_id);

  std::queue<int> Get_isolate_r(std::string ops,int tag);
  std::queue<int> Get_isolate_r2(std::string ops,int tag);

  std::vector<std::pair<int, int>> per_disk_exchange_cell2(std::vector<int> tag_list);

  int calculate_consum_token(int point_id);

  std::vector<int> clear_area(int area_id);

  bool cell_need_change(int cell_id);

  std::vector<int> write(int size, int obj_id, int tag, int is_write_all, bool is_last_rep);

  void move_point(int &point, bool is_f_to_b, bool restrict);

  int cell_need_read(int cell_id, int point_id, bool is_shield,bool is_shield_request);

  void update_reserve_end(int point_id);

  bool cell_is_Blank(int cell_id);

  int countGreaterThanTimestamp(const std::deque<int>& req_id_list, int timestamp);

  void mirror_exchange_cell(std::vector<std::pair<int, int>> change_list);

  // void check_jump();

  std::vector<std::pair<int, int>> per_disk_exchange_cell(std::vector<int> tag_list);

  std::vector<Cell> cells;

  std::unordered_map<int, Request> *request;
  std::unordered_map<int, Object> *objects;

  int disk_id;
  int mirror_disk_id;
  int cell_num;
  int init_token;
  int exchange_time;
  std::vector<Point> point;

  std::vector<Point*> mirror_point;
};

#endif // DISK_H

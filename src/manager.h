#ifndef MANAGER_H
#define MANAGER_H

#include "disk.h"
#include "entity.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

class Manager {
public:
  Manager(int disk_num, int cell_per_disk, int init_token, int tag_num,int change_times, int period_num);

  std::pair<std::vector<std::string>, std::vector<int>> read_batch();

  std::vector<int> delete_batch(const std::vector<int> &batch);

  void build_request(const std::vector<std::tuple<int, int>> &batch);

  std::vector<int> check_finish(std::vector<std::pair<int, int>> read_list);

  void write_into_first(std::vector<std::tuple<int, int, int>> wirte_per_timestamp);

  void write_into_second(std::vector<std::tuple<int, int, int>> wirte_per_timestamp);

  std::pair<std::vector<int>,std::vector<std::pair<int,int>>> exchange_cell();

  std::vector<int> busy_req();

  std::pair<int,int> find_disk(int tag);

  bool req_need_busy(int obj_id);

  void Statistics();

  void clear();

  int disk_num;
  int cell_per_disk;
  int tag_num;
  int period_num;
  int init_token;
  int init_exchange_time;
  int busy_num_last_period;
  int fin_num_last_period;

  std::vector<Disk> disk;
  std::unordered_map<int, Object> objects;
  std::unordered_map<int, Request> request;
  std::vector<int> tag_write_disk_id;

  std::vector<int> tag_list;

  std::vector<std::pair<int, int>> may_expired_req;

  std::vector<int> busy_req_list;
};

#endif // MANAGER_H

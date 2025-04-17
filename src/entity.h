#ifndef ENTITY_H
#define ENTITY_H

#include <queue>
#include <deque>
#include <vector>

class Object {
public:
  Object();
  std::vector<int> replica;
  std::vector<std::vector<int>> unit;
  int size;
  int tag;
  std::vector<int> block_req_num;
  std::deque<int> req_id_list;

  int write_area;

  int not_fin_req_num();

  void update_block_req_num();

  std::vector<int> interview_timestamp;
  int last_slice_interview_times;
  int create_timestamp;
  int delete_timestamp;
  bool is_read;
};

struct Request {
  int object_id;
  int create_timestamp;
};

#endif

#include "entity.h"
#include "constants.h"

Object::Object() {
  replica.resize(REP_NUM);
  unit.resize(REP_NUM);
  create_timestamp = 0;
  delete_timestamp = MAX_TIME_SLICING;
  // block_req_num.assign(size,0);
}

int Object::not_fin_req_num() {
  int maxnum = 0;
  for (int i = 0; i < size; i++) {
    maxnum = std::max(maxnum, block_req_num[i]);
  }
  return maxnum;
};

void Object::update_block_req_num() {

  for (int i = 0; i < size; i++) {
    if (block_req_num[i] > req_id_list.size()) {
      block_req_num[i] = req_id_list.size();
    }
  }
};


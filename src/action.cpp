#include "constants.h"
#include "logger.h"
#include "manager.h"
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

void project_init() {
//   auto file = freopen(
//     "/Users/apple/Documents/code/C++/Disk_V3/data/sample_practice.in", "r", stdin);
// assert(file != nullptr);
  PERIOD = 1;
  TIMESTAMP = 0;
  LOG_INIT();
};

void timestamp_action() {
  scanf("%*s%*d");
  printf("TIMESTAMP %d\n", TIMESTAMP);
//  ////  // LOG_INFO("TIMESTAMP %d", TIMESTAMP);
  fflush(stdout);
}

void delete_action(Manager &MAN) {

  int n_delete;

  scanf("%d", &n_delete);

  std::vector<int> ids(n_delete);
  for (int i = 0; i < n_delete; i++) {
    scanf("%d", &ids[i]);
  }

  auto result = MAN.delete_batch(ids); // 返回删除对象未完成的请求

  printf("%d\n", static_cast<int>(result.size()));

  for (const auto &r : result) {
    printf("%d\n", r);
  }

  fflush(stdout);
}

void write_action(Manager &MAN) {
  int n_write;

  scanf("%d", &n_write);

  for (int i = 0; i < n_write; i++) {
    int write_id, size, tag;

    scanf("%d %d %d", &write_id, &size, &tag);

    Object &obj = MAN.write_into(write_id, size, tag);

    printf("%d\n", write_id);

    for (int j = 0; j < REP_NUM; j++) {
      printf("%d", obj.replica[j]);
      for (int k = 0; k < size; k++) {
        printf(" %d", obj.unit[j][k]);
      }
      printf("\n");
    }
  }

  fflush(stdout);
}

void read_action(Manager &MAN) {

  int num_read;

  scanf("%d", &num_read);

  std::vector<std::pair<int, int>> requests(num_read);

  for (int i = 0; i < num_read; i++) {
    int request_id, object_id;

    scanf("%d %d", &request_id, &object_id);

    requests[i] = std::make_pair(request_id, object_id);
  }

  MAN.build_request(requests);

  auto result = MAN.read_batch();

  for (int i = 0; i < result.first.size(); i++) {
    printf("%s\n", result.first[i].c_str());
  }

  printf("%d\n", static_cast<int>(result.second.size()));

  for (const auto &id : result.second) {
    printf("%d\n", id);
  }

  std::vector<int> busy_req_list = MAN.busy_req();

  printf("%d\n", static_cast<int>(busy_req_list.size()));

  for (const auto &id : busy_req_list) {
    printf("%d\n", id);
  }

  fflush(stdout);
}

void change_action(Manager &MAN) {

  scanf("%*s %*s");

  printf("GARBAGE COLLECTION\n");


  // std::vector<std::pair<int,int>> ops;

  // std::vector<int> ops_size;

  auto [ops_size,ops] = MAN.exchange_cell();

  int index = 0;
  for(int i = 0; i<ops_size.size();i++){
      printf("%d\n",ops_size[i]);
//      // LOG_INFO("DISK %d 交换了 %d\n",i+1,ops_size[i]);
      for(int j = 0;j<ops_size[i];j++){
          printf("%d %d\n",ops[index].first,ops[index].second);
//          // LOG_INFO("交换 %d %d\n",ops[index].first,ops[index].second);
          index++;
      }
  }

  fflush(stdout);
}

#include "constants.h"
#include "logger.h"
#include "manager.h"
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>
#include <tuple>

void timestamp_action()
{
  scanf("%*s%*d");
  printf("TIMESTAMP %d\n", TIMESTAMP);
  fflush(stdout);
}

void delete_action(Manager &MAN)
{

  int n_delete;
  std::vector<int> ids;

  if (IS_FIRST)
  {
    scanf("%d", &n_delete);
    ids.resize(n_delete);
    for (int i = 0; i < n_delete; i++)
    {
      scanf("%d", &ids[i]);
    }
    DELETE_DATA[TIMESTAMP] = ids;
  }
  else
  {
    ids = DELETE_DATA[TIMESTAMP];
  }

  auto result = MAN.delete_batch(ids); // 返回删除对象未完成的请求

  printf("%d\n", static_cast<int>(result.size()));

  for (const auto &r : result)
  {
    printf("%d\n", r);
  }

  fflush(stdout);
}

void write_action(Manager &MAN)
{

  std::vector<std::tuple<int, int, int>> write_data;
  if (IS_FIRST)
  {
    int n_write;
    scanf("%d", &n_write);
    for (int i = 0; i < n_write; i++)
    {
      int write_id, size, tag;
      scanf("%d %d %d", &write_id, &size, &tag);
      write_data.push_back({write_id, size, tag});
    }
    WRITE_DATA[TIMESTAMP] = write_data;
    MAN.write_into_first(write_data);
  }
  else
  {
    write_data = WRITE_DATA[TIMESTAMP];
    MAN.write_into_second(write_data);
  }

  for (auto [obj_id, size, tag] : write_data)
  {
    printf("%d\n", obj_id);
    for (int j = 0; j < REP_NUM; j++)
    {
      printf("%d", MAN.objects[obj_id].replica[j]);
      for (int k = 0; k < size; k++)
      {
        printf(" %d", MAN.objects[obj_id].unit[j][k]);
      }
      printf("\n");
    }
  }
  fflush(stdout);
}

void read_action(Manager &MAN)
{

  std::vector<std::tuple<int, int>> requests;
  if (IS_FIRST)
  {
    int num_read;
    scanf("%d", &num_read);
    requests.resize(num_read);

    for (int i = 0; i < num_read; i++)
    {
      int request_id, object_id;
      scanf("%d %d", &request_id, &object_id);
      requests[i] = {request_id, object_id};
    }

    READ_DATA[TIMESTAMP] = requests;
  }
  else
  {
    requests = READ_DATA[TIMESTAMP];
  }

  MAN.build_request(requests);

  auto result = MAN.read_batch();

  for (int i = 0; i < result.first.size(); i++)
  {
    printf("%s\n", result.first[i].c_str());
  }

  printf("%d\n", static_cast<int>(result.second.size()));

  for (const auto &id : result.second)
  {
    printf("%d\n", id);
  }

  std::vector<int> busy_req_list = MAN.busy_req();

  printf("%d\n", static_cast<int>(busy_req_list.size()));

  for (const auto &id : busy_req_list)
  {
    printf("%d\n", id);
  }

  fflush(stdout);
}

void change_action(Manager &MAN)
{
  if (IS_FIRST)
  {
    scanf("%*s %*s");
  }
  printf("GARBAGE COLLECTION\n");

  auto [ops_size, ops] = MAN.exchange_cell();

  int index = 0;
  for (int i = 0; i < ops_size.size(); i++)
  {
    printf("%d\n", ops_size[i]);
    for (int j = 0; j < ops_size[i]; j++)
    {
      printf("%d %d\n", ops[index].first, ops[index].second);
      index++;
    }
  }

  fflush(stdout);
}

void obj_tag_action(Manager &MAN)
{

  int tag0num = 0;
  for (auto [obj_id, obj] : MAN.objects)
  {
    if(obj.tag==0){
      tag0num++;
    }
  }
  LOG_INFO("TAG为0的数量为 %d 总数量为 %d",tag0num,MAN.objects.size());

  int times = 0;
  scanf("%d", &times);
  for (int x = 0; x < times; x++)
  {
    int obj_id, tag;
    scanf("%d %d", &obj_id, &tag);
    MAN.objects[obj_id].tag = tag;
  }
  // 首先统计出每种tag的特征信息，然后遍历每一个tag为0的object，给他们加上tag信息
}
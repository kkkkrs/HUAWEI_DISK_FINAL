#include "manager.h"
#include "TagScheduler.h"
#include "constants.h"
#include "disk.h"
#include "entity.h"
#include "logger.h"
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <set>
#include <tuple>
#include <string>
#include <vector>

Manager::Manager(int disk_num, int cell_per_disk, int init_token, int tag_num, int exchange_time, int period_num)
    : disk_num(disk_num), cell_per_disk(cell_per_disk), tag_num(tag_num), period_num(period_num), init_exchange_time(exchange_time), init_token(init_token)
{

  this->fin_num_last_period = 0;

  this->tag_write_disk_id.assign(tag_num + 1, 1);

  for (int i = 0; i <= disk_num; i++)
  {
    disk.push_back(Disk(i, cell_per_disk, init_token, &objects, &request));
  }

  for (int i = 1; i <= disk_num; i++)
  {
    int mirror_disk = i > 5 ? i - 5 : i + 5;
    disk[i].mirror_disk_id = mirror_disk;
    disk[i].mirror_point.push_back(&disk[mirror_disk].point[0]);
    disk[i].mirror_point.push_back(&disk[mirror_disk].point[1]);
    disk[i].mirror_point.push_back(&disk[mirror_disk].point[2]);
  }
}

void Manager::Statistics()
{
  if(IS_FIRST){
    LOG_INFO("ROUND 1");
  }else{
    LOG_INFO("ROUND 2");
  }
  LOG_INFO("READ_NUM %d READ_SCORE %.2f", fin_num, READ_SCORE);
  LOG_INFO("BUSY_NUM %d BUSY_SCORE %.2f", busy_num, BUSY_SCORE);
  LOG_INFO("SCORE %.2f", SCORE);
}

std::pair<int, int> Manager::find_disk(int tag)
{

  int max_blank_space = 0;
  int max_disk = 0;
  int tag_middle = this->cell_per_disk * 0.33 / tag_num * tag;
  for (int i = 1; i <= 5; i++)
  {
    int cur_blank = 0;
    int right = tag_middle;
    while (right <= this->cell_per_disk)
    {
      if (disk[i].cells[right].obj_id == 0)
      {
        cur_blank++;
      }
      else
      {
        if (objects[disk[i].cells[right].obj_id].tag != tag)
        {
          break;
        }
      }
      right++;
    }

    int left = tag_middle;
    while (left >= 0)
    {
      if (disk[i].cells[left].obj_id == 0)
      {
        cur_blank++;
      }
      else
      {
        if (objects[disk[i].cells[left].obj_id].tag != tag)
        {
          break;
        }
      }
      left--;
    }
    if (cur_blank > max_blank_space)
    {
      max_blank_space = cur_blank;
      max_disk = cur_blank;
    }
  }
  return {max_disk, max_blank_space};
}

// 单个对象写入
void Manager::write_into_first(std::vector<std::tuple<int, int, int>> wirte_per_timestamp)
{
  for (auto [obj_id, size, tag] : wirte_per_timestamp)
  {
    // 对于tag不为0的对象
    // 首先根据tag选择区域，在五个磁盘中找到该区域的空闲区间最大的一个磁盘号返回，并且返回空闲区间大小
    // 然后判断空闲区间大小是否能够写入，如果可以就直接写入
    // 如果空闲区间大小不够，那么就继续向tag两边的区域延伸
    int disk_id = this->tag_write_disk_id[tag]; // 记录本体存在哪一个磁盘，副本在这上面+5

    objects[obj_id].size = size;
    objects[obj_id].tag = tag;
    objects[obj_id].block_req_num.assign(size, 0);
    objects[obj_id].create_timestamp = TIMESTAMP;

    int tag_skew = 0;

    int the_first_write_disk_id = 0;

    for (int i = 0; i < REP_NUM; i++)
    {
      // i = 0,第一个副本，存在磁盘本体 是 1，2，3，4，5
      // i = 1,第二个副本，存在磁盘副本 是 6，7，8，9，10
      if (i == 1)
      {
        continue;
      }

      bool is_last_rep = false; // 判断需不需要写入后三分之一

      if (i == 2) // 对于最后一个副本的情况
      {
        is_last_rep = true;
        disk_id = disk_id + 6;
        disk_id = disk_id == 11 ? 6 : disk_id;
        if (rand() % 2)
        {
          disk_id -= 5;
        }
      }

      int temp_cnt = 0;
      std::vector<int> tmp = disk[disk_id].write_first(size, obj_id, tag, tag_skew, is_last_rep);

      while (tmp.empty()) // 在写入磁盘副本的时候不可能为空，因为这个时候的disk_id是本体已经写入的id，磁盘副本一定可以写入
      {
        if (!is_last_rep)
        {
          disk_id = disk_id % 5 + 1;
        }
        else
        {
          disk_id = disk_id % this->disk_num + 1;
        }
        if (is_last_rep && (disk_id == the_first_write_disk_id + 5 || disk_id == the_first_write_disk_id))
        {
          disk_id = disk_id % this->disk_num + 1;
        }
        temp_cnt++;
        if (temp_cnt == 5)
        {
          tag_skew++;
          temp_cnt = 0;
        }

        tmp = disk[disk_id].write_first(size, obj_id, tag, tag_skew, is_last_rep);
        if (tmp.empty())
        {
          tmp = disk[disk_id].write_first(size, obj_id, tag, 0 - tag_skew, is_last_rep);
        }
      }

      // 现在temp里面存放的是 副本存的cell序列

      if (!i) // 现在是第一个副本，我可以直接写入第二个副本
      {
        for (int j = 0; j < tmp.size(); j++)
        {
          disk[disk_id + 5].cells[tmp[j]].obj_id = obj_id;
          disk[disk_id + 5].cells[tmp[j]].block_id = j;
        }
        objects[obj_id].replica[1] = disk_id + 5;
        objects[obj_id].unit[1] = tmp;
      }

      objects[obj_id].replica[i] = disk_id;
      objects[obj_id].unit[i] = tmp;
      the_first_write_disk_id = i == 0 ? disk_id : the_first_write_disk_id;
    }
    this->tag_write_disk_id[tag] = this->tag_write_disk_id[tag] % 5 + 1;
  }
}

void Manager::update_tag_rank()
{

  DISK_START.clear();
  TAG_RANK.clear();
  DISK_START.resize(this->tag_num + 1, 0);
  if (IS_FIRST)
  {
    TAG_RANK = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    DISK_START[0] = 1;
    for (int i = 0; i < 16; i++)
    {
      DISK_START[i + 1] = this->cell_per_disk * 0.33 / 16 + DISK_START[i];
    }
  }
  else
  {
    // TODO:
    TAG_RANK = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    DISK_START[0] = 1;
    for (int i = 0; i < 16; i++)
    {
      DISK_START[i + 1] = this->cell_per_disk * 0.55 / 16 + DISK_START[i];
    }
  }
}

void Manager::write_into_second(std::vector<std::tuple<int, int, int>> wirte_per_timestamp)
{
  for (auto [obj_id, size, _] : wirte_per_timestamp)
  {

    int tag = objects[obj_id].tag;
    int disk_id = this->tag_write_disk_id[tag]; // 记录本体存在哪一个磁盘，副本在这上面+5
    objects[obj_id].block_req_num.assign(size, 0);

    int tag_skew = 0;

    int the_first_write_disk_id = 0;

    for (int i = 0; i < REP_NUM; i++)
    {
      // i = 0,第一个副本，存在磁盘本体 是 1，2，3，4，5
      // i = 1,第二个副本，存在磁盘副本 是 6，7，8，9，10
      if (i == 1)
      {
        continue;
      }

      bool is_last_rep = false; // 判断需不需要写入后三分之一

      if (i == 2) // 对于最后一个副本的情况
      {
        is_last_rep = true;
        disk_id = disk_id + 6;
        disk_id = disk_id == 11 ? 6 : disk_id;
        if (rand() % 2)
        {
          disk_id -= 5;
        }
      }

      int temp_cnt = 0;
      std::vector<int> tmp = disk[disk_id].write_first(size, obj_id, tag, tag_skew, is_last_rep);

      while (tmp.empty()) // 在写入磁盘副本的时候不可能为空，因为这个时候的disk_id是本体已经写入的id，磁盘副本一定可以写入
      {

        if (!is_last_rep)
        {
          disk_id = disk_id % 5 + 1;
        }
        else
        {
          disk_id = disk_id % this->disk_num + 1;
        }
        if (is_last_rep && (disk_id == the_first_write_disk_id + 5 || disk_id == the_first_write_disk_id))
        {
          disk_id = disk_id % this->disk_num + 1;
        }
        temp_cnt++;
        if (temp_cnt == 5)
        {
          tag_skew++;
          temp_cnt = 0;
        }

        tmp = disk[disk_id].write_first(size, obj_id, tag, tag_skew, is_last_rep);
        if (tmp.empty())
        {
          tmp = disk[disk_id].write_first(size, obj_id, tag, 0 - tag_skew, is_last_rep);
        }

      }

      // 现在temp里面存放的是 副本存的cell序列

      if (!i) // 现在是第一个副本，我可以直接写入第二个副本
      {
        for (int j = 0; j < tmp.size(); j++)
        {
          disk[disk_id + 5].cells[tmp[j]].obj_id = obj_id;
          disk[disk_id + 5].cells[tmp[j]].block_id = j;
        }
        objects[obj_id].replica[1] = disk_id + 5;
        objects[obj_id].unit[1] = tmp;
      }

      objects[obj_id].replica[i] = disk_id;
      objects[obj_id].unit[i] = tmp;
      the_first_write_disk_id = i == 0 ? disk_id : the_first_write_disk_id;
    }
    this->tag_write_disk_id[tag] = this->tag_write_disk_id[tag] % 5 + 1;
  }
}

bool Manager::req_need_busy(int obj_id)
{

  // if (objects[obj_id].unit[0][0] > this->cell_per_disk * 82 / 100 && objects[obj_id].tag == 0)
  if (objects[obj_id].unit[0][0] > this->cell_per_disk * 82 / 100)
  {
    return true;
  }

  return false;
}

void Manager::build_request(const std::vector<std::tuple<int, int>> &batch)
{
  busy_req_list.clear();
  for (auto [req_id, obj_id] : batch)
  {
    // 构建新请求
    Request new_req;
    new_req.object_id = obj_id;

    if (IS_FIRST)
      objects[obj_id].interview_timestamp.push_back(TIMESTAMP);

    if (req_need_busy(obj_id))
    {
      busy_req_list.push_back(req_id);
      continue;
    }

    new_req.create_timestamp = TIMESTAMP;

    this->request[req_id] = new_req;

    objects[obj_id].req_id_list.push_back(req_id);

    for (int i = 0; i < objects[obj_id].size; i++)
    {
      objects[obj_id].block_req_num[i]++;
    }
  }
}

// 批量读
std::pair<std::vector<std::string>, std::vector<int>> Manager::read_batch()
{

  std::vector<std::string> operations;
  std::vector<int> single_fin_list;
  std::vector<int> finish_list;
  for (int i = 1; i <= this->disk_num; i++)
  {

    this->disk[i].refresh();

    for (int j = 1; j <= 2; j++)
    {
      single_fin_list = check_finish(this->disk[i].work(j));

      this->disk[i].update_reserve_end(j); // 更新一下point预定位置的结尾

      finish_list.insert(finish_list.end(), single_fin_list.begin(), single_fin_list.end());

      operations.push_back(this->disk[i].point[j].actions);
    }
  }
  fin_num_last_period += finish_list.size();
  return {operations, finish_list};
}

// 批量删
std::vector<int> Manager::delete_batch(const std::vector<int> &batch)
{
  std::vector<int> abort_req;
  for (int b : batch)
  {
    auto &obj = objects[b];

    while (!obj.req_id_list.empty())
    {
      abort_req.push_back(obj.req_id_list.front());
      request.erase(obj.req_id_list.front());
      obj.req_id_list.pop_front();
    }

    // 清除磁盘空间
    int i = 0;
    for (int disk_id : obj.replica)
    {
      for (int cell_id : obj.unit[i])
      {
        this->disk[disk_id].cells[cell_id].obj_id = 0;
        this->disk[disk_id].cells[cell_id].block_id = 0;
      }
      i++;
    }
    objects[b].delete_timestamp = TIMESTAMP;
  }

  return abort_req;
}

std::vector<int> Manager::check_finish(std::vector<std::pair<int, int>> read_list)
{
  // pair的第一个值是对象号，第二个值是块号
  std::vector<int> finish_list;
  for (const auto &[obj_id, block_id] : read_list)
  {
    if (obj_id == 0)
    {
      continue;
    }
    objects[obj_id].block_req_num[block_id] = 0; // 该块已经被访问过了
    int not_fin_req_num = objects[obj_id].not_fin_req_num();
    while (objects[obj_id].req_id_list.size() > not_fin_req_num)
    {
      finish_list.push_back(objects[obj_id].req_id_list.front());
      objects[obj_id].req_id_list.pop_front();
    }
  }

  for (int i = 0; i < finish_list.size(); i++)
  {
    //    // LOG_INFO("REQUEST FINISH %d",TIMESTAMP-request[finish_list[i]].create_timestamp);
    int time = TIMESTAMP - request[finish_list[i]].create_timestamp;

    if (time <= 10)
    {
      SCORE += (1 - 0.005 * time) * (objects[request[finish_list[i]].object_id].size + 1) * 0.5;
      READ_SCORE += (1 - 0.005 * time) * (objects[request[finish_list[i]].object_id].size + 1) * 0.5;
      fin_num++;
    }
    else
    {
      SCORE += (1.05 - 0.01 * time) * (objects[request[finish_list[i]].object_id].size + 1) * 0.5;
      READ_SCORE += (1.05 - 0.01 * time) * (objects[request[finish_list[i]].object_id].size + 1) * 0.5;
      fin_num++;
    }

    request.erase(finish_list[i]);
  }

  return finish_list;
}

std::vector<int> Manager::busy_req()
{

  static std::set<int> may_busy_req;

  int temp = busy_req_list.size();

  if (TIMESTAMP % 30 == 0)
  {
    may_busy_req.clear();
    for (auto &[req_id, req] : request)
    {
      if (TIMESTAMP - req.create_timestamp >= 75)
      {
        may_busy_req.insert(req_id);
      }
    }
  }

  for (auto it = may_busy_req.begin(); it != may_busy_req.end();)
  {
    const auto req_id = *it;
    if (request.count(req_id) == 0)
    {
      it = may_busy_req.erase(it);
    }
    else if (TIMESTAMP - request[req_id].create_timestamp == 105)
    {
      busy_req_list.push_back(req_id);

      int obj_id = request[req_id].object_id;

      SCORE -= (objects[obj_id].size + 1) * 0.5;

      BUSY_SCORE += (objects[obj_id].size + 1) * 0.5;

      busy_num++;
      // LOG_INFO("TIMESTAMP %d TAG %d BUSY_REQ",TIMESTAMP,objects[obj_id].tag);

      objects[obj_id].req_id_list.pop_front();
      objects[obj_id].update_block_req_num();
      request.erase(req_id);

      it = may_busy_req.erase(it);
    }
    else
    {
      ++it;
    }
  }

  busy_num_last_period += busy_req_list.size();
  // LOG_INFO("TIMESTAMP %d BUSY REQ %d", TIMESTAMP, static_cast<int>(busy_req_list.size()) - temp);

  return busy_req_list;
}

std::pair<std::vector<int>, std::vector<std::pair<int, int>>> Manager::exchange_cell()
{

  std::vector<std::pair<int, int>> ops;

  std::vector<int> ops_size;

  // 处理前五个磁盘，然后接收返回的结果，通过结果更改后面五个磁盘
  for (int i = 1; i <= this->disk_num / 2; i++)
  {
    // this->disk[i].exchange_time = init_exchange_time;

    // std::vector<std::pair<int, int>> tmp = this->disk[i].per_disk_exchange_cell(tag_list);

    // this->disk[i].mirror_exchange_cell(tmp);

    // this->disk[i + 5].mirror_exchange_cell(tmp);

    // std::vector<std::pair<int, int>> tmp2 = this->disk[i].per_disk_exchange_cell2(tag_list);

    // this->disk[i].mirror_exchange_cell(tmp2);

    // this->disk[i + 5].mirror_exchange_cell(tmp2);

    // ops.insert(ops.end(), tmp.begin(), tmp.end());

    // ops.insert(ops.end(), tmp2.begin(), tmp2.end());

    // ops_size.push_back(tmp.size() + tmp2.size());

    ops_size.push_back(0);
  }

  ops_size.insert(ops_size.end(), ops_size.begin(), ops_size.end());
  ops.insert(ops.end(), ops.begin(), ops.end());

  return {ops_size, ops};
}

void Manager::clear()
{
  SCORE = 0;
  BUSY_SCORE = 0;
  READ_SCORE = 0;
  busy_num = 0;
  fin_num = 0;

  request.clear();
  this->fin_num_last_period = 0;
  tag_write_disk_id.clear();
  this->tag_write_disk_id.assign(tag_num + 1, 1);

  disk.clear();
  for (int i = 0; i <= disk_num; i++)
  {
    disk.push_back(Disk(i, cell_per_disk, init_token, &objects, &request));
  }

  for (int i = 1; i <= disk_num; i++)
  {
    int mirror_disk = i > 5 ? i - 5 : i + 5;
    disk[i].mirror_disk_id = mirror_disk;
    disk[i].mirror_point.push_back(&disk[mirror_disk].point[0]);
    disk[i].mirror_point.push_back(&disk[mirror_disk].point[1]);
    disk[i].mirror_point.push_back(&disk[mirror_disk].point[2]);
  }

  
}
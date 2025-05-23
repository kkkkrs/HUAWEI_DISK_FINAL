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

Manager::Manager(int disk_num, int cell_per_disk, int init_token, int tag_num, int exchange_time1, int exchange_time2, int period_num)
    : disk_num(disk_num), cell_per_disk(cell_per_disk), tag_num(tag_num), period_num(period_num), init_exchange_time1(exchange_time1), init_exchange_time2(exchange_time2), init_token(init_token)
{

  this->fin_num_last_period = 0;

  this->req_num = 0;

  this->tag_write_disk_id.assign(tag_num + 1, 1);

  for (int i = 0; i <= disk_num; i++)
  {
    disk.push_back(Disk(i, cell_per_disk, init_token, &objects, &request));
  }
}

void Manager::Statistics()
{
  if (IS_FIRST)
  {
    //    LOG_INFO("ROUND 1");
    //    LOG_INFO("GLOBAL: MAX_JUMP_TIME_BEFORE_PRE=%d", MAX_JUMP_TIME_BEFORE_PRE);
    //    LOG_INFO("GLOBAL: POINTER_RESTRICTION_RANGE=%d", POINTER_RESTRICTION_RANGE);
    //    LOG_INFO("GLOBAL: WINDOW_LEN=%d", WINDOW_LEN);
    //    LOG_INFO("GLOBAL: slice_len=%d", slice_len);

    // 第一轮专有参数
    //    LOG_INFO("ROUND1: LEAST_READ_NUM_FIRST=%d", LEAST_READ_NUM);
    //    LOG_INFO("ROUND1: FIRST_Turn_down=%d", FIRST_Turn_down);
    //    LOG_INFO("ROUND1: FIRST_TAG_AREA=%.2f", FIRST_TAG_AREA);
    //    LOG_INFO("ROUND1: jump_req_num_threshold_first=%.1f", jump_req_num_threshold_first);
    //    LOG_INFO("ROUND1: shield_request_time_first=%d", shield_request_time_first);
    //    LOG_INFO("ROUND1: forecast_window_len=%d", forecast_window_len);
  }
  else
  {
    //    LOG_INFO("ROUND 2");
    //    LOG_INFO("ROUND2: LEAST_READ_NUM_SECOND=%d", LEAST_READ_NUM);
    //    LOG_INFO("ROUND2: jump_req_num_threshold_second=%.1f", jump_req_num_threshold_second);
    //    LOG_INFO("ROUND2: shield_request_time_second=%d", shield_request_time_second);
  }
  //  LOG_INFO("READ_NUM %d READ_SCORE %.2f", fin_num, READ_SCORE);
  //  LOG_INFO("BUSY_NUM %d BUSY_SCORE %.2f", busy_num, BUSY_SCORE);
  //  LOG_INFO("SCORE %.2f", SCORE);
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

    int disk_id = this->tag_write_disk_id[tag];

    objects[obj_id].size = size;
    objects[obj_id].tag = tag;
    objects[obj_id].block_req_num.assign(size, 0);
    objects[obj_id].create_timestamp = TIMESTAMP;
    if(tag==0){
      objects[obj_id].is_tag_0 = true;
    }else{
      objects[obj_id].is_tag_0 = false;
    }

    int tag_skew = 0;

    int the_first_write_disk_id = 0;

    std::vector<int> rep_here;

    for (int i = 0; i < REP_NUM; i++)
    {

      bool is_last_rep = false; // 判断需不需要写入后三分之二

      if (i != 0) // 对于最后两个副本的情况
      {
        is_last_rep = true;
      }

      while (find(rep_here.begin(), rep_here.end(), disk_id) != rep_here.end())
      {
        disk_id = disk_id % this->disk_num + 1;
      }

      int temp_cnt = 0;

      std::vector<int> tmp = disk[disk_id].write_first(size, obj_id, tag, tag_skew, is_last_rep);

      while (tmp.empty()) // 在写入磁盘副本的时候不可能为空，因为这个时候的disk_id是本体已经写入的id，磁盘副本一定可以写入
      {
        disk_id = disk_id % this->disk_num + 1;

        while (find(rep_here.begin(), rep_here.end(), disk_id) != rep_here.end())
        {
          disk_id = disk_id % this->disk_num + 1;
        }

        temp_cnt++;

        if (temp_cnt == 10)
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
      objects[obj_id].replica[i] = disk_id;
      objects[obj_id].unit[i] = tmp;
      rep_here.push_back(disk_id);
    }
    this->tag_write_disk_id[tag] = this->tag_write_disk_id[tag] % this->disk_num + 1;
  }
}

void Manager::update_tag_rank()
{

  DISK_START.clear();
  TAG_RANK.clear();
  DISK_START.resize(this->tag_num + 2, 0);

  if (IS_FIRST)
  {
    TAG_RANK = {16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    DISK_START[0] = 1;
    for (int i = 0; i < 16; i++)
    {
      DISK_START[i + 1] = this->cell_per_disk * FIRST_TAG_AREA / 16 + DISK_START[i];
    }
  }
  else
  {

    // 根据分段来对记录每一个时期该tag的数量、读取量、删除量、写入量
    // 对tag进行分区
    // 对tag进行排序
    // 分配每一个tag的start

    int slice_num = MAX_TIME_SLICING / slice_len + 1; // 145

    tag_obj_cnt_per_slice.assign(tag_num + 1, std::vector<int>(slice_num + 1, 0));
    tag_obj_read_per_slice.assign(tag_num + 1, std::vector<int>(slice_num + 1, 0));
    std::vector<int> tag_obj_max_cnt(tag_num + 1, 0);

    for (auto [obj_id, obj] : objects)
    {
      int begin_slice = obj.create_timestamp / slice_len;
      int end_slice = obj.delete_timestamp / slice_len;
      while (begin_slice != end_slice + 1)
      {
        tag_obj_cnt_per_slice[obj.tag][begin_slice] += obj.size;
        tag_obj_max_cnt[obj.tag] = std::max(tag_obj_max_cnt[obj.tag], tag_obj_cnt_per_slice[obj.tag][begin_slice]);
        begin_slice++;
      }
      for (auto read_ts : obj.interview_timestamp)
      {
        int read_slice = read_ts / slice_len;
        tag_obj_read_per_slice[obj.tag][read_slice] += obj.size;
      }
    }

    tag_read_per_cell.assign(slice_num, std::vector<double>(this->tag_num + 1, 0));

    for (int i = 0; i < slice_num; i++)
    {
      for (int j = 1; j <= tag_num; j++)
      {
        tag_read_per_cell[i][j] = tag_obj_read_per_slice[j][i] * 1.0 / tag_obj_cnt_per_slice[j][i];
      }
    }

    std::vector<int> Disk_partition_per_tag(this->tag_num + 1);

    TAG_RANK.resize(this->tag_num + 1);

    DISK_START[0] = 1;

    int sum_object = 0;

    for (int i = 1; i <= this->tag_num; i++)
    {
      sum_object += tag_obj_max_cnt[i];
    }

    for (int i = 1; i <= this->tag_num; i++)
    {
      Disk_partition_per_tag[i] = tag_obj_max_cnt[i] * 1.0 / sum_object * this->disk[1].cell_num;
    }

    LinearTagScheduler scheduler(this->tag_num, slice_num - 1);

    scheduler.load_data(tag_read_per_cell);

    // 获取最优序列
    auto sequence = scheduler.get_optimal_sequence();

    sequence.push_back(0);

    for (int i = 0; i < sequence.size(); i++)
    {
      TAG_RANK[sequence[i]] = i;
      DISK_START[i + 1] = DISK_START[i] + Disk_partition_per_tag[sequence[i]];
    }

    DISK_START[16] = this->disk[1].cell_num;
  }
}

void Manager::update_tag_list()
{
  tag_list.clear();
  tag_list.reserve(tag_num); // 预分配内存提高效率

  for (int i = 1; i <= tag_num; ++i)
  { // 建议使用前缀自增
    tag_list.push_back(i);
  }

  if (IS_FIRST)
  {
    // 根据目前所有的区域前面900个时间片的读取量除以区域中cell的数量来对区域进行排序
    std::vector<int> tag_cell_num(this->tag_num + 1, 0);
    std::vector<int> tag_read(this->tag_num + 1, 0);

    for (auto [obj_id, obj] : objects)
    {
      if (obj.tag == 0)
      {
        obj.last_slice_interview_times = 0;
        continue;
      }
      tag_cell_num[obj.tag] += obj.size;
      tag_read[obj.tag] += obj.last_slice_interview_times;
      obj.last_slice_interview_times = 0;
    }
    // 修正后的lambda表达式
    std::sort(tag_list.begin(), tag_list.end(),
              [tag_cell_num, tag_read](int a, int b)
              {
                return tag_read[a] * 1.0 / tag_cell_num[a] > tag_read[b] * 1.0 / tag_cell_num[b];
                // return tag_read[a] > tag_read[b];
              });
  }
  else
  {
    // 修正后的lambda表达式
    std::sort(tag_list.begin(), tag_list.end(),
              [this](int a, int b)
              {
                return tag_read_per_cell[SLICE][a] > tag_read_per_cell[SLICE][b];
              });
  }
}

void Manager::update_busy_area()
{
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

    std::vector<int> rep_here;

    for (int i = 0; i < REP_NUM; i++)
    {

      bool is_last_rep = false; // 判断需不需要写入后三分之一

      if (i != 0)
      {
        is_last_rep = true;
      }

      while (find(rep_here.begin(), rep_here.end(), disk_id) != rep_here.end())
      {
        disk_id = disk_id % this->disk_num + 1;
      }

      int temp_cnt = 0;
      std::vector<int> tmp = disk[disk_id].write_first(size, obj_id, tag, tag_skew, is_last_rep);

      while (tmp.empty()) // 在写入磁盘副本的时候不可能为空，因为这个时候的disk_id是本体已经写入的id，磁盘副本一定可以写入
      {
        disk_id = disk_id % this->disk_num + 1;

        while (find(rep_here.begin(), rep_here.end(), disk_id) != rep_here.end())
        {
          disk_id = disk_id % this->disk_num + 1;
        }

        temp_cnt++;

        if (temp_cnt == 10)
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

      objects[obj_id].replica[i] = disk_id;
      objects[obj_id].unit[i] = tmp;
      rep_here.push_back(disk_id);
    }
    this->tag_write_disk_id[tag] = this->tag_write_disk_id[tag] % this->disk_num + 1;
  }
}

bool Manager::req_need_busy(int obj_id)
{

  // if (objects[obj_id].unit[0][0] > this->cell_per_disk * 82 / 100 && objects[obj_id].tag == 0)
  // if (IS_FIRST && objects[obj_id].unit[0][0] > this->cell_per_disk * 30 / 100 && objects[obj_id].unit[0][0] < this->cell_per_disk * 55 / 100)
  if (IS_FIRST && TIMESTAMP > 18000)
  {
    for (auto cell_id : objects[obj_id].unit[0])
    {
      if (cell_id > this->cell_per_disk * FIRST_Turn_down / 100)
      {
        return true;
      }
    }
  }

  if (IS_FIRST && !run_first)
  {
    return true;
  }

  if (!IS_FIRST && !run_second)
  {
    return true;
  }

  if (find(busy_area.begin(), busy_area.end(), objects[obj_id].write_area) != busy_area.end())
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

    objects[obj_id].last_slice_interview_times++;

    if (req_need_busy(obj_id))
    {
      if (IS_FIRST)
      {
        busy_req_list.push_back(req_id);
      }
      else
      {
        drop_req_list.push_back({req_id, TIMESTAMP});
      }
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
    //    //    //    // LOG_INFO("REQUEST FINISH %d",TIMESTAMP-request[finish_list[i]].create_timestamp);
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

  if (TIMESTAMP % 100 == 0)
  {
    may_busy_req.clear();
    for (auto &[req_id, req] : request)
    {
      if (TIMESTAMP - req.create_timestamp >= 5)
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
      //      //      // LOG_INFO("TIMESTAMP %d TAG %d BUSY_REQ",TIMESTAMP,objects[obj_id].tag);

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

  for (auto [req_id, create_ts] : drop_req_list)
  {
    if (TIMESTAMP - create_ts == 105)
    {
      busy_req_list.push_back(req_id);
    }
  }
  busy_num_last_period += busy_req_list.size();
  //  //  // LOG_INFO("TIMESTAMP %d BUSY REQ %d", TIMESTAMP, static_cast<int>(busy_req_list.size()) - temp);

  return busy_req_list;
}

std::pair<std::vector<int>, std::vector<std::pair<int, int>>> Manager::exchange_cell()
{

  std::vector<std::pair<int, int>> ops;

  std::vector<int> ops_size;

  for (int i = 1; i <= this->disk_num; i++)
  {

    if (IS_FIRST)
    {
      this->disk[i].exchange_time = init_exchange_time1;
    }
    else
    {
      this->disk[i].exchange_time = init_exchange_time2;
    }

    std::vector<std::pair<int, int>> tmp = this->disk[i].per_disk_exchange_cell(tag_list);

    this->disk[i].mirror_exchange_cell(tmp);

    ops.insert(ops.end(), tmp.begin(), tmp.end());

    std::vector<std::pair<int, int>> tmp3 = this->disk[i].per_disk_exchange_cell(tag_list);

    this->disk[i].mirror_exchange_cell(tmp3);

    ops.insert(ops.end(), tmp3.begin(), tmp3.end());

    std::vector<std::pair<int, int>> tmp2 = this->disk[i].per_disk_exchange_cell2(tag_list);

    this->disk[i].mirror_exchange_cell(tmp2);

    ops.insert(ops.end(), tmp2.begin(), tmp2.end());

    ops_size.push_back(tmp.size() + tmp3.size() + tmp2.size());
  }

  return {ops_size, ops};
}

void Manager::clear()
{
  busy_area.clear();
  SCORE = 0;
  BUSY_SCORE = 0;
  READ_SCORE = 0;
  busy_num = 0;
  fin_num = 0;
  SLICE = 0;

  request.clear();
  this->fin_num_last_period = 0;
  tag_write_disk_id.clear();
  this->tag_write_disk_id.assign(tag_num + 1, 1);

  disk.clear();
  for (int i = 0; i <= disk_num; i++)
  {
    disk.push_back(Disk(i, cell_per_disk, init_token, &objects, &request));
  }
}

void Manager::cal_obj_tag()
{

  int window_num = MAX_TIME_SLICING / forecast_window_len + 1;

  std::vector<std::vector<double>> tag_trait(tag_num + 1, std::vector<double>(window_num, 0));

  std::vector<std::vector<double>> tag_obj_num(tag_num + 1, std::vector<double>(window_num, 0));

  for (auto [obj_id, obj] : objects)
  {
    if (obj.tag == 0)
    {
      continue;
    }

    int begin_wind = obj.create_timestamp / forecast_window_len;
    int end_wind = obj.delete_timestamp / forecast_window_len;

    while (begin_wind != end_wind + 1)
    {
      tag_obj_num[obj.tag][begin_wind]++;
      begin_wind++;
    }

    for (auto interview_ts : obj.interview_timestamp)
    {
      int window_index = interview_ts / forecast_window_len;
      tag_trait[obj.tag][window_index]++;
    }
  }

  for (int i = 1; i <= this->tag_num; i++)
  {
    for (int j = 0; j < window_num; j++)
    {
      tag_trait[i][j] /= tag_obj_num[i][j];
    }
  }
  // 遍历所有tag0，在有统计的时间段内与已知信息进行比较，选择差异最小的

  for (auto &[obj_id, obj] : objects)
  {
    // if (obj.tag != 0)
    // {
    //   continue;
    // }
    // pair<window_index , read_times>
    std::vector<double> read_per_obj(window_num, 0);
    std::vector<double> distance(tag_num + 1, 0);

    // 在有统计的window，计算与每一个tag的欧氏距离（绝对值？），然后选择最小的

    for (auto interview_ts : obj.interview_timestamp)
    {
      int window_index = interview_ts / forecast_window_len;
      // read_per_obj[window_index]+=obj.size;
      read_per_obj[window_index]++;
    }

    int begin_wind = obj.create_timestamp / forecast_window_len;

    int end_wind = obj.delete_timestamp / forecast_window_len;

    for (int window_index = 0; window_index < window_num; window_index++)
    {
      if (window_index <= begin_wind || window_index >= end_wind)
      {
        continue;
      }
      for (int tag = 1; tag <= tag_num; tag++)
      {
        distance[tag] += (read_per_obj[window_index] - tag_trait[tag][window_index]) * (read_per_obj[window_index] - tag_trait[tag][window_index]);
        // distance[tag]+=abs(read_per_obj[window_index]-tag_trait[tag][window_index]);
      }
    }
    int min_tag = 0;
    double Min = 100000;

    for (int tag = 1; tag <= tag_num; tag++)
    {
      if (distance[tag] < Min)
      {
        Min = distance[tag];
        min_tag = tag;
      }
    }
    obj.tag = min_tag;

    //    //    // LOG_INFO("%d %d",obj_id,obj.tag);
  }
}

void Manager::forecast_tag()
{
  // 需要开一个数组记录每个tag在每300个时间片的read特征
  // 对于每一个tag0，如果有数据的window数量大于一个值，那么我们就可以对其进行tag分配
  // 将它的tag赋为 -预测tag，可以在垃圾回收的时候移动到自己的tag区域
  // 报busy的时候不要根据obj的写入区域
  // 在确定一个区域不要时，对对应的obj进行标记

  int window_num = TIMESTAMP / forecast_window_len + 1;

  std::vector<std::vector<double>> tag_trait(tag_num + 1, std::vector<double>(window_num, 0));

  std::vector<std::vector<double>> tag_obj_num(tag_num + 1, std::vector<double>(window_num, 0));

  for (auto [obj_id, obj] : objects)
  {
    if (obj.tag <= 0)
    {
      continue;
    }

    int begin_wind = obj.create_timestamp / forecast_window_len;
    int end_wind = std::min(obj.delete_timestamp / forecast_window_len, TIMESTAMP / forecast_window_len);

    while (begin_wind != end_wind + 1)
    {
      tag_obj_num[obj.tag][begin_wind]++;
      begin_wind++;
    }

    for (auto interview_ts : obj.interview_timestamp)
    {
      int window_index = interview_ts / forecast_window_len;
      tag_trait[obj.tag][window_index]++;
    }
  }

  for (int i = 1; i <= this->tag_num; i++)
  {
    for (int j = 0; j < window_num; j++)
    {
      tag_trait[i][j] /= tag_obj_num[i][j];
    }
  }
  // 遍历所有tag0，在有统计的时间段内与已知信息进行比较，选择差异最小的

  for (auto &[obj_id, obj] : objects)
  {
    if(!obj.is_tag_0){
      continue;
    }
    // if (obj.tag != 0)
    // {
    //   continue;
    // }
    // pair<window_index , read_times>
    std::vector<double> read_per_obj(window_num, 0);
    std::vector<double> distance(tag_num + 1, 0);

    std::set<int> read_win;

    for (auto interview_ts : obj.interview_timestamp)
    {
      int window_index = interview_ts / forecast_window_len;
      read_per_obj[window_index]++;
      read_win.insert(window_index);
    }

    if (read_win.size() < LEAST_READ_WIND_NUM)
    {
      continue;
    }

    int begin_wind = obj.create_timestamp / forecast_window_len;

    int end_wind = std::min(obj.delete_timestamp / forecast_window_len, TIMESTAMP / forecast_window_len);

    for (int window_index = 0; window_index < window_num; window_index++)
    {
      if (window_index <= begin_wind || window_index >= end_wind)
      {
        continue;
      }
      for (int tag = 1; tag <= tag_num; tag++)
      {
        distance[tag] += (read_per_obj[window_index] - tag_trait[tag][window_index]) * (read_per_obj[window_index] - tag_trait[tag][window_index]);
      }
    }
    int min_tag = 0;
    double Min = 100000;

    for (int tag = 1; tag <= tag_num; tag++)
    {
      if (distance[tag] < Min)
      {
        Min = distance[tag];
        min_tag = tag;
      }
    }
    obj.tag = min_tag;

    //    //    // LOG_INFO("%d %d",obj_id, obj.tag);
  }
}
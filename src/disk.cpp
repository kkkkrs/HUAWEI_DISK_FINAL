#include "disk.h"
#include "constants.h"
//#include "logger.h"
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>
#include <string>
#include <tuple>
#include <algorithm>

Point::Point(int disk_id, int point_id)
{
  this->disk_id = disk_id;
  this->point_id = point_id;
  this->actions = "";
  this->since_last_jump = MAX_JUMP_TIME_BEFORE_PRE;
  this->pre_token = 1;
  this->position = 1;
  this->jump_here = 0;
  this->reserve_end = -1;
};

int Disk::find_min_area()
{
  int max_blank_space = 0;
  int max_rank = 0;
  for (int i = 0; i <= 15; i++)
  {
    int cur_blank_space = 0;
    int start = DISK_START[i];
    int end = DISK_START[i + 1];
    while (start != end)
    {
      if (cells[start].obj_id == 0)
      {
        cur_blank_space++;
      }
      start++;
    }
    if (cur_blank_space > max_blank_space)
    {
      max_blank_space = cur_blank_space;
      max_rank = i;
    }
  }
  return max_rank;
}

std::vector<int> Disk::write_first(int size, int obj_id, int tag, int tag_skew, bool is_last_rep)
{

  int t_rank = TAG_RANK[tag] + tag_skew;

  int start, end;
  bool is_f_to_b = tag_skew >= 0;
  bool is_restrict = true;

  if (t_rank > 15 || t_rank < 0)
  {
    if (TAG_RANK[tag] + tag_skew > 15 && TAG_RANK[tag] - tag_skew < 0)
    {
      tag = 0;
    }
    else
    {
      return std::vector<int>();
    }
  }
  else
  {
    start = DISK_START[t_rank];   // 开始找寻的位置
    end = DISK_START[t_rank + 1]; // 结束找寻的位置
  }

  if (tag == 0 && !is_last_rep)
  {
    t_rank = 16;
    is_f_to_b = true;
    end = this->cell_num + 1;
    if (tag_skew > 5)
    {
      start = 1;
      end = this->cell_num + 1;
    }
  }

//  // LOG_INFO("TAG %d START %d END %d",tag,start,end);

  if (is_last_rep)
  {
    start = this->cell_num + 1;
    end = REAL_CELL_NUM;
    is_restrict = false;
    is_f_to_b = true;
  }

  // 寻找一段足够连续的区域
  int count = 0;
  int temp_count = 0;
  int window_begin = start;
  int window_end = start;
  while (window_end != end)
  {

    if (this->cells[window_end].obj_id == 0)
    {
      count++;
      temp_count++;
    }
    if (count == size)
    {
      break;
    }
    if (window_end > start + WINDOW_LEN)
    {
      if (this->cells[window_begin].obj_id == 0)
      {
        count--;
      }
      move_point(window_begin, is_f_to_b, is_restrict);
    }
    move_point(window_end, is_f_to_b, is_restrict);
  }

  if (count >= size)
  {
    start = window_begin;
  }
  else if (temp_count >= size)
  {
  }
  else
  {
    return std::vector<int>();
  }

  count = 0;
  std::vector<int> wrote_cell_id(size);
  while (count < size)
  {
    if (this->cells[start].obj_id == 0)
    {
      this->cells[start].obj_id = obj_id;
      this->cells[start].block_id = count;
      wrote_cell_id[count] = start;
      count++;
    }
    move_point(start, is_f_to_b, is_restrict);
  }

  if (!is_last_rep)
  {
    objects->at(obj_id).write_area = t_rank;
//    // LOG_INFO("TAG %d ,T_RANK %d", tag, t_rank);
  }

  return wrote_cell_id;
}

Disk::Disk(int disk_id, int cell_num, int init_token, std::unordered_map<int, Object> *objects, std::unordered_map<int, Request> *request)
{
  this->disk_id = disk_id;
  this->cell_num = cell_num;
  this->init_token = init_token;
  this->cells.resize(REAL_CELL_NUM + 1, Cell(request, objects));
  this->objects = objects;
  this->request = request;

  this->point.push_back(Point(0, 0)); // 这个磁头不会使用
  this->point.push_back(Point(disk_id, 1));
  this->point.push_back(Point(disk_id, 2));
}

void Disk::refresh()
{

  this->point[1].actions = "";
  this->point[1].token = init_token;
  this->point[2].actions = "";
  this->point[2].token = init_token;
}

std::vector<std::pair<int, int>> Disk::work(int point_id)
{
  std::vector<std::pair<int, int>> read_list;

  std::string original_ops = get_ori_ops(point_id);

  if (original_ops[0] != 'r' && original_ops[0] != 'p')
  {
    this->point[point_id].actions = "j " + original_ops;
    this->point[point_id].pre_token = 1;
    this->point[point_id].since_last_jump = 0;
    this->point[point_id].position = std::stoi(original_ops);
    return read_list;
  }

  std::string replace_ops = find_replacement_indices(original_ops, point[point_id].pre_token);

  int s_index = 0;

  while (point[point_id].token > 0 && s_index < replace_ops.size())
  {
    bool is_read = replace_ops[s_index] == 'r';
    auto tmp = prosess_single_cell(point_id, is_read);

    if (tmp.first != 0)
    {
      read_list.push_back(tmp);
    }

    s_index++;
  }

  if (point[point_id].token < 0)
  {
    s_index--;
  }

  replace_ops.erase(s_index).push_back('#');

  point[point_id].actions = replace_ops; // 将磁盘动作添加进去

  return read_list;
}

std::pair<int, int> Disk::find_jump_point(int point_id, bool is_shield_request)
{

  int begin = 1;
  int end = 1;
  int cur_req_num = 0;
  int max_req_num = 0;
  // int max_real_req_num = 0;
  // int cur_real_req_num = 0;
  int max_position = 1;
  std::queue<int> que;

  for (int i = 0; i < this->init_token; i++)
  {
    int cell_num = cell_need_read(end, point_id, true, is_shield_request);
    cur_req_num += cell_num;
    que.push(cell_num);
    end++;
    // cur_real_req_num += cell_need_read(end, point_id, true, false);
  }

  while (end != this->cell_num + 1)
  {
    int cell_num = cell_need_read(end, point_id, true, is_shield_request);
    cur_req_num += cell_num;
    que.push(cell_num);
    cur_req_num -= que.front();
    que.pop();
    end++;
    begin++;
    if (cur_req_num >= max_req_num)
    {
      max_req_num = cur_req_num;
      max_position = begin;
    }
  }

  if (max_req_num == 0 && is_shield_request)
  {
    return find_jump_point(point_id, false);
  }

  // int tag = (*objects)[cells[point[point_id].position].obj_id].tag;

  while (max_position > 0 && cell_need_read(max_position, point_id, true, false))
  {
    max_position--;
  }

  max_position++;

  return {max_req_num, max_position};
}

std::pair<int, int> Disk::prosess_single_cell(int point_id, bool is_read)
{
  std::pair<int, int> read_cell = {0, 0};

  if (!is_read)
  {
    this->point[point_id].token -= 1;
    this->point[point_id].pre_token = 1;
    this->point[point_id].position++;
    return read_cell;
  }

  int consum_token = calculate_consum_token(point_id);
  this->point[point_id].token -= consum_token;
//  //  ////  // LOG_INFO("时间戳%d 磁盘%d 剩余token %d",TIMESTAMP
  ///,this->disk_id,this->token);
  if (point[point_id].token < 0)
  {
    return read_cell;
  }
  // token剩余数量足够，执行读操作
  point[point_id].pre_token = consum_token; // 更新pre_token

  int obj_id = this->cells[this->point[point_id].position].obj_id;

  int block_id = this->cells[this->point[point_id].position].block_id;

  this->point[point_id].position++;

  read_cell.first = obj_id;

  read_cell.second = block_id;

  return read_cell;
}

int Disk::calculate_consum_token(int point_id)
{
  if (this->point[point_id].pre_token == 1)
  {
    return 64;
  }
  else
  {
    return std::max(16, static_cast<int>(ceil(point[point_id].pre_token * 0.8)));
  }
}

std::string Disk::find_replacement_indices(const std::string &sequence, int pre_token)
{
  int n = sequence.length();
  // std::vector<std::vector<std::vector<int> > > dp(n ,
  // std::vector<std::vector<int> > (2,std::vector<int>(9,999999999)));
  int dp_r[3001][10];
  int dp_p[3001];
  for (int i = 0; i < n; ++i)
  {
    dp_p[i] = 99999;
    for (int k = 0; k < 10; ++k)
    {
      dp_r[i][k] = 99999; // 初始化为大值
    }
  }
  int consum_que[9] = {1, 64, 52, 42, 34, 28, 23, 19, 16};
  int Min_book[3001] = {0};
  int pre_token_id = 0;

  for (int i = 0; i < 9; i++)
  {
    if (pre_token == consum_que[i])
    {
      pre_token_id = i;
    }
  }

  if (pre_token_id == 8)
  {
    pre_token_id--;
  }
  dp_r[0][pre_token_id + 1] = consum_que[pre_token_id + 1];
  Min_book[0] = pre_token_id + 1;
  if (sequence[0] == 'p')
  {
    dp_p[0] = 1;
    Min_book[0] = 10;
  }

  // dp[i][0][j]表示第i个字符为‘r’时，使用的token数量为consume[j]，最少的消耗token数量
  // dp[i][0][j]应该为dp[i-1][0][j-1]+consume[j]
  // dp[i][0][j]应该为dp[i-1][0][j-1]+consume[j]
  // dp[i][1][0]表示第i个字符为‘p’时，使用的token数量为consume[0]，最少的消耗token数量
  for (int i = 1; i < n; i++)
  { // r为0,p为1
    int Min = 999999;
    for (int j = 1; j < 8; j++)
    {
      if (j == 1)
      {
        dp_r[i][1] = dp_p[i - 1] + 64;
      }
      else
      {
        dp_r[i][j] = dp_r[i - 1][j - 1] + consum_que[j];
      }
      if (Min > dp_r[i][j])
      {
        Min = dp_r[i][j];
        Min_book[i] = j;
      }
    }

    dp_r[i][8] = std::min(dp_r[i - 1][8], dp_r[i - 1][7]) + 16;
    if (Min > dp_r[i][8])
    {
      Min = dp_r[i][8];
      Min_book[i] = 8;
    }
    if (sequence[i] == 'p')
    {
      if (Min_book[i - 1] / 10 == 1)
      {
        dp_p[i] = dp_p[i - 1] + 1;
      }
      else
      {
        dp_p[i] = dp_r[i - 1][Min_book[i - 1] % 10] + 1;
      }

      if (dp_p[i] < Min)
      {
        Min = dp_p[i];
        Min_book[i] = 10;
      }
    }
  }
  std::string ans = "";
  int Min_0 = Min_book[n - 1] / 10;
  int Min_1 = Min_book[n - 1] % 10;
  if (Min_0 == 0)
  {
    ans.push_back('r');
    // ans = 'r' + ans;
  }
  else
  {
    ans.push_back('p');
    // ans = 'p' + ans;
  }
  for (int i = n - 1; i >= 1; i--)
  {
    if (Min_0 == 0)
    {
      if (Min_1 == 1)
      {
        Min_0 = 1;
        Min_1 = 0;
      }
      else
      {
        Min_0 = 0;
        if (Min_1 != 8 || dp_r[i - 1][8] > dp_r[i - 1][7])
          Min_1--;
      }
    }
    else
    {
      Min_0 = Min_book[i - 1] / 10;
      Min_1 = Min_book[i - 1] % 10;
    }
    if (Min_0 == 0)
    {
      // ans = 'r' + ans;
      ans.push_back('r');
    }
    else
    {
      // ans = 'p' + ans;
      ans.push_back('p');
    }
  }
  std::reverse(ans.begin(), ans.end());
  return ans;
}

std::string Disk::get_ori_ops(int point_id)
{
  int temp_point = this->point[point_id].position;
  std::string ori_ops = "";

  if (!run_first && IS_FIRST)
  {
    return "1";
  }

  if (!run_second && !IS_FIRST)
  {
    return "1";
  }

  if (this->point[point_id].jump_here)
  {
    ori_ops = std::to_string(this->point[point_id].jump_here);
    this->point[point_id].jump_here = 0;
    return ori_ops;
  }

  // 如果超出了范围，就直接jump
  if (temp_point >= this->cell_num + 1)
  {
    auto [max_req_num, jump_point] = find_jump_point(point_id, true);
    ori_ops = std::to_string(jump_point);
    return ori_ops;
  }

  int read_num = 0;

  bool not_jump = false;

  for (int i = 0; i < this->init_token; i++)
  {

    if (temp_point == this->point[3 - point_id].position)
    {
      break;
    }

    int cell_read_num = cell_need_read(temp_point, point_id, false, false);
    if (cell_read_num)
    {
      ori_ops.push_back('r');
      read_num += cell_read_num;
    }
    else
    {
      ori_ops.push_back('p');
    }
    temp_point++;

    if (i == 15 && read_num >= 15)
    {
      not_jump = true;
    }
  }

  if (!not_jump && (read_num < 1 || (read_num < LEAST_READ_NUM && this->point[point_id].since_last_jump >= MAX_JUMP_TIME_BEFORE_PRE)))
  {
    auto [max_req_num, jump_point] = find_jump_point(point_id, true);

    if (IS_FIRST && max_req_num >= read_num * jump_req_num_threshold_first)
    {
//      // LOG_INFO("TIMESTAMP %d last %d",TIMESTAMP,this->point[point_id].since_last_jump);
      ori_ops = std::to_string(jump_point);
    }

    if (!IS_FIRST && max_req_num >= read_num * jump_req_num_threshold_second)
    {
//      // LOG_INFO("TIMESTAMP %d last %d",TIMESTAMP,this->point[point_id].since_last_jump);
      ori_ops = std::to_string(jump_point);
    }

    this->point[point_id].since_last_jump = 0;
  }

  this->point[point_id].since_last_jump++;

  if (ori_ops[0] == 'r' || ori_ops[0] == 'p' || ori_ops.size() == 0)
  {
    while (temp_point < this->point[point_id].position + this->init_token)
    {
      int cell_read_num = cell_need_read(temp_point, point_id, false, false);
      if (cell_read_num)
      {
        ori_ops.push_back('r');
      }
      else
      {
        ori_ops.push_back('p');
      }
      temp_point++;
    }
  }

  return ori_ops;
}

void Disk::move_point(int &point, bool is_f_to_b, bool restrict)
{

  if (restrict)
  {

    if (is_f_to_b)
    {
      point = point % this->cell_num + 1;
    }
    else
    {
      point = point - 1;
      point = point == 0 ? this->cell_num : point;
    }
    return;
  }

  if (is_f_to_b)
  {
    point++;
  }
  else
  {
    point--;
  }
}

int Disk::countGreaterThanTimestamp(const std::deque<int> &req_id_list, int timestamp)
{
  auto it = std::upper_bound(req_id_list.begin(), req_id_list.end(), timestamp,
                             [this](int ts, const int req_id)
                             {
                               return ts < (*request)[req_id].create_timestamp;
                             });
  return std::distance(it, req_id_list.end());
}

int Disk::cell_need_read(int cell_id, int point_id, bool is_shield, bool is_shield_request) // 当point为0时会全部屏蔽
{

  if (is_shield || !point_id)
  {

    if (!point_id)
    {
      if (cell_id - point[1].position >= 0 && point[1].reserve_end - cell_id >= 0)
      {
        return 0;
      }
      if (cell_id - point[2].position >= 0 && point[2].reserve_end - cell_id >= 0)
      {
        return 0;
      }
    }
    else
    {
      if (cell_id - point[3 - point_id].position >= 0 && point[3 - point_id].reserve_end - cell_id >= 0)
      {
        return 0;
      }
    }
  }

  int obj_id = this->cells[cell_id].obj_id;

  int block_id = this->cells[cell_id].block_id;

  if (obj_id == 0)
  {
    return 0;
  }

  int shield_request_req_num = 0;

  if (is_shield_request)
  {
    if(IS_FIRST){
      shield_request_req_num = countGreaterThanTimestamp((*objects)[obj_id].req_id_list, TIMESTAMP - shield_request_time_first);
    }else{
      shield_request_req_num = countGreaterThanTimestamp((*objects)[obj_id].req_id_list, TIMESTAMP - shield_request_time_second);
    }
  }

  int req_num = std::max(0, ((*objects)[obj_id].block_req_num[block_id] - shield_request_req_num));

  return req_num;
}

std::queue<int> Get_where_replace(std::string ori_ops, std::string replace_ops)
{
  std::queue<int> ans;
  if (IS_FIRST)
  {
    for (int i = 0; i < ori_ops.size(); i++)
    {
      if (ori_ops[i] == 'p')
      {
        ans.push(i);
      }
    }
  }
  else
  {
    for (int i = 0; i < ori_ops.size(); i++)
    {
      if (ori_ops[i] != replace_ops[i])
      {
        ans.push(i);
      }
    }
  }
  return ans;
}

std::queue<int> Disk::Get_isolate_r(std::string ops, int tag)
{
  std::queue<int> ans;

  int t_rank = TAG_RANK[tag];

  int begin = DISK_START[t_rank];

  int end = DISK_START[t_rank + 1];

  if (IS_FIRST)
  {
    for (int i = cell_num; i >= 0; i--)
    {
      if (i >= begin && i <= end)
      {
        continue;
      }
      if (cells[i].obj_id == 0)
      {
        continue;
      }
      if ((*objects)[cells[i].obj_id].tag == tag)
      {
        ans.push(i - begin);
      }
    }
  }
  else
  {
    for (int i = 0; i <= this->cell_num; i++)
    {
      if (i >= begin && i <= end)
      {
        continue;
      }
      if (cells[i].obj_id == 0)
      {
        continue;
      }
      if ((*objects)[cells[i].obj_id].tag == tag)
      {
        ans.push(i - begin);
      }
    }
  }
  return ans;
}

std::queue<int> Disk::Get_isolate_r2(std::string ops, int tag)
{
  std::queue<int> ans;

  int t_rank = TAG_RANK[tag];

  int begin = DISK_START[t_rank];

  int end = DISK_START[t_rank + 1];

  for (int i = ops.size() - 1; i > 0; i--)
  {
    if (ops[i] == 'r')
    {
      ans.push(i);
    }
  }
  return ans;
}

std::vector<std::pair<int, int>> Disk::per_disk_exchange_cell(std::vector<int> tag_list)
{

  std::vector<std::pair<int, int>> ops;
  for (int i = 0; i < tag_list.size(); i++)
  {
    // 首先得到原始的操作字符串
    // 然后得到替换后的字符串
    // 然后找到哪一些是被替换为r的下标，接下来查看替换后的字符串
    // 优先检索rpr，然后prrp，然后prrrp，将这些r填入被替换的r位置上

    int t_rank = TAG_RANK[tag_list[i]];

    int begin = DISK_START[t_rank];

    int end = DISK_START[t_rank + 1];

    std::string ori_ops = "";

    for (int i = begin; i < end; i++)
    {
      if (cells[i].obj_id != 0)
      {
        ori_ops.push_back('r');
      }
      else
      {
        ori_ops.push_back('p');
      }
    }

    std::string replace_ops = find_replacement_indices(ori_ops, 64);

    std::queue<int> replace_index = Get_where_replace(ori_ops, replace_ops);

    std::queue<int> isolate_index = Get_isolate_r(ori_ops, tag_list[i]);

    while (!replace_index.empty() && !isolate_index.empty() && this->exchange_time > 0)
    {
      int first = replace_index.front() + begin;
      int second = isolate_index.front() + begin;
      if ((*objects)[cells[second].obj_id].tag != tag_list[i])
      {
        isolate_index.pop();
        continue;
      }
      (*objects)[cells[second].obj_id].write_area = TAG_RANK[tag_list[i]];
      ops.push_back(std::make_pair(first, second));
      exchange_time--;
      replace_index.pop();
      isolate_index.pop();
    }
  }
  return ops;
}

std::vector<std::pair<int, int>> Disk::per_disk_exchange_cell2(std::vector<int> tag_list)
{

  std::vector<std::pair<int, int>> ops;
  for (int i = 0; i < tag_list.size(); i++)
  {
    // 首先得到原始的操作字符串
    // 然后得到替换后的字符串
    // 然后找到哪一些是被替换为r的下标，接下来查看替换后的字符串
    // 优先检索rpr，然后prrp，然后prrrp，将这些r填入被替换的r位置上

    int t_rank = TAG_RANK[tag_list[i]];

    int begin = DISK_START[t_rank];

    int end = DISK_START[t_rank + 1];

    std::string ori_ops = "";

    for (int i = begin; i < end; i++)
    {
      if (cells[i].obj_id != 0)
      {
        ori_ops.push_back('r');
      }
      else
      {
        ori_ops.push_back('p');
      }
    }

    std::string replace_ops = find_replacement_indices(ori_ops, 64);

    std::queue<int> replace_index = Get_where_replace(ori_ops, replace_ops);

    std::queue<int> isolate_index = Get_isolate_r2(ori_ops, tag_list[i]);

    while (!replace_index.empty() && !isolate_index.empty() && this->exchange_time > 0)
    {
      int first = replace_index.front() + begin;
      int second = isolate_index.front() + begin;
      if ((*objects)[cells[second].obj_id].tag != tag_list[i])
      {
        isolate_index.pop();
        continue;
      }
      (*objects)[cells[second].obj_id].write_area = TAG_RANK[tag_list[i]];
      ops.push_back(std::make_pair(first, second));
      exchange_time--;
      replace_index.pop();
      isolate_index.pop();
    }
  }
  return ops;
}

void Disk::update_reserve_end(int point_id)
{
  point[point_id].reserve_end = point[point_id].position + POINTER_RESTRICTION_RANGE;
}

void Disk::mirror_exchange_cell(std::vector<std::pair<int, int>> change_list)
{
  for (auto [first, second] : change_list)
  {
    int first_obj_id = cells[first].obj_id;
    int first_block_id = cells[first].block_id;
    int second_obj_id = cells[second].obj_id;
    int second_block_id = cells[second].block_id;

    if (first_obj_id)
    {
      (*objects)[first_obj_id].unit[0][first_block_id] = second;
    }
    if (second_obj_id)
    {
      (*objects)[second_obj_id].unit[0][second_block_id] = first;
    }
    std::swap(cells[first].obj_id, cells[second].obj_id);
    std::swap(cells[first].block_id, cells[second].block_id);
  }
}
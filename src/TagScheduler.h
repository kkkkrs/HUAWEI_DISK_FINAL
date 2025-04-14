#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

class LinearTagScheduler {
  int tag_num;                            // 标签总数（1-based编号）
  int period_num;                         // 观测周期数
  std::vector<std::vector<double>> tag_data; // 数据存储[周期][标签]

  // 计算两个标签序列的欧氏距离
  double calculate_distance(int tag1, int tag2) const {
    double sum = 0.0;
    for (int t = 1; t <= period_num; ++t) {
      double diff = static_cast<double>(tag_data[t][tag1]) -
                    static_cast<double>(tag_data[t][tag2]);
      sum += diff * diff;
    }
    return std::sqrt(sum);
  }

public:
  LinearTagScheduler(int tags, int periods)
      : tag_num(tags), period_num(periods),
        tag_data(periods + 1, std::vector<double>(tags + 1, 0)) {}

  // 核心优化函数：获取最优线性序列
  std::vector<int> get_optimal_sequence() {
    const int n = tag_num;

    // 1. 构建距离矩阵（0-based索引）
    std::vector<std::vector<double>> dist(n, std::vector<double>(n, 0));
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        dist[i][j] = calculate_distance(i + 1, j + 1); // 转换为1-based编号
      }
    }

    // 2. 动态规划初始化
    const int FULL_MASK = (1 << n) - 1;
    const double INF = std::numeric_limits<double>::max();

    // 使用二维数组存储状态
    std::vector<std::vector<double>> dp(1 << n, std::vector<double>(n, INF));
    std::vector<std::vector<int>> prev(1 << n, std::vector<int>(n, -1));

    // 初始化单节点路径
    for (int u = 0; u < n; ++u) {
      dp[1 << u][u] = 0.0;
    }

    // 3. 状态转移
    for (int mask = 0; mask < (1 << n); ++mask) {
      for (int u = 0; u < n; ++u) {
        // 跳过无效状态
        if (!(mask & (1 << u)) || dp[mask][u] >= INF)
          continue;

        // 尝试扩展路径
        for (int v = 0; v < n; ++v) {
          if (mask & (1 << v))
            continue;

          const int new_mask = mask | (1 << v);
          const double new_cost = dp[mask][u] + dist[u][v];

          // 更新更优路径
          if (new_cost < dp[new_mask][v]) {
            dp[new_mask][v] = new_cost;
            prev[new_mask][v] = u;
          }
        }
      }
    }

    // 4. 寻找最优终点
    int best_end = -1;
    double min_cost = INF;
    for (int u = 0; u < n; ++u) {
      if (dp[FULL_MASK][u] < min_cost) {
        min_cost = dp[FULL_MASK][u];
        best_end = u;
      }
    }

    // 5. 回溯路径（0-based → 1-based）
    std::vector<int> path;
    int current = best_end;
    int mask = FULL_MASK;

    while (current != -1) {
      path.push_back(current + 1); // 转换为1-based编号
      int prev_node = prev[mask][current];
      mask ^= (1 << current); // 移除当前节点
      current = prev_node;
    }

    // std::reverse(path.begin(), path.end());

    return path;
  }

  // 数据更新接口
  void update_data(int period, int tag, double value) {
    if (period < 1 || period > period_num || tag < 1 || tag > tag_num) {
      throw std::out_of_range("Invalid index");
    }
    tag_data[period][tag] = value;
  }

  // 批量数据加载
  void load_data(const std::vector<std::vector<double>> &data) {
    if (data.size() != period_num + 1 || data[0].size() != tag_num + 1) {
      throw std::invalid_argument("Data dimension mismatch");
    }
    tag_data = data;
  }
};

// #include <vector>
// #include <cmath>
// #include <algorithm>
// #include <limits>
////// //#include "logger.h"

// class TagScheduler {
//     int tag_num;    // 总tag数量
//     int period_num; // 总周期数
//     std::vector<std::vector<int>> tag_request_count;

// public:
//     TagScheduler(int tags, int periods)
//         : tag_num(tags), period_num(periods),
//           tag_request_count(periods+1, std::vector<int>(tags+1, 0)) {}

//     // 核心调度函数
//     std::vector<int> get_optimal_sequence() {
//         const int n = tag_num;
//         const int cycles = period_num;

//         // 1. 数据准备：将输入数据转换为double类型的趋势数据
//         std::vector<std::vector<double>> trend_data(n,
//         std::vector<double>(cycles)); for(int tag = 1; tag <= n; ++tag) {
//             for(int t = 1; t <= cycles; ++t) {
//                 trend_data[tag-1][t-1] = tag_request_count[t][tag];
//             }
//         }

//         // 2. 计算距离矩阵
//         auto compute_distances = [&]() {
//             std::vector<std::vector<double>> dist(n, std::vector<double>(n,
//             0)); for(int i = 0; i < n; ++i) {
//                 for(int j = 0; j < n; ++j) {
//                     double sum = 0.0;
//                     for(int k = 0; k < cycles; ++k) {
//                         double diff = trend_data[i][k] - trend_data[j][k];
//                         sum += diff * diff;
//                     }
//                     dist[i][j] = std::sqrt(sum);
//                 }
//             }
//             return dist;
//         };

//         // 3. 动态规划求解
//         const auto& dist = compute_distances();
//         const int FULL_MASK = (1 << n) - 1;
//         const double INF = std::numeric_limits<double>::max();

//         // DP表：dp[mask][u] 表示经过mask集合中的节点，最后位于u的最小代价
//         std::vector<std::vector<double>> dp(1 << n, std::vector<double>(n,
//         INF)); std::vector<std::vector<int>> prev(1 << n, std::vector<int>(n,
//         -1));

//         // 初始化起点
//         for(int u = 0; u < n; ++u) {
//             dp[1 << u][u] = 0.0;
//         }

//         // 状态转移
//         for(int mask = 0; mask < (1 << n); ++mask) {
//             for(int u = 0; u < n; ++u) {
//                 if(!(mask & (1 << u)) || dp[mask][u] == INF) continue;

//                 for(int v = 0; v < n; ++v) {
//                     if(mask & (1 << v)) continue;

//                     int new_mask = mask | (1 << v);
//                     if(dp[new_mask][v] > dp[mask][u] + dist[u][v]) {
//                         dp[new_mask][v] = dp[mask][u] + dist[u][v];
//                         prev[new_mask][v] = u;
//                     }
//                 }
//             }
//         }

//         // 4. 回溯路径
//         int end_node = -1;
//         double min_cost = INF;
//         for(int u = 0; u < n; ++u) {
//             if(dp[FULL_MASK][u] < min_cost) {
//                 min_cost = dp[FULL_MASK][u];
//                 end_node = u;
//             }
//         }

//         std::vector<int> path;
//         int current_mask = FULL_MASK;
//         int current_node = end_node;

//         while(current_node != -1) {
//             path.push_back(current_node + 1); // 转换为1-based编号
//             int prev_node = prev[current_mask][current_node];
//             current_mask &= ~(1 << current_node);
//             current_node = prev_node;
//         }

//         std::reverse(path.begin(), path.end());

//         // for(int i = 0;i<path.size();i++){
////// //        //     LOG_INFO("TAG %d 和 TAG %d 的相关系数
///%d",path[i],path[i+1],dist[path[i]][path[i+1]]);
//         // }

//         return path;
//     }

//     // 数据更新接口（示例）
//     void update_data(int period, int tag, int value) {
//         tag_request_count[period][tag] = value;
//     }
// };

// // #include <vector>
// // #include <cmath>
// // #include <algorithm>
// // #include <limits>
// // #include <stdexcept>
////// //// #include "logger.h"

// // class CircularTagScheduler {
// //     int tag_num;    // Tag总数（有效编号1~tag_num）
// //     int period_num; // 观测周期数（有效周期1~period_num）

// //     // 引用外部静态数据（注意：需保证数据生命周期）
// //     const std::vector<std::vector<int> >& tag_request_count;

// //     // 转换数据索引到内部存储格式
// //     inline int internal_tag_id(int original) const { return original - 1;
// }
// //     inline int internal_period_id(int original) const { return original -
// 1; }

// //     // 计算两个tag序列的DTW距离（处理不同长度序列）
// //     double dynamic_time_warping(const std::vector<double>& s1,
// //                                const std::vector<double>& s2) const {
// //         const size_t n = s1.size(), m = s2.size();
// //         std::vector<std::vector<double>> dtw(n+1, std::vector<double>(m+1,
// INFINITY));
// //         dtw[0][0] = 0.0;

// //         for(size_t i=1; i<=n; ++i) {
// //             for(size_t j=1; j<=m; ++j) {
// //                 double cost = std::abs(s1[i-1] - s2[j-1]);
// //                 dtw[i][j] = cost + std::min({dtw[i-1][j], dtw[i][j-1],
// dtw[i-1][j-1]});
// //             }
// //         }
// //         return dtw[n][m];
// //     }

// // public:
// //     // 构造函数：绑定外部数据引用
// //     CircularTagScheduler(int tags, int periods,
// //                         const std::vector<std::vector<int>>& ext_data)
// //         : tag_num(tags), period_num(periods),
// //           tag_request_count(ext_data)
// //     {
// //         // 验证数据维度
// //         if(tag_request_count.size() != period_num+1 ||
// //            tag_request_count[0].size() != tag_num+1) {
////// ////             LOG_ERROR("数据维度不匹配: 期望 %dx%d 实际 %zdx%zu",
// //                      period_num+1, tag_num+1,
// //                      tag_request_count.size(),
// //                      tag_request_count.empty() ? 0 :
// tag_request_count[0].size());
// //             throw std::invalid_argument("Invalid data dimensions");
// //         }
// //     }

// //     // 核心优化接口（异常安全版本）
// //     std::vector<int> optimize_sequence() noexcept(false) {
// //         try {
// //             // 1. 数据有效性检查
// //             validate_data_integrity();

// //             // 2. 构建时间序列数据（0-based内部索引）
// //             std::vector<std::vector<double>> time_series(tag_num);
// //             for(int tag = 1; tag <= tag_num; ++tag) {
// //                 const int tid = internal_tag_id(tag);
// //                 time_series[tid].reserve(period_num);
// //                 for(int t = 1; t <= period_num; ++t) {
// //                     const int pid = internal_period_id(t);
// //                     time_series[tid].push_back(
// //                         static_cast<double>(tag_request_count[pid][tid])
// //                     );
// //                 }
// //             }

// //             // 3. 计算相似度矩阵
// //             auto dist_matrix = compute_distance_matrix(time_series);

// //             // 4. 动态规划求解TSP（代码同上，略）
// //             // ...

// //             return result_sequence;
// //         }
// //         catch(const std::exception& e) {
////// ////             LOG_CRITICAL("优化过程异常: %s", e.what());
// //             throw; // 重新抛出异常
// //         }
// //     }

// // private:
// //     // 数据完整性验证
// //     void validate_data_integrity() const {
// //         for(int t=1; t<=period_num; ++t) {
// //             const int pid = internal_period_id(t);
// //             if(tag_request_count[pid].size() != tag_num+1) {
// //                 throw std::runtime_error("数据在周期" + std::to_string(t)
// + "处损坏");
// //             }
// //         }
// //     }

// //     // 计算距离矩阵（可配置不同度量方式）
// //     std::vector<std::vector<double>> compute_distance_matrix(
// //         const std::vector<std::vector<double>>& series) const
// //     {
// //         const int n = series.size();
// //         std::vector<std::vector<double>> dist(n, std::vector<double>(n,
// 0.0));

// //         #pragma omp parallel for
// //         for(int i=0; i<n; ++i) {
// //             for(int j=i+1; j<n; ++j) {
// //                 double d = dynamic_time_warping(series[i], series[j]);
// //                 dist[i][j] = d;
// //                 dist[j][i] = d; // 对称矩阵
// //             }
// //         }
// //         return dist;
// //     }
// // };

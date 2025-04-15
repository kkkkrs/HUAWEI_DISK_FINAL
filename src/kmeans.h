#include <random>
#include <unordered_map>
#include <vector>

// 主聚类函数
std::unordered_map<int, std::vector<int>>
clusterObjects(const std::unordered_map<int, std::vector<int>> &data,
               int n_clusters, int window_size, int total_num);
// 生成特征向量
std::vector<int> generateFeatureVector(const std::vector<int> &timestamps,
                                       int window_size, int num_windows);

// 特征提取
std::unordered_map<int, std::vector<int>>
extractFeatures(const std::unordered_map<int, std::vector<int>> &data,
                int window_size, int num_windows);
// 计算两点之间的欧几里得距离
int euclideanDistance(const std::vector<int> &a, const std::vector<int> &b);
// 找到最近的质心
int findClosestCentroid(const std::vector<int> &feature,
                        const std::vector<std::vector<int>> &centroids);
// 更新质心
std::vector<std::vector<int>>
updateCentroids(const std::unordered_map<int, std::vector<int>> &features,
                const std::vector<int> &labels, int n_clusters);
// KMeans 聚类
std::unordered_map<int, std::vector<int>>
kmeansClustering(const std::unordered_map<int, std::vector<int>> &features,
                 int n_clusters = 10);

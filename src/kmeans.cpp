#include <random>
#include <unordered_map>
#include <vector>

// 生成特征向量
std::vector<int> generateFeatureVector(const std::vector<int> &timestamps,
                                       int window_size, int num_windows) {
  std::vector<int> feature_vector(num_windows, 0);
  for (int t : timestamps) {
    int window_idx = t / window_size;
    if (window_idx < num_windows) {
      feature_vector[window_idx]++;
    }
  }
  return feature_vector;
}

// 特征提取
std::unordered_map<int, std::vector<int>>
extractFeatures(const std::unordered_map<int, std::vector<int>> &data,
                int window_size, int num_windows) {
  std::unordered_map<int, std::vector<int>> features;
  for (const auto &[obj_id, timestamps] : data) {
    features[obj_id] =
        generateFeatureVector(timestamps, window_size, num_windows);
  }
  return features;
}

// 计算两点之间的欧几里得距离
int euclideanDistance(const std::vector<int> &a, const std::vector<int> &b) {
  int dist = 0;
  for (size_t i = 0; i < a.size(); ++i) {
    dist += (a[i] - b[i]) * (a[i] - b[i]);
  }
  return dist;
}

// 找到最近的质心
int findClosestCentroid(const std::vector<int> &feature,
                        const std::vector<std::vector<int>> &centroids) {
  int min_dist = std::numeric_limits<int>::max();
  int closest_centroid = 0;
  for (size_t i = 0; i < centroids.size(); ++i) {
    int dist = euclideanDistance(feature, centroids[i]);
    if (dist < min_dist) {
      min_dist = dist;
      closest_centroid = i;
    }
  }
  return closest_centroid;
}

// 更新质心
std::vector<std::vector<int>>
updateCentroids(const std::unordered_map<int, std::vector<int>> &features,const std::vector<int> &labels, int n_clusters) {
  std::vector<std::vector<int>> new_centroids(
      n_clusters, std::vector<int>(features.begin()->second.size(), 0));
  std::vector<int> counts(n_clusters, 0);

  int idx = 0;
  for (const auto &[obj_id, feature] : features) {
    int cluster = labels[idx++];
    for (size_t j = 0; j < feature.size(); ++j) {
      new_centroids[cluster][j] += feature[j];
    }
    counts[cluster]++;
  }

  for (int i = 0; i < n_clusters; ++i) {
    if (counts[i] > 0) {
      for (size_t j = 0; j < new_centroids[i].size(); ++j) {
        new_centroids[i][j] /= counts[i];
      }
    }
  }
  return new_centroids;
}

// KMeans 聚类
std::unordered_map<int, std::vector<int>> kmeansClustering(const std::unordered_map<int, std::vector<int>> &features, int n_clusters = 10) {
  std::vector<std::vector<int>> centroids(n_clusters);
  std::vector<int> labels(features.size());

  // 初始化质心
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, features.size() - 1);
  auto it = features.begin();
  for (int i = 0; i < n_clusters; ++i) {
    std::advance(it, dis(gen));
    centroids[i] = it->second;
    it = features.begin();
  }

  // 迭代更新质心
  for (int iter = 0; iter < 10; ++iter) {
    int idx = 0;
    for (const auto &[obj_id, feature] : features) {
      labels[idx++] = findClosestCentroid(feature, centroids);
    }
    centroids = updateCentroids(features, labels, n_clusters);
  }

  // 整理聚类结果
  std::unordered_map<int, std::vector<int>> clusters;
  int idx = 0;
  for (const auto &[obj_id, feature] : features) {
    clusters[labels[idx++]].push_back(obj_id);
  }
  return clusters;
}

// 主聚类函数
std::unordered_map<int, std::vector<int>>
clusterObjects(const std::unordered_map<int, std::vector<int>> &data, int n_clusters, int window_size, int total_num) {
  // int window_size = 20;
  int num_windows = total_num / window_size + 1;
  auto features = extractFeatures(data, window_size, num_windows);
  return kmeansClustering(features, n_clusters);
}

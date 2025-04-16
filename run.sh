#!/bin/zsh

# 构建项目
cmake -S ./src -B ./src/build
cmake --build ./src/build

# rm -rf replay

# 获取第一个参数，表示你想运行的数据集编号
choice=$1
shift  # 把第一个参数移除，这样 $@ 就只剩下后续传给程序的参数

# 选择要运行的数据集
case $choice in
  1)
    python ./run.py interactor/interactor data/sample_practice_1.in "./src/code_craft" "$@"
    ;;
  2)
    python ./run.py interactor/interactor data/sample_practice_2.in "./src/code_craft" "$@"
    ;;
  3)
    python ./run.py interactor/interactor data/sample_practice_3.in "./src/code_craft" "$@"
    ;;
  *)
    echo "❌ 参数错误！请输入 1、2 或 3 选择数据集。"
    exit 1
    ;;
esac

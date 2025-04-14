#!/bin/zsh

cmake -S ./src -B ./src/build
cmake --build ./src/build
sleep 2
python ./run.py interactor/interactor-live data/sample_official.in "./src/code_craft" "$@"
# python ./run.py interactor/macos/interactor data/practice.in "./srcpp/code_craft" "$@"
# python ./run.py interactor/macos/interactor data/sample_extra.in "./srcpp/code_craft" "$@"
# python ./run.py interactor/macos/interactor data/sample_offical.in "./src/code_craft" "$@"


# # 切换到 code/build 目录
# cd code/build
# # 运行 make 命令
# make

# echo "构建完成！"

# # 切换回上一级目录
# cd ..
# cd ..

# rm -rf replay

# conda activate base

# # 运行 Python 脚本，并将传递给本脚本的所有参数传递给 Python 脚本
# python code/run.py interactor/macos/interactor data/sample_practice.in "./srcpp/code_craft"

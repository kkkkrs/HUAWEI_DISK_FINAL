import os
import sys
import zipfile
import argparse
import subprocess
from datetime import datetime


def comment_log_lines(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    new_lines = []
    for line in lines:
        if 'LOG_DEBUG' in line or 'LOG_INFO' in line or 'LOG_WARNING' in line or 'LOG_ERROR' in line or 'LOG' in line or 'log' in line:
            new_lines.append('//' + line)
        else:
            new_lines.append(line)
    with open(file_path, 'w', encoding='utf-8') as file:
        file.writelines(new_lines)


def uncomment_log_lines(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    new_lines = []
    for line in lines:
        if line.startswith('//') and ('LOG_DEBUG' in line or 'LOG_INFO' in line or 'LOG_WARNING' in line or 'LOG_ERROR' in line or 'LOG' in line or 'log' in line):
            new_lines.append(line[2:])
        else:
            new_lines.append(line)
    with open(file_path, 'w', encoding='utf-8') as file:
        file.writelines(new_lines)


def process_files(directory, operation):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.cpp') or file.endswith('.h'):
                file_path = os.path.join(root, file)
                if operation == 'comment':
                    comment_log_lines(file_path)
                elif operation == 'uncomment':
                    uncomment_log_lines(file_path)


def should_ignore(path):
    """
    判断路径是否应该被忽略（名称中包含 code_craft 或为 LOGGER.h 或为 .DS_Store）
    """
    base_name = os.path.basename(path)
    return "code_craft" in base_name or base_name == "LOGGER.h" or ".DS_Store" in base_name


def compress_files(input_dir, output_zip_path):
    """
    压缩指定目录下的文件
    """
    try:
        # 确保提交文件夹存在
        output_dir = os.path.dirname(output_zip_path)
        os.makedirs(output_dir, exist_ok=True)

        with zipfile.ZipFile(output_zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for root, dirs, files in os.walk(input_dir):
                # 跳过 build 文件夹
                if 'build' in dirs:
                    dirs.remove('build')
                if 'debug' in dirs:
                    dirs.remove('debug')
                # 过滤掉需要忽略的文件夹
                dirs[:] = [d for d in dirs if not should_ignore(os.path.join(root, d))]
                for file in files:
                    file_path = os.path.join(root, file)
                    if not should_ignore(file_path):
                        # 计算在压缩包中的相对路径
                        arcname = os.path.relpath(file_path, input_dir)
                        zipf.write(file_path, arcname)
        print(f"成功压缩 {input_dir} 到 {output_zip_path}")
    except Exception as e:
        print(f"压缩过程中出现错误: {e}")


if __name__ == "__main__":
    # 创建命令行参数解析器
    parser = argparse.ArgumentParser(description='压缩指定目录下的文件')
    parser.add_argument('archive_name', type=str, help='压缩包名称（不包含扩展名）')
    args = parser.parse_args()

    input_dir = "./src"
    output_folder = "subbmit"
    now = datetime.now()
    time_str = now.strftime("%m%d%p%H:%M")

    output_zip_name = f"{time_str}_{args.archive_name}.zip"
    output_zip_path = os.path.join(output_folder, output_zip_name)

    # 在打包之前执行注释操作
    process_files(input_dir, "comment")

    # 执行压缩操作
    compress_files(input_dir, output_zip_path)

    # 在打包之后执行取消注释操作
    process_files(input_dir, "uncomment")
    
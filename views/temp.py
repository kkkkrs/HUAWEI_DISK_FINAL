import sys

def count_tags():
    tag_counts = {}
    
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        
        try:
            _, tag = line.split()
            tag = int(tag)
            tag_counts[tag] = tag_counts.get(tag, 0) + 1
        except ValueError:
            continue  # 跳过格式错误的行

    # 按tag数值排序输出
    for tag in sorted(tag_counts.keys()):
        print(f"{tag}: {tag_counts[tag]}")

if __name__ == "__main__":
    count_tags()
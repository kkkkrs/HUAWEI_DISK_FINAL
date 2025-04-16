def parse_prediction_line(line):
    """解析预测文件中的单行数据"""
    parts = line.strip().split()
    try:
        return int(parts[-2]), int(parts[-1])  # 提取最后两个数字：obj_id 和 predicted_tag
    except (IndexError, ValueError) as e:
        print(f"解析行时出错：'{line.strip()}'\n错误详情：{str(e)}")
        return None, None

def load_predictions(pred_file):
    """加载预测结果"""
    predictions = {}
    with open(pred_file, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            if line.strip():
                obj_id, pred_tag = parse_prediction_line(line)
                if obj_id is not None and pred_tag is not None:
                    if obj_id in predictions:
                        print(f"警告：对象ID {obj_id} 存在重复预测（第{line_num}行）")
                    predictions[obj_id] = pred_tag
    return predictions

def load_ground_truth(truth_file):
    """加载真实标签"""
    ground_truth = {}
    with open(truth_file, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if line:
                try:
                    obj_id, true_tag = map(int, line.split())
                    if obj_id in ground_truth:
                        print(f"警告：对象ID {obj_id} 存在重复真实标签（第{line_num}行）")
                    ground_truth[obj_id] = true_tag
                except ValueError as e:
                    print(f"解析行时出错：'{line}'\n错误详情：{str(e)}（第{line_num}行）")
    return ground_truth

def calculate_predicted_accuracy(predictions, ground_truth):
    """计算已预测样本的准确率"""
    stats = {
        'total_predictions': len(predictions),
        'valid_predictions': 0,
        'correct_predictions': 0,
        'tag_details': {}
    }

    # 统计有效预测
    for obj_id, pred_tag in predictions.items():
        if obj_id in ground_truth:
            true_tag = ground_truth[obj_id]
            stats['valid_predictions'] += 1
            
            # 初始化标签统计
            if true_tag not in stats['tag_details']:
                stats['tag_details'][true_tag] = {
                    'total': 0,
                    'correct': 0
                }
            
            # 更新统计
            stats['tag_details'][true_tag]['total'] += 1
            if pred_tag == true_tag:
                stats['correct_predictions'] += 1
                stats['tag_details'][true_tag]['correct'] += 1

    # 计算准确率
    stats['overall_accuracy'] = (
        stats['correct_predictions'] / stats['valid_predictions'] 
        if stats['valid_predictions'] > 0 
        else 0
    )
    
    # 计算各标签准确率
    for tag, detail in stats['tag_details'].items():
        detail['accuracy'] = (
            detail['correct'] / detail['total'] 
            if detail['total'] > 0 
            else 0
        )
    
    return stats

def format_percentage(value):
    """格式化百分比输出"""
    return f"{value * 100:.2f}%"

if __name__ == "__main__":
    # 配置文件路径
    PRED_FILE = "../logs/日志20250416_141344.log"
    TRUTH_FILE = "../data/sample_practice_map_1.txt"
    
    print("正在加载数据...")
    try:
        predictions = load_predictions(PRED_FILE)
        ground_truth = load_ground_truth(TRUTH_FILE)
    except FileNotFoundError as e:
        print(f"文件加载失败：{str(e)}")
        exit(1)
    
    print("\n数据分析中...")
    stats = calculate_predicted_accuracy(predictions, ground_truth)
    
    # 打印统计报告
    print("\n" + "="*50)
    print(f"预测分析报告".center(50))
    print("="*50)
    print(f"总预测数量: {stats['total_predictions']}")
    print(f"有效预测数量: {stats['valid_predictions']} "
          f"({format_percentage(stats['valid_predictions']/stats['total_predictions'])})")
    print(f"无效预测数量: {stats['total_predictions'] - stats['valid_predictions']}")
    print(f"\n整体准确率: {format_percentage(stats['overall_accuracy'])} "
          f"({stats['correct_predictions']}/{stats['valid_predictions']})")
    
    # 各标签准确率
    print("\n各标签详细统计:")
    print("-"*50)
    print(f"{'标签':<6} {'准确率':<10} {'正确数':<8} {'总数':<6} {'覆盖率':<10}")
    print("-"*50)
    for tag in sorted(stats['tag_details'].keys()):
        detail = stats['tag_details'][tag]
        coverage = detail['total'] / stats['valid_predictions'] if stats['valid_predictions'] > 0 else 0
        print(f"{tag:<6} {format_percentage(detail['accuracy']):<10} "
              f"{detail['correct']:<8} {detail['total']:<6} "
              f"{format_percentage(coverage):<10}")
    
    print("="*50)
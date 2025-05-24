#!/bin/bash

# 配置参数
SERVER_IP="127.0.0.1"            # 服务器IP
PORTS=(8081 8082 8083 8084)      # 可用服务器端口
TEST_CODE='{
  "code": "#include <iostream>\n#include <vector>\n#include <algorithm>\nint main() {\n  std::vector<int> v(10000);\n  std::generate(v.begin(), v.end(), rand);\n  std::sort(v.begin(), v.end());\n  std::cout << \"Sorted \" << v.size() << \" elements\" << std::endl;\n  return 0;\n}",
  "input": ""
}'
INITIAL_CONCURRENCY=50           # 初始并发数
MAX_CONCURRENCY=1000              # 最大并发数
STEP_SIZE=50                     # 并发递增步长
TEST_DURATION=10                 # 每个压力级别的测试时长(秒)
COOLDOWN=5                       # 压力级别间的冷却时间(秒)
RESULT_FILE="stress_test_results.csv"

# 初始化结果文件
echo "timestamp,concurrency,port,response_time,status,cpu_usage,mem_usage" > "$RESULT_FILE"

# 获取系统信息函数
get_system_metrics() {
  local metrics
  metrics=$(top -bn1 | grep "Cpu(s)" | awk '{printf "%.1f,%.1f", 100-$8, $4}')
  echo "$metrics"  # 返回 CPU使用率%,内存使用率%
}

# 单个请求测试函数
send_request() {
  local port=$1
  local url="http://$SERVER_IP:$port/compiler_run"
  local start_time=$(date +%s%3N)
  
  # 发送请求并捕获HTTP状态码和响应时间
  local http_code=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Content-Type: application/json" \
    -d "$TEST_CODE" \
    -X POST "$url" --connect-timeout 10 --max-time 30)
  
  local end_time=$(date +%s%3N)
  local duration=$((end_time - start_time))
  
  # 判断请求状态
  local status="FAIL"
  [[ $http_code =~ ^[23][0-9]{2}$ ]] && status="SUCCESS"
  
  # 获取系统指标
  local system_metrics=$(get_system_metrics)
  local cpu_usage=$(echo "$system_metrics" | cut -d',' -f1)
  local mem_usage=$(echo "$system_metrics" | cut -d',' -f2)
  
  # 记录结果
  echo "$(date +'%Y-%m-%d %H:%M:%S'),$current_concurrency,$port,$duration,$status,$cpu_usage,$mem_usage" >> "$RESULT_FILE"
}

# 压力测试函数
run_stress_test() {
  local concurrency=$1
  local port=${PORTS[$RANDOM % ${#PORTS[@]}]}
  
  echo "正在测试并发级别: $concurrency 使用端口: $port"
  
  for ((i=0; i<concurrency; i++)); do
    send_request "$port" &
    sleep 0.1  # 控制请求发起速率
  done
  
  sleep "$TEST_DURATION"  # 保持当前并发压力
  
  # 等待所有请求完成
  wait
  sleep "$COOLDOWN"  # 冷却期
}

# 主测试流程
echo "开始编译主机压力测试..."
echo "测试代码复杂度: 排序10000个元素的向量"

current_concurrency=$INITIAL_CONCURRENCY
while [[ $current_concurrency -le $MAX_CONCURRENCY ]]; do
  run_stress_test "$current_concurrency"
  
  # 检查系统是否过载
  avg_cpu=$(awk -F',' '{sum+=$6; count++} END {print sum/count}' "$RESULT_FILE" | tail -n 10)
  if (( $(echo "$avg_cpu > 90" | bc -l) )); then
    echo "警告: CPU使用率超过90%，停止增加并发"
    break
  fi
  
  current_concurrency=$((current_concurrency + STEP_SIZE))
done

# 结果分析
echo "测试完成，结果已保存到 $RESULT_FILE"
echo ""
echo "====== 性能上限分析 ======"
awk -F',' '
  $5 == "SUCCESS" {success++} 
  $5 == "FAIL" {fail++}
  {total++}
  END {
    printf "成功请求: %d (%.1f%%)\n失败请求: %d (%.1f%%)\n", 
    success, success/total*100, fail, fail/total*100
  }' "$RESULT_FILE"

echo ""
echo "最大有效并发: $((current_concurrency - STEP_SIZE))"
echo "峰值CPU使用率: $(awk -F',' '{print $6}' "$RESULT_FILE" | sort -nr | head -1)%"
echo "峰值内存使用率: $(awk -F',' '{print $7}' "$RESULT_FILE" | sort -nr | head -1)%"
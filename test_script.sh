#!/bin/bash

# 配置参数
PORTS=(8081 8082 8083 8084)
TARGET_RPS=300                     # 目标RPS（整数）
INTERVAL_MS=100                    # 控制粒度（毫秒）
DURATION=30                        # 测试时长（秒）
SERVER_IP="127.0.0.1"
RESULT_FILE="throughput_test_$(date +%s).csv"
LOCK_FILE="/tmp/curl_test.lock"
STATS_FILE="/tmp/curl_test.stats"  # 新增：统计文件
STATS_REFRESH=1                    # 监控刷新间隔（秒）

# 测试代码
TEST_CODE='{
  "code": "#include <iostream>\n#include <vector>\n#include <algorithm>\nint main() {\n  std::vector<int> v(5000);\n  std::generate(v.begin(), v.end(), rand);\n  std::sort(v.begin(), v.end());\n  return 0;\n}",
  "input": ""
}'

# 初始化结果文件和统计文件
echo "timestamp,port,response_time,status" > "$RESULT_FILE"
echo "0 0" > "$STATS_FILE"  # 格式：总请求数 成功请求数

# 初始化端口统计文件
for port in "${PORTS[@]}"; do
  echo "0 0 0 0" > "/tmp/curl_test_port_${port}.stats"  # 格式：成功数 总响应时间 失败数 错误信息
done

# 请求函数
send_request() {
  local port=${PORTS[$RANDOM % ${#PORTS[@]}]}
  local url="http://$SERVER_IP:$port/compiler_run"
  local temp_out=$(mktemp)
  
  local start_time=$(date +%s%3N)
  http_code=$(curl -s -o /dev/null -w "%{http_code}" \
    --connect-timeout 5 \
    --max-time 10 \
    -H "Content-Type: application/json" \
    -d "$TEST_CODE" \
    -X POST "$url" \
    --local-port $(( 20000 + RANDOM % 10000 )) 2>"$temp_out")
  
  local end_time=$(date +%s%3N)
  local duration=$((end_time - start_time))
  local status="FAIL"
  
  # 更新统计（使用文件锁确保原子性）
  {
    # 更新全局统计
    flock -x 200
    read total success < "$STATS_FILE"
    ((total++))
    if [[ $http_code =~ ^[23][0-9]{2}$ ]]; then
      status="SUCCESS"
      ((success++))
    fi
    echo "$total $success" > "$STATS_FILE"
  } 200>"$STATS_FILE.lock"
  
  # 更新端口统计
  {
    flock -x 300
    read success_count total_time fail_count error < "/tmp/curl_test_port_${port}.stats"
    if [[ $status == "SUCCESS" ]]; then
      ((success_count++))
      ((total_time += duration))
    else
      ((fail_count++))
      error=$(<"$temp_out")
    fi
    echo "$success_count $total_time $fail_count \"$error\"" > "/tmp/curl_test_port_${port}.stats"
  } 300>"/tmp/curl_test_port_${port}.lock"

  # 写入结果
  (
    flock -x 400
    echo "$(date +'%Y-%m-%d %H:%M:%S.%3N'),$port,$duration,$status" >> "$RESULT_FILE"
  ) 400>"$LOCK_FILE"
  
  rm -f "$temp_out"
}

# 实时监控函数
monitor() {
  local last_update=0
  while (( SECONDS < DURATION + 2 )); do
    if (( SECONDS - last_update >= STATS_REFRESH )); then
      clear
      echo "===== 编译服务压力测试实时监控 ====="
      echo "测试时长: $SECONDS/$DURATION 秒 | 目标RPS: $TARGET_RPS"
      
      # 读取全局统计
      read total_requests success_requests < "$STATS_FILE"
      
      # 计算成功率（避免除零错误）
      local success_rate=0
      if (( total_requests > 0 )); then
        success_rate=$(( success_requests * 100 / total_requests ))
      fi
      
      echo "总请求数: $total_requests | 成功率: $success_rate%"
      echo "----------------------------------------"
      printf "%-8s %-10s %-10s %-12s\n" "PORT" "SUCCESS" "FAIL" "AVG_TIME(ms)"
      
      for port in "${PORTS[@]}"; do
        # 读取端口统计
        read success_count total_time fail_count error < "/tmp/curl_test_port_${port}.stats"
        local total=$(( success_count + fail_count ))
        local avg_time=0
        (( total > 0 )) && avg_time=$(( total_time / total ))
        
        printf "%-8s %-10s %-10s %-12s\n" \
          "$port" "$success_count" "$fail_count" "$avg_time"
      done
      
      echo "----------------------------------------"
      # 显示第一个端口的最近错误（截断为100个字符）
      read success_count total_time fail_count error < "/tmp/curl_test_port_${PORTS[0]}.stats"
      echo "最近错误: ${error:0:100}"
      last_update=$SECONDS
    fi
    sleep 0.5
  done
}

# 吞吐量控制器
rps_controller() {
  local interval_sec=$(( INTERVAL_MS / 1000 ))
  local requests_per_interval=$(( TARGET_RPS * INTERVAL_MS / 1000 ))
  local requests_per_second=$(( TARGET_RPS ))
  local sleep_ns=$(( 1000000000 / requests_per_second ))  # 纳秒
  
  while (( SECONDS < DURATION )); do
    local batch_start=$(date +%s%N)
    
    # 发射请求
    for ((i=0; i<requests_per_interval; i++)); do
      send_request &
      
      # 使用纳秒级延迟（需要bc支持）
      if command -v bc &> /dev/null; then
        local delay=$(echo "scale=9; $sleep_ns / 1000000000" | bc)
        sleep $delay
      else
        # 回退到毫秒级延迟
        sleep 0.$(( RANDOM % 10 ))
      fi
    done
    
    # 等待本间隔剩余时间
    local batch_end=$(date +%s%N)
    local elapsed_ns=$(( batch_end - batch_start ))
    local interval_ns=$(( interval_sec * 1000000000 ))
    
    if (( elapsed_ns < interval_ns )); then
      local remaining_ns=$(( interval_ns - elapsed_ns ))
      local remaining_sec=$(echo "scale=9; $remaining_ns / 1000000000" | bc)
      sleep $remaining_sec
    fi
  done
}

# 清理函数
cleanup() {
  # 等待所有后台进程完成
  wait
  
  # 读取最终统计
  read TOTAL_REQUESTS SUCCESS_REQUESTS < "$STATS_FILE"
  
  # 计算最终成功率
  local success_rate=0
  if (( TOTAL_REQUESTS > 0 )); then
    success_rate=$(( SUCCESS_REQUESTS * 100 / TOTAL_REQUESTS ))
  fi
  
  # 清理临时文件
  rm -f "$LOCK_FILE" /tmp/curl_test_*.stats /tmp/curl_test_*.lock
  
  # 最终统计报告
  clear
  echo "====== 测试最终报告 ======"
  echo "总运行时间: $DURATION 秒"
  echo "总请求数: $TOTAL_REQUESTS"
  echo "成功请求: $SUCCESS_REQUESTS ($success_rate%)"
  echo "详细结果已保存到: $RESULT_FILE"
}
trap cleanup EXIT

# 启动测试
monitor &
rps_controller
wait

# 等待监控结束
sleep 2
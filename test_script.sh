#!/bin/bash

submit_code() {
  local user_id=$1
  local user_code=$2
  local question_id=$3
  local server_ip=$4
  local server_port=$5

  # 只读取header
  local header_file="header.cpp"
  local header_code
  header_code=$(<"$header_file")

  # 替换header中的占位符
  local full_code="${header_code//\/\/ USER_CODE/$user_code}"

  # 输出拼接后的代码到debug_code.cpp，便于调试
  echo "$full_code" > debug_code.cpp

  # 生成JSON负载
  local json_payload
  json_payload=$(jq -n --arg code "$full_code" --arg input "" '{"code": $code, "input": $input}')

  # 提交到OJ主服务器
  local start_time=$(date +%s%3N)
  response=$(curl -s -X POST "http://$server_ip:$server_port/judge/$question_id" \
    -H "Content-Type: application/json" \
    -d "$json_payload")
  local end_time=$(date +%s%3N)
  local duration=$((end_time - start_time))
  local status=$(echo "$response" | jq '.status')
  local reason=$(echo "$response" | jq -r '.reason')
  echo "$start_time,$user_id,$question_id,$status,$duration,\"$reason\"" >> result.csv

  echo "User $user_id received response: $response"
}

# 测试参数
server_ip="127.0.0.1"
server_port=8080
question_id=1
user_code='if (x < 0) return false; long long y = 0, t = x; while (t) { y = y * 10 + t % 10; t /= 10; } return x == y;'
  

echo "timestamp,user_id,question_id,status,duration_ms,reason" > result.csv

end_time=$((SECONDS + 10))
while [ $SECONDS -lt $end_time ]; do
  for i in {1..10}; do
    submit_code $i "$user_code" $question_id $server_ip $server_port &
  done
  wait
done

echo "Load test completed."
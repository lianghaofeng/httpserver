# QUICK START

### 1. 编译项目

cd hphs
mkdir -p build && cd build
g++ -std=c++17 -Wall -Wextra -O2 -pthread ../src/*.cpp -o http_server

### 2. 运行
cd build
./http_server 8080 4 ../www
端口 + 线程数 + www的目录

### 3. 访问网页
浏览器打开： `http://localhost:8080`

### 4. 性能测试
使用ab测试（macOS自带）

**格式：**
```bash
ab -n 总请求数 -c 并发数 http://服务器地址/
```

**例子1：1000请求，100并发**
```bash
ab -n 1000 -c 100 http://localhost:8080/
```

16线程结果(普通读写)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.7      0      11
Processing:     2    9   1.8      9      17
Waiting:        2    9   1.8      9      17
Total:          6   10   2.5      9      20
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.8      0       4
Processing:     2    8   1.7      8      13
Waiting:        1    7   1.6      7      12
Total:          4    9   1.6      9      14
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.9      0       5
Processing:     1    8   2.9      8      16
Waiting:        0    7   2.9      6      15
Total:          1    8   2.9      8      17
```


---

**例子2：10000请求，500并发**
```bash
ab -n 10000 -c 500 http://localhost:8080/
```

16线程结果(普通读写)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   3.0      0      26
Processing:    14   42   5.5     42      73
Waiting:        1   42   5.4     42      73
Total:         28   43   5.8     42      76
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   2.2      0      17
Processing:    12   39   3.5     39      56
Waiting:        1   38   3.6     38      54
Total:         21   39   3.1     39      58
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   2.8      0      18
Processing:     4   38  11.3     38      79
Waiting:        1   37  11.3     37      78
Total:          8   40  10.4     39      80
```

---

**例子3：持续60秒，100并发**
```bash
ab -t 60 -n 100000 -c 100 http://localhost:8080/
```

16线程结果(普通读写)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.4      0      19
Processing:     2    8   1.3      8      23
Waiting:        2    8   1.3      8      23
Total:          3    8   1.4      8      24
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.7      0       8
Processing:     1    7   1.7      7      25
Waiting:        0    6   1.6      6      21
Total:          2    8   1.6      7      25
```

16线程结果(sendfile 零拷贝 + work-stealing)
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.7      0      43
Processing:     1    9   9.2      7     167
Waiting:        0    8   8.4      6     159
Total:          1    9   9.3      7     167
```


---

**例子4：持续60秒，5000并发**
```bash
ab -t 60 -n 99999 -c 5000 http://localhost:8080/
```

16线程结果(普通读写)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    3  11.7      0     109
Processing:    45  442  94.5    428     811
Waiting:        0  442  94.6    428     811
Total:        111  444  88.8    428     811
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    3  11.6      0     103
Processing:    69  409 102.5    392     848
Waiting:        1  408 102.3    391     846
Total:        125  412  98.5    392     849
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    3  13.0      0      91
Processing:    32  414 154.7    384    1149
Waiting:        1  413 154.3    383    1145
Total:        118  417 151.0    385    1149
```

**例子5：10000请求，500并发，读取100M大文件**
```bash
dd if=/dev/zero of=www/test-100M.dat bs=10M count=1
ab -t 60 -n 10000 -c 500 http://localhost:8080/test-100M.dat
```

16线程-100M文件(普通读写)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    5   2.1      4       9
Processing:   222 31385 16487.3  31876   60084
Waiting:      199 31125 16504.2  31589   59833
Total:        222 31390 16485.2  31880   60086
```

16线程-100M文件(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    2   6.5      0      41
Processing:   183 4868 840.6   5055    5803
Waiting:        1 4687 837.5   4875    5615
Total:        223 4870 834.4   5055    5803
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   3.7      0      25
Processing:   179 5046 953.7   5053    6470
Waiting:        2 4865 950.3   4879    6289
Total:        203 5047 950.5   5053    6470
```

**例子6：持续60秒，10000并发**
```bash
ab -t 60 -n 999999 -c 10000 http://localhost:8080/
```

16线程结果(普通线程池)： 
```
运行中途崩溃
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   23 285.4      0    5148
Processing:    80  864 287.7    792    1927
Waiting:        1  863 287.3    791    1918
Total:        180  888 399.2    794    5951
```


**例子7：持续60秒，20000并发**
```bash
ab -t 600 -n 9999999 -c 20000 http://localhost:8080/
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0 5120 1534.0   5058   10438
Processing:   348 7569 1720.9   7758   13094
Waiting:        3 5661 1966.0   6047   10500
Total:       4812 12689 1047.8  12569   15466
```
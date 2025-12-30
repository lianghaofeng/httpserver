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
Connect:        0    1   1.8      0       8
Processing:     2    7   2.7      7      18
Waiting:        1    6   2.5      6      15
Total:          2    8   2.8      8      18
```

200线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   2.6      0      14
Processing:     2    6   2.8      5      19
Waiting:        0    3   1.7      3      10
Total:          2    7   4.8      6      27
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
Connect:        0    1   5.5      0      40
Processing:     1   31  10.8     33      54
Waiting:        0   30  10.8     33      52
Total:          1   32  11.6     34      64
```

200线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    2   2.7      1      26
Processing:     2   17   7.6     15      60
Waiting:        1   11   6.5     10      50
Total:          4   19   8.4     17      61
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
Connect:        0    0   0.3      0      20
Processing:     0    8  10.6      6     228
Waiting:        0    6   9.8      5     222
Total:          0    8  10.7      6     228
```

200线程结果(sendfile 零拷贝 + work-stealing)
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.7      0      15
Processing:     0    5   4.1      4      67
Waiting:        0    2   1.3      1      57
Total:          0    5   4.3      4      68
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
Connect:        0    6  26.5      0     257
Processing:    64  409 276.3    336    1801
Waiting:        0  408 275.4    335    1793
Total:        243  415 273.0    338    1801
```

200线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   33  30.6     24     170
Processing:    32  196 150.4    151    1139
Waiting:        2  182 147.8    142    1124
Total:         51  229 151.3    184    1192
```

**例子5：10000请求，500并发，读取100M大文件**
```bash
dd if=/dev/zero of=www/test-100M.dat bs=100M count=1
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
Connect:        0    3   9.0      0      41
Processing:   148 9054 2416.2   9792   11448
Waiting:        0 8727 2399.1   9456   11150
Total:        188 9057 2407.7   9792   11448
```

16线程-100M文件(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    3   7.2      0      33
Processing:   244 8790 2192.3   9326   11193
Waiting:        1 8473 2185.2   8967   10879
Total:        276 8793 2185.5   9327   11193
```

200线程-100M文件(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    5  11.6      1      41
Processing:  2165 8095 2696.1   8013   13848
Waiting:        1 4605 2134.8   4396    9843
Total:       2201 8101 2688.6   8014   13849
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
Connect:        0   22 180.2      0    2025
Processing:   215  906 622.1    706    4534
Waiting:        2  904 620.4    704    4524
Total:        430  928 628.0    711    4534
```

200线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   69  96.8     47    1592
Processing:    54  432 385.9    316    2465
Waiting:        1  404 370.7    298    2425
Total:        112  501 396.6    369    2513
```


**例子7：持续60秒，20000并发**
```bash
ab -t 60 -n 9999999 -c 20000 http://localhost:8080/
```

16线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        1 3995 1962.0   3443    8068
Processing:    96 6711 2315.7   7125   11517
Waiting:        2 4734 2279.1   4879    8127
Total:       3440 10706 1828.6  11064   13657
```

200线程结果(sendfile 零拷贝 + work-stealing)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0 3560 1660.8   3145   12324
Processing:   153 5722 1926.2   5987    8967
Waiting:       38 3845 1880.7   3741    7902
Total:       3283 9283 1376.4   9602   17299
```
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

4线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.4      0       2
Processing:     9   30   8.3     28      62
Waiting:        7   30   8.3     28      62
Total:          9   30   8.6     28      64
```

16线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.6      0      52
Processing:     2   14   6.6     13     136
Waiting:        2   14   6.6     13     136
Total:          4   14   6.6     13     136
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.5      0       2
Processing:    10   21   7.1     18      44
Waiting:        9   18   6.7     15      41
Total:         10   21   7.4     18      46
```

---

**例子2：10000请求，500并发**
```bash
ab -n 10000 -c 500 http://localhost:8080/
```

4线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.7      0      13
Processing:    15  103  21.7     99     257
Waiting:        6  103  21.7     99     257
Total:         19  103  21.8     99     263
```

16线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.4      0       9
Processing:    15   75  18.0     73     196
Waiting:        6   75  18.0     73     196
Total:         16   75  18.2     73     199
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   1.9      0      12
Processing:    10   79  23.0     77     216
Waiting:        4   76  23.0     75     214
Total:         19   80  23.7     78     226
```

---

**例子3：持续60秒，100并发**
```bash
ab -t 60 -c 100 http://localhost:8080/
```

4线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.4      0      31
Processing:     2   21  12.1     19     312
Waiting:        2   21  12.0     19     312
Total:          4   21  12.1     19     312
```

16线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.6      0      38
Processing:     3   15   7.0     13     113
Waiting:        3   15   6.9     13     112
Total:          5   15   7.0     14     113
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.6      0      39
Processing:     3   14   7.8     13     129
Waiting:        2   12   6.8     11     103
Total:          4   15   7.8     13     129
```

---

**例子4：持续60秒，5000并发**
```bash
ab -t 60 -c 5000 http://localhost:8080/
```

4线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   11  35.6      0     171
Processing:    64 1132 255.8   1112    1803
Waiting:        5 1132 255.9   1112    1802
Total:        177 1144 240.3   1115    1803
```

16线程结果：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   10  30.0      0     152
Processing:    63  790 175.5    797    1697
Waiting:        6  790 175.5    797    1697
Total:        158  800 170.6    812    1779
```

16线程结果(sendfile 零拷贝)：
```
Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    6  20.9      0     131
Processing:    36  840 202.7    815    1678
Waiting:       14  838 202.5    812    1675
Total:        150  846 201.2    816    1749
```
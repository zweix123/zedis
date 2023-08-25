# zedis

Redis的Modern C++简单实现（现在看来还不够Modern），内容包括：简单的Socket编程、聚焦于数据结构、零散的功能比如TTL。

## Build

```bash
mkdir build
cd build
cmake ..
make
```

## Use

+ 服务端: `./build/src/server`
+ 客户端: `./build/src/client`前缀, 后接命令, 支持命令如下:
  ```
  set 键 值
  get 键
  del 键
  # 有序集
  zadd   有序集名 键 值
  zrem   有序集名 键 值  # 删除
  zscore 有序集名 键  # 查询
  zquery 有序集名 起始键 起始值 偏移量
  # TTL
  pexpire 键 TTL  # 设置存活时间, 键应该提前set, 时间单位为毫秒
  pttl    键      # 查看存活时间
  ```

## Develop

### 通信协议

客户端向服务端
+ 开始四个字节表示后面消息的字节数
+ 开始四个字节表示命令个数
+ 每个命令四个字节表示命令字节数

服务端向客户端
+ 开始四个字节表示后面消息的字节数
+ 消息使用TLV(type-length-value)格式

### 内存泄露

+ 检测使用
  ```bash
  valgrind -s --track-origins=yes --leak-check=full --tool=memcheck 可执行文件
  ```

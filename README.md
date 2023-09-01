# zedis

Redis的Modern C++实现，内容包括但不限于，简单的Socket编程，大量的侵入式数据结构实现，其他零散的Redis功能比如TTL。

>目前看来项目还是有点问题  
>一方面是项目文件结构，使用的[C++项目模板](https://github.com/Codesire-Deng/TemplateRepoCxx)不能处理自定义的库是声明和定义多文件的形式（而且我现阶段也没能力写出更好的）  
>另一方面是代码整体风格不统一，发心是Modern C++，但是侵入式的数据结构不太容易用智能指针管理，后续考虑用CRTP进行改良。
>>还是应该多看优秀的开源代码呀。

+ 项目需求，在学完《C++ Primer》之后，发现对很多C++11之后的特性和实践理解不够深，考虑在实践中进行更深的理解，而Web Server写的人太多了，接触到[《Build Your Own Redis with C/C++》](https://build-your-own.org/redis/)，调研后发现它的核心在于数据结构，有部分网络编程，同时也涉及Unix对多线程的API，虽然叫C/C++，其实更应该叫做C with vector and string，再考虑到Redis是很有名的开源软件，于是想根据这个资料将代码改写成Modern C++的mini Redis项目。

+ 网络部分：
  + 字节流封装，TCP是面向字节的，双方拿到字节流后要自行解析

## Build

```bash
mkdir build -p && cd build && cmake .. && make
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

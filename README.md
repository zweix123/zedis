# zedis

```bash
# 内存泄露检测
valgrind -s  --track-origins=yes --leak-check=full --tool=memcheck
```

## Protocol

对于request

+ 开始四个字节表示后面消息的字节数
+ 对于内容
  + 开始四个字节表示命令个数
    + 对于每个命令
      + 开始四个字节表示后面命令内容的字节数
      + 命令内容

对于reponse

+ 开始四个字节表示后面消息的字节数
  + 内容使用TLV(type-length-value)格式

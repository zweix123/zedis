import select
import subprocess
import sys

SERVER_NORMAL_ERR = (
    "msg /home/netease/Projects/zedis/include/connect.h:82 try_fill_buffer: EOF"
)
SERVER_ABNORMAL_ERR_PORT = ""


class AnsErr(Exception):
    def __init__(self, ans: str, output: str) -> None:
        super().__init__()
        self.ans = ans
        self.output = output

    def __str__(self) -> str:
        return f'对比错误\nans = """{self.ans}""", \noutput = """{self.output}"""'


class BindErr(Exception):
    def __init__(self, *args: object) -> None:
        super().__init__(*args)

    def __str__(self) -> str:
        return super().__str__()


server_proc = None
try:
    # 启动服务端并监控其输出
    server_proc = subprocess.Popen(
        ["../build/src/server"], stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )

    client_commands = [
        ["../build/src/client", "set", "key1", "value1"],
        ["../build/src/client", "get", "key1"],
        ["../build/src/client", "set", "key2", "value2"],
        ["../build/src/client", "get", "key2"],
        ["../build/src/client", "keys"],
        ["../build/src/client", "del", "key1"],
        ["../build/src/client", "del", "key2"],
    ]

    commands_answer = [
        "[nil]",
        "[str]: value1",
        "[nil]",
        "[str]: value2",
        "[arr]: len = 2\n  [str]: key2\n  [str]: key1",
        "[int]: 1",
        "[int]: 1",
    ]

    assert len(client_commands) == len(commands_answer)

    for cmd, ans in zip(client_commands, commands_answer):
        # 启动客户端(携带命令)并监控其输出
        client_proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        # 拿到输出
        # 注意这里的communicate会阻塞脚本的进程，等待子进程运行完毕
        client_output, client_error = client_proc.communicate()
        client_output_text = client_output.decode().strip()
        if client_output_text != ans:
            raise AnsErr(ans, client_output_text)

        # 下面要读取服务端的输出
        # 因为zedis的join的嘛, 所以不能干等着, 得使用IO复用的方式
        while True:
            ready_to_read, _, _ = select.select([server_proc.stderr], [], [], 0)
            if ready_to_read:
                server_output = server_proc.stderr.readline()
                if server_output:
                    server_output_text = server_output.decode().strip()
                    if server_output_text == SERVER_NORMAL_ERR:
                        pass
                    elif server_output_text == SERVER_ABNORMAL_ERR_PORT:
                        raise BindErr()
                    else:
                        server_output_text
                        raise Exception()
            else:
                break
    server_proc.terminate()
except (AnsErr, BindErr, Exception) as e:
    sys.stderr.write(str(e))
    server_proc.kill()

print("测试通过")

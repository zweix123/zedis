import os
import platform
import subprocess
import sys

current_platform = platform.system()
if current_platform != "Linux":
    print("该脚本只能运行在Linux上")
    sys.exit(1)


DIR_PRE = "../build/test/"


def run_exe(filepath: str):
    # 判断文件是否可执行
    if os.access(filepath, os.X_OK) and os.path.isfile(filepath):
        # 运行可执行文件，并捕获返回值和标准错误输出
        result = subprocess.run(
            filepath, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        # 判断返回值和标准错误输出是否为空
        if result.returncode != 0 or result.stderr:
            # 输出错误信息并退出程序
            sys.stderr.write(f"Error in {filepath}\n")
            sys.exit(1)


def run_py(filepath: str):
    # 判断文件是否为 Python 文件
    if os.path.splitext(filepath)[-1] == ".py":
        # 运行 Python 文件，并捕获返回值和标准错误输出
        result = subprocess.run(
            ["python3", filepath], stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        # 判断返回值和标准错误输出是否为空
        if result.returncode != 0 or result.stderr:
            # 输出错误信息并退出程序
            sys.stderr.write(f"Error in {filepath}\n")
            sys.exit(1)
    else:
        assert False


for filename in os.listdir("."):
    if filename.endswith("txt") or filename == os.path.basename(__file__):
        continue
    elif filename.endswith("py"):
        run_py(filename)
    elif filename.endswith("cpp"):
        filepath = os.path.join(DIR_PRE, filename[:-4])
        run_exe(filepath)
    else:
        assert False
    print(filename + " ok")

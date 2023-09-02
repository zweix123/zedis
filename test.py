import subprocess

# Start server as a subprocess
server_process = subprocess.Popen(["./build/src/server"])

# Define list of client commands to execute
client_commands = [
    ["./build/src/client", "SET", "key1", "value1"],
    # ["./build/src/client", "GET", "key1"],
    # ["./build/src/client", "SET", "key2", "value2"],
    # ["./build/src/client", "GET", "key2"],
    # ["./build/src/client", "DEL", "key1"],
    # ["./build/src/client", "DEL", "key2"],
]

try:
    # Execute client commands
    for cmd in client_commands:
        client_process = subprocess.Popen([cmd])
        client_process.wait()

    # Shutdown server
    server_process.terminate()
except Exception as e:
    server_process.kill()
    # try:

    #     server_process.wait(timeout=5)
    # except subprocess.TimeoutExpired:
    #     server_process.kill()

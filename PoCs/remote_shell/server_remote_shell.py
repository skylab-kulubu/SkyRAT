# Remote Shell Server Module
# Sends commands to client and prints output
import socket

SERVER_IP = '0.0.0.0'
SERVER_PORT = 4445
BUFFER_SIZE = 4096

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((SERVER_IP, SERVER_PORT))
    s.listen(1)
    print(f"[+] Listening on {SERVER_IP}:{SERVER_PORT}")
    conn, addr = s.accept()
    with conn:
        print(f"[+] Connection from {addr}")
        while True:
            cmd = input("Shell> ")
            if not cmd:
                continue
            conn.sendall(cmd.encode())
            if cmd == 'exit':
                break
            output = b''
            while True:
                data = conn.recv(BUFFER_SIZE)
                if b'<END_OF_OUTPUT>' in data or not data:
                    output += data.replace(b'<END_OF_OUTPUT>\n', b'')
                    break
                output += data
            print(output.decode(errors='ignore'))
    print("[+] Connection closed.")

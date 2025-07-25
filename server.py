import socket
import threading
from os import getenv

def handle_client(client_socket, addr):
    print(f"Connection from {addr}")
    recv_size = int(getenv("RECV_SIZE","1024"))
    encode = str(getenv("ENCODE","utf-8"))
    try:
        while True:
            data = client_socket.recv(recv_size)
            if not data:
                break
            message = data.decode(encode)
            print(f"Received from {addr}: {message}")
    except Exception as e:
        print(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        print(f"Connection with {addr} closed")

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    print("""\
    
    ___          ___    _  _____
  ,' _/ /7 _ __ / o | .' \/_  _/
 _\ `. //_7\V //  ,' / o / / /  
/___,'//\\\\  )//_/`_\/_n_/ /_/   
           //                   

    """)
    
    host = getenv("LHOST","localhost")
    port = getenv("LPORT","1911")
    try:
        port = int(port)
    except ValueError:
        print("Invalid port number. Please enter a valid integer.")
        return
    if port < 1024:
        print("You must start the program as root to use port 1024 and gold.")
        print("If the program does not work, change the port number to a value greater than 1024.")
    elif port > 65535:
        print("Port number must lower then 65535")
    server.bind((host, port))
    server.listen(5)
    print(f"TCP Server listening on {host}:{port}")

    try:
        while True:
            client_socket, addr = server.accept()
            client_thread = threading.Thread(target=handle_client, args=(client_socket, addr))
            client_thread.daemon = True
            client_thread.start()
    except KeyboardInterrupt:
        print("\nServer shutting down...")
    finally:
        server.close()

def help:

def keylogger:


def cli(args):
    command_list = {"help", "keylogger"}
    if args not in command_list:
        print("Available commands: " + command_list)
    else:
        for i in range(len(command_list)):
            if args == command_list[i]:
                # todo
                





if __name__ == "__main__":
    start_server()

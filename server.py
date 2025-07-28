import socket
import threading

OUT_FILE="output.txt"
def handle_client(client_socket, addr):
    print(f"Connection from {addr}")
    try:
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            message = data.decode('utf-8')
            print(f"Received from {addr}: {message}")
    except Exception as e:
        print(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        print(f"Connection with {addr} closed")

def out_new_line(data:str,outfile:str="output.txt"):
    with open(outfile,"a") as file:
        file.write(f"{data}\n")

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
    
    host = 'localhost'
    port = input("Enter port number to start the server: ")
    try:
        port = int(port)
    except ValueError:
        print("Invalid port number. Please enter a valid integer.")
        return
    if port < 1024 or port > 65535:
        print("Port number must be between 1024 and 65535.")
        return
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

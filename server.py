import socket
import threading

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

def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    host = 'localhost'
    port = 27015
    
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

if __name__ == "__main__":
    start_server()
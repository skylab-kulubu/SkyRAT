import socket
import time
from os import getenv
from dotenv import load_dotenv
from logger import get_logger
load_dotenv()
RHOST=str(getenv("RHOST","127.0.0.1"))
RPORT=int(getenv("RPORT","1911"))
MESSAGES=getenv("MESSAGES",None)
DELAY=int(getenv("DELAY",1))
ENCODING=str(getenv("ENCODING","utf-8"))
"""
RHOST="127.0.0.1"
RPORT=1911
MESSAGES="hello from world, omg its working, hi"
DELAY=1
ENCODING=utf-8
"""
def test_client(rhost:str=RHOST, 
                rport:int=RPORT, 
                messages=MESSAGES, 
                delay:int=DELAY,
                encoding:str=ENCODING,
                logger=get_logger()):
    if messages is None:
        messages = [
            "Hello, server!",
            "This is a test message.",
            "Goodbye!"
        ]
    else:
        messages = messages.split(", ")
    try:
        with socket.create_connection((rhost, rport)) as sock:
            for msg in messages:
                logger.info(f"Sending: {msg}")
                sock.sendall(msg.encode(encoding))
                time.sleep(delay)
    except Exception as e:
        logger.error(f"Client error: {e}")

if __name__ == "__main__":
    test_client()

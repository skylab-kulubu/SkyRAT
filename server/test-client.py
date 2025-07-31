import socket
import time
from os import getenv
from dotenv import load_dotenv
from logger import get_logger
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
import base64

load_dotenv()
RHOST=str(getenv("RHOST","127.0.0.1"))
RPORT=int(getenv("RPORT","1911"))
MESSAGES=getenv("MESSAGES",None)
DELAY=int(getenv("DELAY",1))
ENCODING=str(getenv("ENCODING","utf-8"))
PUBLIC_KEY_PATH=str(getenv("PUBLIC_KEY_PATH","public.pem"))
"""
RHOST="127.0.0.1"
RPORT=1911
MESSAGES="hello from world, omg its working, hi"
DELAY=1
ENCODING=utf-8
"""

def get_cipher_rsa(public_key_path:str=PUBLIC_KEY_PATH):
    with open(public_key_path, "rb") as f:
        server_pub_key = RSA.import_key(f.read())
    cipher_rsa = PKCS1_OAEP.new(server_pub_key)
    return cipher_rsa

cipher_rsa=get_cipher_rsa()

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
                encrypted = cipher_rsa.encrypt(msg.encode(encoding))
                sock.sendall(base64.b64encode(encrypted))
                time.sleep(delay)
    except Exception as e:
        logger.error(f"Client error: {e}")

if __name__ == "__main__":
    test_client()

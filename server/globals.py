from os import getenv, getcwd
LHOST = str(getenv("LHOST", "localhost"))
LPORT = int(getenv("LPORT", "1911"))
RECV_SIZE = int(getenv("RECV_SIZE", "1024"))
ENCODING = str(getenv("ENCODING", "utf-8"))
OUT_FILE = str(getenv("OUT_FILE", "output"))
OUTPUT_FORMAT = str(getenv("OUTPUT_FORMAT", "CLF")
                    )  # Options: CLF, JSON (to-do)
"""
CLF Log Format Sample
192.168.122.16 - john [10/Oct/2000:13:55:36 -0700] "PRESS D" 768
<src_addr> - <user or agent name> <time_stamp> <action> <bytes_sent>

JSON Log Format Sample
TO-DO
"""

OUTPUT_TIMEZONE = str(getenv("OUTPUT_TIMEZONE", "UTC"))
PROMPT = getenv("PROMPT", "$ ")
KEY_DIR = str(getenv("KEY_DIR", f"{getcwd()}/keys"))
PRIVATE_KEY_NAME = str(getenv("PRIVATE_KEY_NAME", None))
PUBLIC_KEY_NAME = str(getenv("PUBLIC_KEY_NAME", None))
TLS_ENABLED = getenv("TLS", False)
AGENTS_JSON = getenv("AGENTS_JSON", f"{getcwd()}/agents.json")
RSA_KEY_SIZE = int(getenv("RSA_KEY_SIZE", 2048))
PRIVATE_KEY_NAME = str(getenv("PRIVATE_KEY_NAME", None))
PUBLIC_KEY_NAME = str(getenv("PUBLIC_KEY_NAME", None))
KEY_DIR = str(getenv("KEY_DIR", f"{getcwd()}/keys"))

from Crypto.PublicKey import RSA
from os import getenv, getcwd
from dotenv import load_dotenv
from logger import get_logger 

logger=get_logger()
load_dotenv()
RSA_KEY_SIZE=int(getenv("RSA_KEY_SIZE",2048))
PRIVATE_KEY_PATH=str(getenv("PRIVATE_KEY_NAME","private.pem"))
PUBLIC_KEY_PATH=str(getenv("PUBLIC_KEY_PATH","public.pem"))
logger.debug(f"""
RSA_KEY_SIZE={RSA_KEY_SIZE}
PRIVATE_KEY_PATH={getcwd()}/{PRIVATE_KEY_PATH}
PUBLIC_KEY_PATH={getcwd()}/{PUBLIC_KEY_PATH}
""")
def generate_key_pair(
    rsa_key_size:int=RSA_KEY_SIZE,
    private_key_path:str=PRIVATE_KEY_PATH,
    public_key_path:str=PUBLIC_KEY_PATH
):
    key = RSA.generate(rsa_key_size)

    private_key = key.export_key()
    public_key = key.publickey().export_key()

    with open(private_key_path, "wb") as f:

        f.write(private_key)

    with open(public_key_path, "wb") as f:
        f.write(public_key)

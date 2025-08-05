from Crypto.PublicKey import RSA
from os import getenv, getcwd, environ, makedirs, path
from dotenv import load_dotenv
from logger import get_logger
from globals import RSA_KEY_SIZE, PRIVATE_KEY_NAME, PUBLIC_KEY_NAME, KEY_DIR
logger = get_logger()
load_dotenv()
logger.debug(f"""
RSA_KEY_SIZE={RSA_KEY_SIZE}
PRIVATE_KEY_NAME={PRIVATE_KEY_NAME}
PUBLIC_KEY_NAME={PUBLIC_KEY_NAME}
KEY_DIR={KEY_DIR}
""")


def generate_key_pair(
    rsa_key_size: int = RSA_KEY_SIZE,
    private_key_path: str = PRIVATE_KEY_NAME,
    public_key_name: str = PUBLIC_KEY_NAME,
    key_dir: str = KEY_DIR
):
    private_key_ = private_key_path
    logger.debug(f"private_key_={private_key_}")
    public_key_ = public_key_name
    logger.debug(f"public_key_={public_key_}")
    if str(private_key_) == str(None):
        private_key_ = str(input("Private key path [private.pem]:"))
        if private_key_.strip() == "":
            private_key_ = "private.pem"
    logger.debug(f"Private key file: {private_key_}")
    if str(public_key_) == str(None):
        public_key_ = str(input("Private key path [public.pem]:"))
        if public_key_.strip() == "":
            public_key_ = "public.pem"
    logger.debug(f"Public key file: {public_key_}")
    key = RSA.generate(rsa_key_size)
    environ["PRIVATE_KEY_NAME"] = private_key_
    logger.debug(f"PRIVATE_KEY_NAME setted to {private_key_}")
    environ["PUBLIC_KEY_NAME"] = public_key_
    logger.debug(f"PUBLIC_KEY_NAME setted to {public_key_}")
    environ["TLS"] = str(True)
    logger.debug("TLS setted to True")
    private_key = key.export_key()
    public_key = key.publickey().export_key()
    private_key_literal_path = f"{key_dir}/{private_key_}"
    public_key_literal_path = f"{key_dir}/{public_key_}"

    if not path.exists(key_dir):
        makedirs(key_dir)
        logger.debug(f"{key_dir} created.")

    with open(private_key_literal_path, "wb") as f:
        logger.debug(f"Private key literal path: {private_key_literal_path}")
        f.write(private_key)

    with open(public_key_literal_path, "wb") as f:
        logger.debug(f"Public key literal path: {public_key_literal_path}")
        f.write(public_key)
    return private_key_, public_key_

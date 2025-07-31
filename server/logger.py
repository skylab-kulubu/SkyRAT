from os import getenv
from dotenv import load_dotenv
import logging

load_dotenv()

# Logging
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
    if not logger.hasHandlers():
        console_handler = logging.StreamHandler()
        console_handler.setLevel(numeric_level)
        formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)
    return logger


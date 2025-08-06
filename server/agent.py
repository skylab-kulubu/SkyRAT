import json
import os
from logger import get_logger
from socket import socket
logger = get_logger()


class Agent:
    def __init__(self, addr: str,
                 roles: list,
                 json_file: str,
                 socket: socket
                 ) -> None:
        self.addr = addr
        self.roles = roles
        self.json_file = json_file
        self.socket = socket
        self._save_to_json()

    def to_dict(self) -> dict:
        """Convert object to dictionary"""
        return {
            "addr": self.addr,
            "roles": self.roles
        }

    def _save_to_json(self) -> None:
        """Write object to JSON file"""
        data = []
        if os.path.exists(self.json_file):
            with open(self.json_file, 'r') as f:
                try:
                    data = json.load(f)
                except:
                    data = []
        data.append(self.to_dict())

        if self.to_dict() in data:
            logger.debug(f"{self.addr} already exists in agents.")
        else:
            logger.info(
                f"{self.addr} doesn't exist in agents, adding it to JSON.")
            with open(self.json_file, 'w') as f:
                json.dump(data, f, ensure_ascii=False)

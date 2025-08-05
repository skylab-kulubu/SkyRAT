from agent import Agent
from socket import socket
import base64
from logger import get_logger
from Crypto.Cipher import PKCS1_OAEP
import msgpack

from globals import AGENTS_JSON, ENCODING, RECV_SIZE, TLS_ENABLED


class AgentTool:
    """A class for agent utilties"""

    def __init__(self, agents: dict[str,socket] = {}) -> None:
        self.agents = agents

    def add_agent(self, agent: Agent, conn: socket):
        """Add agent to agents dictionary object"""
        self.agents[agent.addr] = conn

    def del_agent(self, agent: Agent):
        """Remove agent from agents dictionary object"""
        del self.agents[agent.addr]

    def get_agents(self):
        """Remove agent object from agetns dictionary object"""
        return self.agents

    def get_agent_by_rhost(self, rhost: str):
        """Get agent by remote addr"""
        return self.agents[rhost]

    def print_agents_table(self):
        """Agents table
        TO-DO
        """
        for agent in self.agents.keys():
            print(agent)

    def broadcast_msg(self):
        """Send message to all agents
        TO-DO
        """
        pass

    def send_msg(self, socket: socket, msg: str):
        """Send message to an agent"""
        socket.sendall(msg.encode())

    def send_msg_by_rhost(self, rhost: str, msg: str):
        """Send message to an agent by remote addr"""
        socket = self.get_agent_by_rhost(rhost)
        self.send_msg(socket, msg)

    # connection handler
    def handle_client(self, client_socket: socket,
                      addr,
                      rsa_chipher: PKCS1_OAEP.PKCS1OAEP_Cipher|None,
                      encoding: str = ENCODING,
                      recv_size: int = RECV_SIZE,
                      tls_enabled:bool|str =  TLS_ENABLED,
                      agents_json: str = AGENTS_JSON,
                      logger=get_logger(),
                      ):
        logger.info(f"\nConnection from {addr}")
        agent = Agent(str(addr), ["agent"], agents_json)
        
        # Add agent to the agents dictionary
        self.add_agent(agent, client_socket)
        
        try:
            while True:
                message = None
                data = client_socket.recv(recv_size)
                if not data:
                    break
                logger.debug(f"DATA SIZE = {len(data)}")
                if tls_enabled and tls_enabled != "False" and rsa_chipher is not None:
                    try:
                        decrypted = rsa_chipher.decrypt(base64.b64decode(data))
                        logger.debug(f"decrypted={decrypted}")
                        deserialized_data = msgpack.loads(decrypted)
                        message = deserialized_data[0]["content"]
                    except Exception as e:
                        logger.error(f"RSA decrypt error: {e}")
                else:
                    deserialized_data = msgpack.loads(data)
                    message = data[0]["content"]
                logger.info(f"Received from {addr}: {message}\n")
        except Exception as e:
            logger.error(f"Error with client {addr}: {e}")
        finally:
            # Remove agent from dictionary when connection closes
            self.del_agent(agent)
            client_socket.close()
            logger.info(f"Connection with {addr} closed")

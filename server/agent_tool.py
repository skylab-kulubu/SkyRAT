from agent import Agent
from socket import socket
class AgentTool:
    """A class for agent utilties"""

    def __init__(self,agents:dict={}) -> None:
       self.agents=agents 

    def add_agent(self,agent:Agent,conn:socket):
        """Add agent to agents dictionary object"""
        self.agents[agent.addr]=conn

    def del_agent(self,agent:Agent):
        """Remove agent from agents dictionary object"""
        del self.agents[agent.addr]
    
    def get_agents(self):
        """Remove agent object from agetns dictionary object"""
        return self.agents

    def get_agent_by_rhost(self,rhost:str):
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

    def send_msg(self,socket:socket,msg:str):
        """Send message to an agent"""
        socket.sendall(msg.encode())

    def send_msg_by_rhost(self,rhost:str,msg:str):
        """Send message to an agent by remote addr"""
        socket = self.get_agent_by_rhost(rhost)
        self.send_msg(socket,msg)

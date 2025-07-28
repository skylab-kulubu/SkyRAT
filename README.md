# ⚠️ Disclaimer

This project is intended **solely for educational and research purposes**.  
It simulates the behavior of Remote Access Trojans (RATs) to explore malware detection, system vulnerabilities, and defensive strategies in controlled environments.

**Do not use this project on unauthorized systems or in production environments.**  
Misuse may violate laws and ethical standards, and could result in severe legal consequences.  
The author assumes **no responsibility** for any damage or misuse of the information provided.

---

# RAT Project Scope

## Actions Possible on the Compromised Device

- Keylogging  
- Screenshot  
  - Webcam access  
- Data download / upload  
- Credential harvesting  
  - Web  
  - WiFi  
  - Computer  
- Remote Shell / Access  
- Privilege Escalation  
- Persistence (Windows Registry operations)  
- Disabling antivirus software  

---

## General Structure of the Project

- The project will be written in **C++**.  
- Separate code will be written for the RAT server and client:  
  - Server program will target **Linux**  
  - Client program will target **Windows**  
- On the client side, a shell subprocess will be created using `CreateProcess()`.  
  - Commands received from the server will be processed in the main process and passed to the shell subprocess.  
  - The results will be sent back to the server.  
- **WinSock2.h** and **OpenSSL** libraries will be used for communication.  
  - Alternatively, instead of OpenSSL, manual encryption with a predefined **public key** may be used.  
- After achieving basic shell I/O and socket communication, special modules will be developed for additional features:  
  - Keylogger module  
  - Screenshot module  
  - AV disabler module  
  - etc.  

---

## Server Side

- The server will operate via a CLI (Command Line Interface).  
- Desired commands and input rules will be defined.  
- A **parsing algorithm** will be written to handle command processing.  

---

## Optional Features

- A **stager program** can be developed to deliver the main RAT program.  
- A **payload (shellcode)** can be created to deliver the stager program.

---
## Authors
- [Mertcan Yavaşoğlu](https://github.com/MertcanYavasoglu)
- [Nilay Karamustafaoğlu](https://github.com/NilayKaramustafaoglu0)
- [Yusuf Emir Gökdoğan](https://github.com/ygokdogan)
- [İzzet Mammadzada](https://github.com/coduronin)

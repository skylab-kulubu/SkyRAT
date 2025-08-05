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

### Enviroment Variables

| Variable Name      | Default Value              | Description                                                        | Used In File(s)                                  |
| ------------------ | -------------------------- | ------------------------------------------------------------------ | ------------------------------------------------ |
| `LHOST`            | `"localhost"`              | IP address the server will bind to                                 | `server.py`                                      |
| `LPORT`            | `"1911"`                   | Port number the server will listen on                              | `server.py`                                      |
| `RECV_SIZE`        | `"1024"`                   | Buffer size in bytes for receiving data via socket                 | `server.py`                                      |
| `ENCODING`         | `"utf-8"`                  | Character encoding used for socket communication                   | `server.py`, `test-client.py`                    |
| `OUT_FILE`         | `"output"`                 | Filename where output logs will be written                         | `server.py`                                      |
| `OUTPUT_FORMAT`    | `"CLF"`                    | Output log format (e.g., CLF = Common Log Format)                  | `server.py`                                      |
| `OUTPUT_TIMEZONE`  | `"UTC"`                    | Timezone used for timestamps in logs                               | `server.py`                                      |
| `PROMPT`           | `"$ "`                     | Command prompt string shown to users                               | `server.py`                                      |
| `KEY_DIR`          | `"{getcwd()}/keys"`        | Directory where key files are stored                               | `server.py`, `generate-key.py`                   |
| `PRIVATE_KEY_NAME` | `None`                     | Path to the RSA private key file (optional override)               | `server.py`, `generate-key.py`                   |
| `PUBLIC_KEY_NAME`  | `"public.pem"` or `None`   | Path to the RSA public key file (optional override)                | `server.py`, `generate-key.py`, `test-client.py` |
| `TLS_ENABLED`      | `False`                    | Flag indicating whether to enable TLS encryption                   | `server.py`                                      |
| `AGENTS_JSON`      | `"{getcwd()}/agents.json"` | Path to JSON file storing agent metadata                           | `server.py`                                      |
| `RSA_KEY_SIZE`     | `2048`                     | Bit length for the generated RSA key pair                          | `generate-key.py`                                |
| `RHOST`            | `"127.0.0.1"`              | Remote host IP the client connects to                              | `test-client.py`                                 |
| `RPORT`            | `"1911"`                   | Remote port the client connects to                                 | `test-client.py`                                 |
| `MESSAGES`         | `None`                     | Optional pre-defined messages sent by the client (comma-separated) | `test-client.py`                                 |
| `DELAY`            | `1`                        | Delay between messages sent by the client (in seconds)             | `test-client.py`                                 |

### Podman/Docker Quick Start

#### Podman

```bash
podman build -t skyRAT-server .
podman run --rm -it -p 4545:4545 \
-e LHOST='0.0.0.0' \
-e LPORT=4545 \
-e PRIVATE_KEY_NAME=keyname.key \
-e TLS=True \
-v ./keys:./keys \
localhost/skyRAT-server
```

#### Docker

```bash
docker build -t skyRAT-server .
docker run --rm -it -p 4545:4545 \
-e LHOST='0.0.0.0' \
-e LPORT=4545 \
-e PRIVATE_KEY_NAME=keyname.key \
-e TLS=True \
-v ./keys:./keys \
skyRAT-server
```

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
- [Ömer Faruk Erdem](https://github.com/farukerdem34)

## API References

### Screenshoot

#### Client -> Server

```json
{
  "type": "screenshoot",
  "content": "base64-encoded-picture-bytes"
}
```

#### Server -> Client

```json
{
  "type": "screenshoot"
}
```

### Keylogger

#### Client -> Server

```json
{
  "type": "keylogger",
  "content": "utf-8-keystroke-data"
}
```


## Building 

### Installing Buildah 

#### MacOs & NixOS

```bash
nix-env -iA buildah
```

#### Other Linux Distros

Fedora
```bash
sudo dnf install buildah
```

Ubuntu/Debian based distros
```bash 
sudo apt update
sudo apt install buildah
```

CentOS
```bash 
sudo yum install epel-release
sudo yum install buildah
```

--- 

```bash
IMAGE_NAME="quay.io/skylab/skyrat-server:latest"
buildah build --jobs "$(nproc)" -t "${IMAGE_NAME}" "${PWD}"
```

## Using Built Image in Docker 

```bash 
IMAGE_NAME="quay.io/skylab/skyrat-server"
buildah push "${IMAGE_NAME}" docker-daemon:"${IMAGE_NAME}"
docker image list | grep "${IMAGE_NAME}" || echo "Something might went wrong while building image"
```

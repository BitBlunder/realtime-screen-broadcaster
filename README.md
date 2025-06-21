# Real-Time Screen Broadcaster

**Real-Time Screen Broadcaster** is a project for capturing a Windows desktop screen in real time and broadcasting the frames as MJPEG stream to multiple web viewers. It consists of a Windows producer written in C++ and a Python WebSocket server with an HTML/JavaScript frontend.

## Project structure

```
producer/      # Windows C++ applications
├─ common/     # shared utilities (logging, win32 helpers…)
├─ dropper/    # helper used to deploy the recorder
└─ recorder/   # screen capture + WebSocket client

server/        # Python Flask+WebSocket server and web UI
```

- **recorder** – captures the screen using the DXGI Desktop Duplication API (with a BitBlt fallback) and spawns FFmpeg to encode frames to MJPEG. Frames are sent over WebSocket to the server.
- **dropper** – extracts the recorder and its resources (FFmpeg binaries) and can launch it. It relies on libarchive to unpack a bundled archive.
- **server** – Flask application using `flask_sock` and `gevent` to receive frames from a single producer and broadcast them to any number of viewers. The web UI is served from `server/templates` and `server/static`.

## How it works

1. The **recorder** connects to the server via WebSocket and registers as the producer.
2. Captured frames are piped to FFmpeg which compresses them to JPEG. The resulting frames are forwarded over the WebSocket connection.
3. The **server** keeps the latest frames in a queue and sends them to connected viewers. Viewers use JavaScript to display the MJPEG frames on a canvas element.
4. Viewers can send a `stop` command which the server relays back to the producer to end the stream.

## Architecture

```
+-------------+       WebSocket     +---------------+       WebSocket      +----------+
|  Recorder   | <-----------------> |    Server     | <------------------> |  Viewer  |
+-------------+                     +---------------+                      +----------+
       ^                                    ^
       | FFmpeg pipe (MJPEG)                |
       +------------------------------------+
```

- **Recorder** (C++)
  - `capture.cpp` – grabs the desktop using DXGI. Falls back to GDI BitBlt when needed.
  - `ffmpeg_pipe.cpp` – spawns FFmpeg and communicates through stdin/stdout pipes.
  - `ws_client.cpp` – minimal WebSocket client based on `asio` and `websocketpp`.
  - `usb_watcher.cpp` – optional USB event watcher used by the application.
- **Server** (Python)
  - `app.py` – Flask application with a `Sock` WebSocket endpoint. Manages one producer and multiple viewers, buffering frames in a queue and forwarding them.
  - `static/` and `templates/` – front‑end assets.

## Building (MSYS2 UCRT + CMake)

The C++ components are intended to be built with the MSYS2 UCRT64 environment.

1. Install [MSYS2](https://www.msys2.org/) and open the **UCRT64** shell.
2. Install the required packages:
   ```bash
   pacman -S git mingw-w64-ucrt-x86_64-toolchain \
              mingw-w64-ucrt-x86_64-cmake \
              mingw-w64-ucrt-x86_64-ninja
   ```
3. Clone this repository and create a build directory:
   ```bash
   git clone <this repo>
   cd realtime-screen-broadcaster
   cmake -S producer -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```
4. Build the applications:
   ```bash
   cmake --build build --config Release
   ```
   The resulting binaries (`turtle-treasure-hunt.exe` and `launcher.exe`) will be in `build/bin`.

### Running the server

The server requires Python 3 with a few packages:

```bash
pip install flask flask_sock gevent simple-websocket
```

Then run:

```bash
cd server
python app.py
```

Visit `http://localhost:5000/` in a browser to view the stream. The producer must connect to `ws://<server-ip>:5000/ws`.

## Notes

- FFmpeg binaries expected by the recorder are provided in `producer/recorder/ffmpeg`.
- All third‑party libraries (asio, websocketpp, libarchive) are included under `producer/external` and built automatically by CMake.
- Configuration for the recorder is read from `producer/recorder/config.ini`.

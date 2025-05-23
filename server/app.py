import secrets
import socket
import ssl

import gevent, flask, flask_sock

from typing import Optional, Dict
import gevent.monkey
from gevent.queue import Queue
from simple_websocket import Server

# --- Apply gevent monkey-patching for cooperative I/O ---
gevent.monkey.patch_all()

class StreamManager:
	"""
	Encapsulates producer, viewers, and frame queue state.
	"""
	def __init__(self, queue_size: int = 4):
		self.viewers: Dict[str, Server] = {}

		self.producer_id: Optional[str] = None
		self.producer_ws: Optional[Server] = None

		self.frame_queue: Queue = Queue(maxsize=queue_size)

	def gen_id(self) -> str:
		"""Generate a random 16-char session ID."""

		return secrets.token_hex(8)


	def register_viewer(self, id: str, ws: Server) -> None:
		"""Add a viewer to the broadcast list."""

		self.viewers[id] = ws

		print(f"Viewer connected: SID={id} (total={len(self.viewers)})")

	def register_producer(self, id: str, ws: Server) -> bool:
		"""Attempt to register a new producer; return True if successful."""

		if self.producer_id:
			return False

		self.producer_id = id
		self.producer_ws = ws

		print(f"Registered producer: SID={id}")

		return True

	def unregister_viewer(self, id: str) -> None:
		"""Remove a viewer from the broadcast list."""

		self.viewers.pop(id, None)

		print(f"Viewer disconnected: SID={id} (total={len(self.viewers)})")

	def unregister_producer(self) -> None:
		"""Clean up producer state and notify viewers of stream end."""

		print(f"Unregistering producer: SID={self.producer_id}")

		self.producer_id = None
		self.producer_ws = None

		for id, ws in list(self.viewers.items()):
			try:
				ws.send("stream_ended")
			except Exception:
				pass

		print("Notified viewers of stream end.")


	def relay_stop(self) -> None:
		"""Forward a 'stop' command to the producer if connected."""
		if self.producer_ws:
			try:
				self.producer_ws.send("stop")
			except Exception:
				pass

	def add_frame(self, data: bytes) -> None:
		"""Put a new frame in the queue if space is available."""

		if not self.frame_queue.full():
			self.frame_queue.put_nowait(data)

	def sender_loop(self) -> None:
		"""Continuously send frames from the queue to all connected viewers."""

		while True:
			stale = []
			frame = self.frame_queue.get()

			for id, ws in self.viewers.items():
				try:
					ws.send(frame)
				except Exception:
					stale.append(id)
			for id in stale:
				self.unregister_viewer(id)

class StreamingApp:
	def __init__(self):
		self.app = flask.Flask(__name__)
		self.sock = flask_sock.Sock(self.app)
		self.manager = StreamManager(queue_size=4)

		gevent.spawn(self.manager.sender_loop)

		self._register_routes()

	def _register_routes(self) -> None:
		@self.app.get('/')
		def index():
			return flask.render_template('index.html')

		@self.sock.route('/ws')
		def ws_handler(ws: Server):
			# Disable Nagle on this connection
			raw_sock = ws.sock if hasattr(ws, 'sock') else None
			raw_sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

			id = self.manager.gen_id()

			first = ws.receive()
			print(f"First message from SID={id}: {first}")

			# Producer flow
			if isinstance(first, str) and first == 'register_producer':
				if not self.manager.register_producer(id, ws):
					ws.send('error: producer_exists')
					ws.close()
					return

				try:
					while True:
						data = ws.receive()
						if data is None:
							break

						self.manager.add_frame(data)
				finally:
					self.manager.unregister_producer()

			# Viewer flow
			elif isinstance(first, str) and first == 'register_viewer':
				self.manager.register_viewer(id, ws)

				try:
					while True:
						msg = ws.receive()
						if msg is None:
							break

						if isinstance(msg, str) and msg.strip().lower() == 'stop':
							self.manager.relay_stop()
				finally:
					self.manager.unregister_viewer(id)

	def run(self, host: str = '0.0.0.0', port: int = 443, debug: bool = True) -> None:
		# SSL context setup
		context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
		# Assumes server.crt and server.key are in the parent directory of app.py
		cert_file = '../server.crt' # Adjusted path
		key_file = '../server.key'   # Adjusted path
		try:
			context.load_cert_chain(cert_file, key_file)
			print(f"Successfully loaded SSL certificate from {cert_file} and key from {key_file}")
		except FileNotFoundError:
			print(f"Error: Could not find SSL certificate ({cert_file}) or key ({key_file}). Ensure they are in the correct location.")
			return
		except ssl.SSLError as e:
			print(f"Error loading SSL certificate or key: {e}")
			return

		from gevent.pywsgi import WSGIServer
		# Run WSS server with SSL context
		http_server = WSGIServer((host, port), self.app, keyfile=key_file, certfile=cert_file)
		print(f"Starting WSS server on https://{host}:{port}")
		http_server.serve_forever()


if __name__ == '__main__':
	app = StreamingApp()
	app.run()

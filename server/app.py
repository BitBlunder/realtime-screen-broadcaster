from gevent import monkey, spawn
from gevent.queue import Queue
from flask import Flask, render_template
from flask_sock import Sock
import secrets

# Patch standard library for cooperative sockets/timers
monkey.patch_all()

app = Flask(__name__)
sock = Sock(app)

# ─────────────────── internal state ──────────────────────────────────
producer_sid: str | None    = None            # SID of single producer
producer_ws                = None            # its WebSocket object
viewers: dict[str, object] = {}              # SID → WebSocket

# Shared frame queue (max 4 frames)
frame_q = Queue(maxsize=4)

# ──────────────────── helpers ───────────────────────────────────────

def gen_sid() -> str:
	"""Generate a random 16-char session id."""
	return secrets.token_hex(8)


def notify_viewers_stream_end():
	for sid, ws in list(viewers.items()):
		try:
			ws.send("stream_ended")
		except Exception:
			viewers.pop(sid, None)


def relay_stop_to_producer():
	if producer_ws:
		try:
			producer_ws.send("stop")
		except Exception:
			pass

# ─────────────────── sender greenlet ────────────────────────────────
def sender():
	"""Continuously send latest frames to all viewers."""
	while True:
		jpg = frame_q.get()  # block until a frame is ready
		stale = []
		for sid, ws in viewers.items():
			try:
				ws.send(jpg)  # bytes -> binary frame
			except Exception:
				stale.append(sid)
		for sid in stale:
			viewers.pop(sid, None)

# start sender once
spawn(sender)

# ─────────────────── HTTP template route ─────────────────────────────
@app.get("/")
def index():
	return render_template("index.html")

# ─────────────────── WebSocket endpoint ──────────────────────────────
@sock.route("/ws")
def ws_handler(ws):
	global producer_sid, producer_ws

	sid = gen_sid()
	first = ws.receive()

	print(f"First message from SID={sid!r}: {first!r}")

	# producer handshake
	if isinstance(first, str) and first == "register_producer":
		if producer_sid:
			ws.send("error: producer_exists")
			ws.close()
			return
		producer_sid, producer_ws = sid, ws
		print(f"[producer] connected SID={sid}")
		try:
			while True:
				data = ws.receive()
				if data is None:
					break
				# queue frame, drop if full
				if not frame_q.full():
					frame_q.put_nowait(data)
		finally:
			print("[producer] disconnected")
			producer_sid, producer_ws = None, None
			notify_viewers_stream_end()
	else:
		# viewer branch
		viewers[sid] = ws
		print(f"[viewer] connected SID={sid} total={len(viewers)}")
		# initial stop request
		if isinstance(first, str) and first.strip().lower() == "stop":
			relay_stop_to_producer()
		try:
			while True:
				msg = ws.receive()
				if msg is None:
					break
				if isinstance(msg, str) and msg.strip().lower() == "stop":
					relay_stop_to_producer()
		finally:
			viewers.pop(sid, None)
			print(f"[viewer] disconnected SID={sid} total={len(viewers)}")

# ─────────────────── entrypoint ──────────────────────────────────────
if __name__ == "__main__":
	app.run(host="0.0.0.0", port=80, debug=True)
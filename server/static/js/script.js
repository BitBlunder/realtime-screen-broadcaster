// grab elements
const canvas       = document.getElementById('video-canvas');
const placeholder  = document.getElementById('placeholder');
const stopBtn      = document.getElementById('stop-button');
const statusEl     = document.getElementById('status');
const resEl        = document.getElementById('resolution');
const fpsEl        = document.getElementById('framerate');
const durEl        = document.getElementById('duration');
const connEl       = document.getElementById('connection');
const fullscreenBtn= document.getElementById('fullscreen-button');
const wrapper      = document.getElementById('stream-wrapper');

const ctx = canvas.getContext('2d');

let ws, streamStart, fpsTmp = 0;
let durTimer, fpsTimer;
let drawing = false;

function setStatus(txt, cls) {
  statusEl.textContent = txt;
  statusEl.className   = cls || '';
}

function updateDuration() {
  const elapsed = Date.now() - streamStart;
  const h = Math.floor(elapsed/3600000),
		m = Math.floor((elapsed%3600000)/60000),
		s = Math.floor((elapsed%60000)/1000);
  durEl.textContent =
	`${String(h).padStart(2,'0')}:` +
	`${String(m).padStart(2,'0')}:` +
	`${String(s).padStart(2,'0')}`;
}

function updateFPS() {
  fpsEl.textContent = `${fpsTmp.toFixed(1)} FPS`;
  fpsTmp = 0;
}

function openWS() {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws';
  ws = new WebSocket(`${proto}://${location.host}/ws`);
  ws.binaryType = 'arraybuffer';

  ws.onopen = () => {
	ws.send("register_viewer");

	setStatus('Connected','status-active');
	connEl.textContent = 'Connected';
	placeholder.style.display = 'none';
	canvas.style.display      = 'block';

	streamStart = Date.now();
	durTimer  = setInterval(updateDuration, 1000);
	fpsTimer  = setInterval(updateFPS, 1000);
  };

  ws.onmessage = async evt => {
	// control frames
	if (typeof evt.data === 'string') {
	  if (evt.data === 'stream_ended') {
		setStatus('Stream stopped');
		connEl.textContent = 'Offline';
		stopBtn.disabled = true;
		clearInterval(durTimer);
		clearInterval(fpsTimer);
	  }
	  return;
	}

	// binary JPEG decode & draw
	if (drawing) return;
	drawing = true;
	try {
	  const blob = new Blob([evt.data], {type:'image/jpeg'});
	  const bitmap = await createImageBitmap(blob);
	  canvas.width  = bitmap.width;
	  canvas.height = bitmap.height;
	  ctx.drawImage(bitmap, 0, 0);
	  bitmap.close();

	  resEl.textContent = `${canvas.width} × ${canvas.height}`;
	  fpsTmp++;
	} finally {
	  drawing = false;
	}
  };

  ws.onclose = () => {
	setStatus('Disconnected','status-error');
	connEl.textContent = 'Disconnected';
	clearInterval(durTimer);
	clearInterval(fpsTimer);
  };
}

// UI bindings
stopBtn.onclick = () => {
  if (ws && ws.readyState === WebSocket.OPEN) ws.send('stop');
};

fullscreenBtn.onclick = () => {
  if (!document.fullscreenElement) wrapper.requestFullscreen();
  else document.exitFullscreen();
};

document.addEventListener('fullscreenchange', () => {
  wrapper.classList.toggle('fullscreen', !!document.fullscreenElement);
});

// kick off on load
document.addEventListener('DOMContentLoaded', openWS);

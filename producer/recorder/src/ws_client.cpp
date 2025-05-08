#include <ws_client.hpp>

#include <mutex>
#include <atomic>
#include <thread>

#include <queue>
#include <vector>

#include <chrono>

#define ASIO_STANDALONE
#define ASIO_HEADER_ONLY
#include <asio.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <utils/log.hpp>

using asio_config = websocketpp::config::asio_client;

struct WsClientContext
{
	std::mutex mtx;
	std::atomic_bool atm_open;
	std::atomic_bool atm_stop;

	websocketpp::connection_hdl h_conn;
	websocketpp::client<asio_config> client;

	std::thread io_thread;
	asio::io_context io_context;
	asio::steady_timer io_timer;

	const size_t buffer_capacity = 128;
	std::queue<std::vector<uint8_t>> ring_buffer;

	WsClientContext()
		: atm_open(false), atm_stop(false), io_timer(asio::steady_timer(io_context)) {}
};

static void
_flush_queue(WsClientContext* self)
{
	static uint64_t debug_frame_id = 0;

	while (!self->ring_buffer.empty() && self->atm_open)
	{
		auto& pkt        = self->ring_buffer.front();
		size_t bytes     = pkt.size();
		double size_kb   = static_cast<double>(bytes) / (1024.0);
		size_t depth     = self->ring_buffer.size();
		uint64_t this_id = debug_frame_id++;

		// Log before send
		LOG_DEBUG("Frame %llu: queue=%llu size=%.2f kB - attempting send...", this_id, depth, size_kb);

		websocketpp::lib::error_code ec;
		self->client.send(
			self->h_conn,
			pkt.data(),
			bytes,
			websocketpp::frame::opcode::binary,
			ec
		);

		if (ec)
		{
			// Log the error and break so we retry this same frame later
			LOG_DEBUG("Frame %llu: send failed (%d – “%s”)", this_id, ec.value(), ec.message().c_str());
			break;
		}

		// Success: pop and log
		self->ring_buffer.pop();
		LOG_DEBUG("Frame %llu: sent OK, new queue depth=%llu",this_id, self->ring_buffer.size());
	}
}

static void
_flush_schedule(WsClientContext* self)
{
	self->io_timer.expires_after(std::chrono::milliseconds(10));

	self->io_timer.async_wait([self](const asio::error_code& ec) {

		if (!ec)
		{
			_flush_queue(self);
			_flush_schedule(self);
		}
	});
}


WsClientContext*
ws_init(const WsClientConfig& ws_config)
{
	WsClientContext* self = new WsClientContext();
	if (!self)
		return nullptr;

	self->client.init_asio(&self->io_context);
	self->client.clear_access_channels(websocketpp::log::alevel::all);
	self->client.set_access_channels(websocketpp::log::alevel::connect | websocketpp::log::alevel::disconnect);

	self->io_timer = asio::steady_timer(self->io_context);

	auto start_connect = [self, ws_config] -> int {
		websocketpp::lib::error_code ec;

		auto con = self->client.get_connection(ws_config.url, ec);
		if (ec)
			return -1;

		self->client.connect(con);

		return 0;
	};

	self->client.set_open_handler([self](websocketpp::connection_hdl h) {
		self->h_conn = h;
		self->atm_open = true;

		self->client.send(h, "register_producer", websocketpp::frame::opcode::text);

		_flush_schedule(self);
	});

	self->client.set_close_handler([self, start_connect](websocketpp::connection_hdl) {
		self->atm_open = false;

		asio::steady_timer* t = new asio::steady_timer(self->io_context, std::chrono::seconds(1));

		t->async_wait([start_connect, t](const asio::error_code&) {
			delete t;
			start_connect();
		});
	});

	self->client.set_fail_handler([self, start_connect](websocketpp::connection_hdl) {
		self->atm_open = false;

		asio::steady_timer* t = new asio::steady_timer(self->io_context, std::chrono::seconds(1));

		t->async_wait([start_connect, t](const asio::error_code&) {
			delete t;
			start_connect();
		});
	});

	self->client.set_message_handler([self](websocketpp::connection_hdl, websocketpp::client<asio_config>::message_ptr msg) {
		if (msg->get_opcode() == websocketpp::frame::opcode::text && msg->get_payload() == "stop")
			self->atm_stop = true;
	});

	if (start_connect() != 0)
		return nullptr;

	self->io_thread = std::thread([self] {
		self->io_context.run();
	});

	return self;
}

void
ws_request_stop(WsClientContext* self)
{
	self->atm_stop.store(true);
}

int
ws_stop_requested(WsClientContext* self)
{
	return self->atm_stop.load() ? 1 : 0;
}

int
ws_send_frame(WsClientContext* self, const uint8_t* data, size_t len)
{
	std::vector<uint8_t> copy(len);
	std::memcpy(copy.data(), data, len);

	std::lock_guard<std::mutex> lk(self->mtx);

	if (self->ring_buffer.size() >= self->buffer_capacity)
		self->ring_buffer.pop();

	self->ring_buffer.push(std::move(copy));

	return 0;
}

void
ws_free(WsClientContext* self)
{
	if (self->atm_open)
	{
		websocketpp::lib::error_code ec;
		self->client.close(self->h_conn, websocketpp::close::status::normal, "", ec);
	}

	self->io_context.stop();

	if (self->io_thread.joinable())
		self->io_thread.join();

	std::lock_guard<std::mutex> lk(self->mtx);
	std::queue<std::vector<uint8_t>> empty;
	std::swap(self->ring_buffer, empty);
}

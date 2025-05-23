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
#include <asio/ssl.hpp> // Added for SSL support

#include <websocketpp/client.hpp>
// #include <websocketpp/config/asio_no_tls_client.hpp> // Replaced with asio_client for TLS
#include <websocketpp/config/asio_client.hpp> 
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <utils/log.hpp>

// using asio_config = websocketpp::config::asio_client; // Already using asio_client, but ensure it's the TLS-enabled one
using client = websocketpp::client<websocketpp::config::asio_tls_client>; // Use asio_tls_client
using context_ptr = websocketpp::lib::shared_ptr<asio::ssl::context>;

struct WsClientContext
{
	std::mutex mtx;
	std::atomic_bool atm_open;
	std::atomic_bool atm_stop;

	websocketpp::connection_hdl h_conn;
	client ws_client; // Changed to the TLS client type

	std::thread io_thread;
	asio::io_context io_context;
	asio::steady_timer io_timer;

	const size_t buffer_capacity = 128;
	std::queue<std::vector<uint8_t>> ring_buffer;

	WsClientContext()
		: atm_open(false), atm_stop(false), io_timer(asio::steady_timer(io_context)) {}
};

static context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
    // Create a new SSL context
    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
        // Configure the context for client-side TLS
        ctx->set_options(asio::ssl::context::default_workarounds |
                         asio::ssl::context::no_sslv2 |
                         asio::ssl::context::no_sslv3 |
                         asio::ssl::context::single_dh_use);

        // Set verification mode to verify the server's certificate
        ctx->set_verify_mode(asio::ssl::verify_peer); // Restore peer verification
        // TODO: Replace with actual path to CA bundle or server certificate
        // For testing, you might use a self-signed certificate and load it directly
        // For production, use a CA bundle
        ctx->load_verify_file("ca.pem"); // Restore loading ca.pem
    } catch (std::exception& e) {
        LOG_ERROR("TLS initialization failed: %s", e.what());
    }
    return ctx;
}


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
		self->ws_client.send( // Changed to ws_client
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

	self->ws_client.init_asio(&self->io_context); // Changed to ws_client
    self->ws_client.set_tls_init_handler(bind(&on_tls_init, websocketpp::lib::placeholders::_1)); // Added TLS init handler
	self->ws_client.clear_access_channels(websocketpp::log::alevel::all); // Changed to ws_client
	self->ws_client.set_access_channels(websocketpp::log::alevel::connect | websocketpp::log::alevel::disconnect); // Changed to ws_client

	self->io_timer = asio::steady_timer(self->io_context);

	auto start_connect = [self, ws_config] -> int {
		websocketpp::lib::error_code ec;

		auto con = self->ws_client.get_connection(ws_config.url, ec); // Changed to ws_client
		if (ec)
			return -1;

		self->ws_client.connect(con); // Changed to ws_client

		return 0;
	};

	self->ws_client.set_open_handler([self](websocketpp::connection_hdl h) { // Changed to ws_client
		self->h_conn = h;
		self->atm_open = true;

		self->ws_client.send(h, "register_producer", websocketpp::frame::opcode::text); // Changed to ws_client

		_flush_schedule(self);
	});

	self->ws_client.set_close_handler([self, start_connect](websocketpp::connection_hdl) { // Changed to ws_client
		self->atm_open = false;

		asio::steady_timer* t = new asio::steady_timer(self->io_context, std::chrono::seconds(1));

		t->async_wait([start_connect, t](const asio::error_code&) {
			delete t;
			start_connect();
		});
	});

	self->ws_client.set_fail_handler([self, start_connect](websocketpp::connection_hdl) { // Changed to ws_client
		self->atm_open = false;

		asio::steady_timer* t = new asio::steady_timer(self->io_context, std::chrono::seconds(1));

		t->async_wait([start_connect, t](const asio::error_code&) {
			delete t;
			start_connect();
		});
	});

	self->ws_client.set_message_handler([self](websocketpp::connection_hdl, client::message_ptr msg) { // Changed to ws_client and client::message_ptr
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
		self->ws_client.close(self->h_conn, websocketpp::close::status::normal, "", ec); // Changed to ws_client
	}

	self->io_context.stop();

	if (self->io_thread.joinable())
		self->io_thread.join();

	std::lock_guard<std::mutex> lk(self->mtx);
	std::queue<std::vector<uint8_t>> empty;
	std::swap(self->ring_buffer, empty);
}

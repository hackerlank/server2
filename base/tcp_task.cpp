#include "common.h"
#include "tcp_task.h"
#include "task_state.h"

using namespace boost;
using namespace boost::asio;

tcp_task::tcp_task(io_service& ios): ios_(ios), sock_(ios_), timer_(ios_)
{
	m_state = NOTUSE;
}

tcp_task::~tcp_task() {
	Xlogger->debug("%s", __PRETTY_FUNCTION__);
}

const std::string tcp_task::get_remote_ip() const 
{
	boost::system::error_code ec;
	ip::tcp::socket::endpoint_type endpoint = sock_.remote_endpoint(ec);
	if (ec)
		return "";
	else {
		return endpoint.address().to_string();
	}
}

void tcp_task::start()
{

	m_state = VERIFY;

	boost::asio::ip::tcp::no_delay option(true);
	sock_.set_option(option);

	async_read();
}

void tcp_task::async_read(){
	boost::asio::async_read(sock_, 
			boost::asio::buffer(header_),
			bind(&tcp_task::handle_read_header,shared_from_this(),_1,_2));
}

bool tcp_task::sendCmd(const void* data, const int len) {
	//对data 进行压缩 加密 later
	std::vector<char> tmp(PH_LEN + len);
	uint32_t n = len;
	bcopy(&n, &tmp[0], PH_LEN);
	bcopy(data, &tmp[PH_LEN],len);
	m_write_buf.sputn(&tmp[0], tmp.size());
	//send 
	sock_.async_write_some(m_write_buf.data(),bind(&tcp_task::handle_write,shared_from_this(),_1,_2));
	return true;
}

void tcp_task::handle_read_header(const boost::system::error_code& error, std::size_t bytes_transferred) {
	//Xlogger->debug("%s, bytes_transferred=%u",__PRETTY_FUNCTION__, bytes_transferred);
	if (error){
		handle_error(error);
	}
	else if (decode_header()) {
		asio::async_read(sock_, asio::buffer(msg_, msg_length_),bind(&tcp_task::handle_read_body,shared_from_this(),_1,_2));
	}
	else {
		handle_error(boost::system::error_code());
	}
}

void tcp_task::handle_read_body(const boost::system::error_code& error, std::size_t bytes_transferred) {
	//Xlogger->debug("%s",__PRETTY_FUNCTION__);
	if (error) {
			handle_error(error);
	}
	else {
		//decode
		//uncompress
		uint32_t reallen = msg_length_;
		handle_msg(msg_, reallen);
	}
}

bool tcp_task::decode_header() {
	msg_length_ = *(uint32_t*)header_;
	//Xlogger->debug("%s, msg length = %u",__PRETTY_FUNCTION__, msg_length_);
	if (msg_length_ > MAX_MSG_SIZE) {
		msg_length_ = 0;
		Xlogger->debug("%s error",__PRETTY_FUNCTION__);
		return false;
	}
	else
		return true;
}

void tcp_task::handle_error(const boost::system::error_code& error) {
	close();
}

void tcp_task::handle_write( const boost::system::error_code& error, std::size_t bytes_transferred) {
	//Xlogger->debug("%s:%u",__PRETTY_FUNCTION__, __LINE__);
	if(error) {
		Xlogger->debug("%s:%u,message=%s",__PRETTY_FUNCTION__, __LINE__, error.message().c_str());
		handle_error(error);
		return ;
	}
	//reduce send buffer
	m_write_buf.consume(bytes_transferred);	
	//if sending msg not complete, keep sending
	if (m_write_buf.size() > 0)
		sock_.async_write_some(m_write_buf.data(),bind(&tcp_task::handle_write,shared_from_this(),_1,_2));
}

void tcp_task::close() { 
	Xlogger->debug("%s use count = %d", __PRETTY_FUNCTION__, shared_from_this().use_count());
	boost::system::error_code ec;
	sock_.shutdown(ip::tcp::socket::shutdown_both, ec);
	Xlogger->debug("%s use count = %d", __PRETTY_FUNCTION__, shared_from_this().use_count());
}


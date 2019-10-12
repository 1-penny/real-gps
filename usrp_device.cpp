#include "usrp_device.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
namespace algo = boost::algorithm;

#ifdef ENABLE_USRP_DEVICE

UsrpDevice::UsrpDevice(const std::string& dev_addr)
{
	m_dev_addr = dev_addr;
	
	initialize();
}

bool UsrpDevice::initialize()
{
	if (!m_usrp) {
		m_usrp = uhd::usrp::multi_usrp::make(m_dev_addr);

		if (m_properties.get<bool>("verbose", false)) {
			std::string msg = m_usrp ?
				"Create usrp ok \n" : "Faield to create usrp\n";
			printf(msg.c_str());
		}
	}

	return m_usrp.get() != nullptr;
}

bool UsrpDevice::open()
{
	if (!m_usrp)
		return false;

	if (! is_open()) {
		uhd::stream_args_t tx_arg;
		tx_arg.cpu_format = "sc16";
		tx_arg.otw_format = "sc16";
		m_tx = m_usrp->get_tx_stream(tx_arg);

		if (m_properties.get<bool>("verbose", false)) {
			std::string msg = m_tx ?
				"Create tx_streamer ok \n" : "Faield to create tx_streamer\n";
			printf(msg.c_str());
		}
	}

	return m_tx.get() != nullptr;
}

void UsrpDevice::close()
{
	if (is_open()) {
		m_tx.reset();
	}
}

bool UsrpDevice::is_open()
{
	return m_usrp && m_tx;
}

int UsrpDevice::send(void* data, int elem_num, int elem_size, int timeout)
{
	if (!is_open() || !data || elem_num <= 0 || elem_size <= 0) {
		if (m_properties.get<bool>("verbose", false)) {
			printf("Faield to send data \n");
		}
		return 0;
	}

	uhd::tx_metadata_t meta;
	int count = m_tx->send(data, elem_num, meta, 1);

	if (count != elem_num) {
		if (m_properties.get<bool>("verbose", false)) {
			printf("usrp send data error : less or no \n");
		}
	}

	return count;
}

bool UsrpDevice::set_param(const std::string& name, const std::string& value)
{
	try {
		bool result = false;

		if (algo::iequals(name, "verbose")) {
			bool verb = boost::lexical_cast<bool>(value);
			result = true;
		}

		if (algo::iequals(name, "tx.freq") && m_usrp) {
			double freq = boost::lexical_cast<double>(value);
			m_usrp->set_tx_freq(freq);
			result = true;
		}

		if (algo::iequals(name, "tx.rate") && m_usrp) {
			int rate = boost::lexical_cast<double>(value);
			m_usrp->set_tx_rate(rate);
			result = true;
		}

		if (algo::iequals(name, "tx.gain") && m_usrp) {
			int gain = boost::lexical_cast<double>(value);
			m_usrp->set_tx_gain(gain);
			result = true;
		}

		if (result) {
			m_properties.put(name, value);
		}

		if (m_properties.get<bool>("verbose", false)) {
			std::string message = result ?
				(boost::format("usrp : set %s ok : %s\n") % name % value).str()
				: (boost::format("usrp : set %s error : %s\n") % name % value).str();
			printf(message.c_str());
		}
				
		return result;
	}
	catch (...) {
		return false;
	}
}

const std::string UsrpDevice::get_param(const std::string& name)
{
	try {
		if (algo::iequals(name, "freq")) {
			double freq = m_usrp->get_tx_freq();
			return boost::lexical_cast<std::string>(freq);
		}

		if (algo::iequals(name, "rate")) {
			double rate = m_usrp->get_tx_rate();
			return boost::lexical_cast<std::string>(rate);
		}

		if (algo::iequals(name, "gain")) {
			double gain = m_usrp->get_tx_gain();
			return boost::lexical_cast<std::string>(gain);
		}

		return "";
	}
	catch (...) {
		return "";
	}
}

#endif //ENABLE_USRP_DEVICE
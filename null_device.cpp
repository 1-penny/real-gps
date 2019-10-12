#include "null_device.h"

#include <stdlib.h>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
namespace algo = boost::algorithm;

#include <thread>

NullDevice::NullDevice()
{
	m_open = false;
	m_rate = -1;
}

bool NullDevice::open()
{
	m_open = true;
	return is_open();
}

void NullDevice::close()
{
	m_open = false;
}

bool NullDevice::is_open()
{
	return m_open;
}

int NullDevice::send(void* data, int elem_num, int elem_size, int timeout)
{
	if (!is_open() || !data || elem_num <= 0 || elem_size <= 0)
		return 0;
	
	if (m_rate > 0) {
		double ms = elem_num / m_rate * 1000 * 0.5;
		int sleep_value = (int) ms;
		if (sleep_value > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_value));
		}
	}

	return elem_num;
}

bool NullDevice::set_param(const std::string& name, const std::string& value)
{
	try {
		bool result = false;
		
		if (algo::iequals(name, "tx.rate")) {
			double rate = boost::lexical_cast<double>(value);
			if (rate > 0) {
				m_rate = rate;
				result = true;
			}
		}
		else {
			result = Device::set_param(name, value);
		}

		if (result) {
			m_properties.put(name, value);
		}

		if (m_properties.get<bool>("verbose", false)) {
			std::string message = result ?
				(boost::format("device : set %s ok : %s\n") % name % value).str()
				: (boost::format("device : set %s error : %s\n") % name % value).str();
			printf(message.c_str());
		}

		return result;
	}
	catch (...) {
		return false;
	}
}

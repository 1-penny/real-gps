#include "file_device.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
namespace algo = boost::algorithm;

FileDevice::FileDevice(const std::string& filename)
{
	m_filename = filename;
}

bool FileDevice::open()
{
	if (!is_open()) {
		if (!m_filename.empty()) {
			m_file.open(m_filename, std::ios::binary);
		}
	}

	return is_open();
}

void FileDevice::close()
{
	if (m_file.is_open()) {
		m_file.close();
	}
}

bool FileDevice::set_param(const std::string& name, const std::string& value)
{
	try {
		bool result = false;

		if (algo::iequals(name, "verbose")) {
			bool verbose = boost::lexical_cast<bool>(value);
			result = true;
		}

		if (algo::iequals(name, "filename")) {
			m_filename = value;
			result = true;
		}

		if (algo::iequals(name, "tx.freq")) {
			double freq = boost::lexical_cast<double>(value);
			m_freq = freq;
			result = true;
		}

		if (algo::iequals(name, "tx.rate")) {
			int rate = boost::lexical_cast<double>(value);
			m_rate = rate;
			result = true;
		}

		if (algo::iequals(name, "tx.gain")) {
			int gain = boost::lexical_cast<double>(value);
			m_gain = gain;
			result = true;
		}

		if (result) {
			m_properties.put(name, value);
		}

		if (m_properties.get<bool>("verbose", false)) {
			std::string message = result ?
				(boost::format("file : set %s ok : %s\n") % name % value).str()
				: (boost::format("file : set %s error : %s\n") % name % value).str();
			printf(message.c_str());
		}

		return result;
	}
	catch (...) {
		return false;
	}
}

bool FileDevice::is_open()
{
	return m_file.is_open();
}

int FileDevice::send(void* data, int elem_num, int elem_size, int timeout)
{
	if (data == nullptr || elem_num <= 0 || elem_size <= 0)
		return 0;

	if (is_open()) {
		m_file.write((const char*)data, (std::streamsize) elem_num * elem_size);
		return elem_num;
	}

	return 0;
}


#include "device.h"

#include "file_device.h"
#include "usrp_device.h"
#include "null_device.h"

#include <boost/algorithm/string.hpp>
namespace algo = boost::algorithm;

Device::~Device()
{
	close();
}

bool Device::set_params(const std::map<std::string, std::string>& params)
{
	for (auto param : params) {
		if (!set_param(param.first, param.second)) {
			return false;
		}
	}

	return true;
}

bool Device::set_param(const std::string& name, const std::string& value)
{
	if (!name.empty() && !value.empty()) {
		m_properties.put(name, value);
		return true;
	}

	return false;
}

const std::string Device::get_param(const std::string& name)
{
	return m_properties.get<std::string>(name, "");
}

Device* Device::make(const std::string& name, const std::map<std::string, std::string>& params)
{
	std::unique_ptr<Device> ptr;

	if (algo::iequals(name, "") || algo::iequals(name, "null")) {
		ptr.reset(new NullDevice);
	}
	else if (algo::iequals(name, "file")) {
		ptr.reset(new FileDevice());
	}
	else if (algo::iequals(name, "usrp")) {
		ptr.reset(new UsrpDevice());
	}

	if (!params.empty()) {
		if (!ptr->set_params(params)) {
			return nullptr;
		}
	}
	
	return ptr.release();
}



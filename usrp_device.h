#pragma once

#include "device.h"
#define ENABLE_USRP_DEVICE

#ifdef ENABLE_USRP_DEVICE


#include <uhd.h>
#include <uhd/usrp/multi_usrp.hpp>
#pragma comment(lib, "uhd.lib")

/**
 * USRP …Ë±∏.
 */
class UsrpDevice : public Device
{
public:
	UsrpDevice(const std::string& dev_addr = "");

public:
	virtual bool initialize();

	virtual bool set_param(const std::string& name, const std::string& value);
	virtual const std::string get_param(const std::string& name);

	virtual bool open();
	virtual void close();
	virtual bool is_open();

	virtual int send(void* data, int elem_num, int elem_size, int timeout = 0);

private:
	std::string m_dev_addr;
	uhd::usrp::multi_usrp::sptr m_usrp;
	uhd::tx_streamer::sptr m_tx;
};

#else

class UsrpDevice : public Device {};

#endif //ENABLE_USRP_DEVICE

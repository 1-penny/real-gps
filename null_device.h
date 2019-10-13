#pragma once

#include "device.h"

class NullDevice : public Device
{
public:
	NullDevice();
	virtual ~NullDevice();

public:
	virtual bool open();
	virtual void close();
	virtual bool is_open();
	
	virtual int send(void* data, int elem_num, int elem_size, int timeout = 0);

	virtual bool set_param(const std::string& name, const std::string& value);

private:
	bool m_open;

	double m_rate;
};
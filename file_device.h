#pragma once

#include "device.h"
#include <fstream>

#include "device.h"

#include <fstream>

// 文件输出设备.
class FileDevice : public Device
{
public:
	FileDevice(const std::string& filename = "");

public:
	virtual bool open();
	virtual void close();
	virtual bool is_open();

	virtual bool set_param(const std::string& name, const std::string& value);

	virtual int send(void* data, int elem_num, int elem_size, int timeout = 0);

private:
	std::string m_filename;
	std::ofstream m_file;


	double m_rate;
	double m_freq;
	double m_gain;
};

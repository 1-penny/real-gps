#pragma once

#include <fstream>

// 输出设备.
class Device
{
public:
	Device() {}
	virtual ~Device();

public:
	virtual int open(void* param = nullptr) { return 0; }
	virtual void close() {}
	virtual int init() { return 0; }
	virtual int send(void* data, unsigned int count, unsigned int elemsize, unsigned int timeout = 0) { return 0; }
};

// 文件输出设备.
class FileDevice : public Device
{
public:
	virtual int open(void* param = nullptr);
	virtual void close();
	virtual int send(void* data, unsigned int count, unsigned int elemsize, unsigned int timeout = 0);

private:
	std::ofstream m_file;
};


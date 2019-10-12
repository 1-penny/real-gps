#ifndef DEVICE_H_
#define DEVICE_H_

#include <string>
#include <map>

#include <boost/property_tree/ptree.hpp>

/**
 * 输出设备.
 */
class Device
{
public:
	static Device* make(const std::string& name, const std::map<std::string, std::string>& params = {});

public:
	Device() {}
	virtual ~Device();

public:
	/**
	 * 设置参数.
	 * @param params 参数字典.
	 * @see set_param()
	 */
	virtual bool set_params(const std::map<std::string, std::string>& params);

	/**
	 * 设置参数.
	 * @param name 参数名称.
	 * @param value 参数内容.
	 * @return 是否设置成功.
	 */
	virtual bool set_param(const std::string& name, const std::string& value);

	/**
	 * 获取参数内容.
	 * @param name 参数名称.
	 * @return 参数内容. 空字符串表示无法获取.
	 */
	virtual const std::string get_param(const std::string& name);

	/**
	 * 打开设备.
	 * 设备如成功打开，即具备读写操作能力.
	 * @return 操作是否成功.
	 */
	virtual bool open() { return true; }

	// 关闭设备.
	virtual void close() {}

	// 设备是否已经打开.
	virtual bool is_open() { return true; }

	/**
	 * 同步发送数据.
	 * @param data 数据缓冲区.
	 * @param elem_num 数据的数量.
	 * @param elem_size 每个数据的大小.
	 * @param timeout 超时时间（ms）. 0表示使用默认值.
	 * @return 实际发送数据数量.
	 */
	virtual int send(void* data, int elem_num, int elem_size, int timeout = 0) { return 0; }

protected:
	boost::property_tree::ptree m_properties;
};

#endif //DEVICE_H_


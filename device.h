#ifndef DEVICE_H_
#define DEVICE_H_

#include <string>
#include <map>

#include <boost/property_tree/ptree.hpp>

/**
 * ����豸.
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
	 * ���ò���.
	 * @param params �����ֵ�.
	 * @see set_param()
	 */
	virtual bool set_params(const std::map<std::string, std::string>& params);

	/**
	 * ���ò���.
	 * @param name ��������.
	 * @param value ��������.
	 * @return �Ƿ����óɹ�.
	 */
	virtual bool set_param(const std::string& name, const std::string& value);

	/**
	 * ��ȡ��������.
	 * @param name ��������.
	 * @return ��������. ���ַ�����ʾ�޷���ȡ.
	 */
	virtual const std::string get_param(const std::string& name);

	/**
	 * ���豸.
	 * �豸��ɹ��򿪣����߱���д��������.
	 * @return �����Ƿ�ɹ�.
	 */
	virtual bool open() { return true; }

	// �ر��豸.
	virtual void close() {}

	// �豸�Ƿ��Ѿ���.
	virtual bool is_open() { return true; }

	/**
	 * ͬ����������.
	 * @param data ���ݻ�����.
	 * @param elem_num ���ݵ�����.
	 * @param elem_size ÿ�����ݵĴ�С.
	 * @param timeout ��ʱʱ�䣨ms��. 0��ʾʹ��Ĭ��ֵ.
	 * @return ʵ�ʷ�����������.
	 */
	virtual int send(void* data, int elem_num, int elem_size, int timeout = 0) { return 0; }

protected:
	boost::property_tree::ptree m_properties;
};

#endif //DEVICE_H_


#pragma once

#include <vector>
#include <complex>
#include <stdint.h>

/**
 * FIFO.
 * ___________________________________________
 * |.......|tail|...|...|...|...|head|.......|
 * -------------------------------------------
 */
class fifo_t
{
public:
	typedef int16_t elem_type;
	typedef std::complex<elem_type> data_type;

public:
	/**
	 * ���캯��.
	 */
	fifo_t(size_t fifo_len, size_t min_len);

	/// ��ȡ��������.
	size_t get_sample_length();

	// �� FIFO �ж�ȡ����.
	size_t read(void * samples, size_t sample_num);

	// �� FIFO ��д������.
	size_t write(void * samples, size_t sample_num);

	// FIFO �Ƿ��ܹ�д��.
	bool is_write_ready();

	// FIFO �Ƿ��ܹ���ȡ.
	bool is_read_ready();
	
private:
	std::vector<data_type> m_fifo;
	long m_head;
	long m_tail;
	size_t sample_length;

	size_t m_fifo_length;
	size_t m_min_length;
};


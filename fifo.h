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
	 * 构造函数.
	 */
	fifo_t(size_t fifo_len, size_t min_len);

	/// 获取数据数量.
	size_t get_sample_length();

	// 从 FIFO 中读取数据.
	size_t read(void * samples, size_t sample_num);

	// 向 FIFO 中写入数据.
	size_t write(void * samples, size_t sample_num);

	// FIFO 是否能够写入.
	bool is_write_ready();

	// FIFO 是否能够读取.
	bool is_read_ready();
	
private:
	std::vector<data_type> m_fifo;
	long m_head;
	long m_tail;
	size_t sample_length;

	size_t m_fifo_length;
	size_t m_min_length;
};


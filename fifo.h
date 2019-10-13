#pragma once

#include <vector>
#include <complex>
#include <stdint.h>

class fifo_t
{
public:
	typedef int16_t elem_type;
	typedef std::complex<elem_type> data_type;

public:
	fifo_t(size_t fifo_len, size_t free_len);

	size_t get_sample_length();

	size_t read(int16_t* samples, size_t sample_num);

	size_t write(int16_t* samples, size_t sample_num);

	bool is_write_ready();
	
private:
	std::vector<int16_t> fifo;
	long head;
	long tail;
	size_t sample_length;

	size_t fifo_length;
	size_t free_length;
};


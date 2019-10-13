#include "fifo.h"

fifo_t::fifo_t(size_t fifolen, size_t freelen) 
{
	m_fifo_length = fifolen;
	m_min_length = freelen;

	m_head = 0;
	m_tail = 0;
	sample_length = 0;

	// Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
	m_fifo.resize(m_fifo_length); // for 16-bit I and Q samples
}

size_t fifo_t::get_sample_length()
{
	long length;

	length = m_head - m_tail;
	if (length < 0)
		length += m_fifo_length;

	return((size_t)length);
}

size_t fifo_t::read(void * buffer, size_t samples)
{
	size_t length;
	size_t samples_remaining;
	data_type* buffer_current = (data_type*) buffer;

	length = get_sample_length();

	if (length < samples)
		samples = length;

	length = samples; // return value

	samples_remaining = m_fifo_length - m_tail;

	if (samples > samples_remaining) {
		memcpy(buffer_current, &(m_fifo[m_tail]), samples_remaining * sizeof(data_type));
		m_tail = 0;
		buffer_current += samples_remaining;
		samples -= samples_remaining;
	}

	memcpy(buffer_current, &(m_fifo[m_tail]), samples * sizeof(data_type));
	m_tail += (long)samples;
	if (m_tail >= m_fifo_length)
		m_tail -= m_fifo_length;

	return(length);
}

size_t fifo_t::write(void * buffer, size_t samples)
{
	memcpy(&(m_fifo[m_head]), buffer, samples * sizeof(data_type));

	m_head += (long)samples;
	if (m_head >= m_fifo_length)
		m_head -= m_fifo_length;

	return samples;
}

bool fifo_t::is_write_ready()
{
	int sample_len = get_sample_length();
	return (sample_len < m_min_length/*NUM_IQ_SAMPLES*/);
}

bool fifo_t::is_read_ready()
{
	return get_sample_length() > 0;
}

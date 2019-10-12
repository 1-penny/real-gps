#include "fifo.h"

fifo_t::fifo_t(size_t fifolen, size_t freelen) 
{
	fifo_length = fifolen;
	free_length = freelen;

	head = 0;
	tail = 0;
	sample_length = 0;

	// Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
	fifo.resize(fifo_length * 2); // for 16-bit I and Q samples
}

size_t fifo_t::get_sample_length()
{
	long length;

	length = head - tail;
	if (length < 0)
		length += fifo_length;

	return((size_t)length);
}

size_t fifo_t::read(int16_t* buffer, size_t samples)
{
	size_t length;
	size_t samples_remaining;
	int16_t* buffer_current = buffer;

	length = get_sample_length();

	if (length < samples)
		samples = length;

	length = samples; // return value

	samples_remaining = fifo_length - tail;

	if (samples > samples_remaining) {
		memcpy(buffer_current, &(fifo[tail * 2]), samples_remaining * sizeof(int16_t) * 2);
		tail = 0;
		buffer_current += samples_remaining * 2;
		samples -= samples_remaining;
	}

	memcpy(buffer_current, &(fifo[tail * 2]), samples * sizeof(int16_t) * 2);
	tail += (long)samples;
	if (tail >= fifo_length)
		tail -= fifo_length;

	return(length);
}

size_t fifo_t::write(int16_t* buffer, size_t samples)
{
	memcpy(&(fifo[head * 2]), buffer, samples * 2 * sizeof(short));

	head += (long)samples;
	if (head >= fifo_length)
		head -= fifo_length;

	return samples;
}

int fifo_t::is_write_ready()
{
	int status = 0;

	int sample_len = get_sample_length();
	if (sample_len < free_length/*NUM_IQ_SAMPLES*/)
		status = 1;

	return(status);
}

//
//size_t get_sample_length(sim_t* s)
//{
//	long length;
//
//	length = s->head - s->tail;
//	if (length < 0)
//		length += FIFO_LENGTH;
//
//	return((size_t)length);
//}
//
//size_t fifo_read(int16_t* buffer, size_t samples, sim_t* s)
//{
//	size_t length;
//	size_t samples_remaining;
//	int16_t* buffer_current = buffer;
//
//	length = get_sample_length(s);
//
//	if (length < samples)
//		samples = length;
//
//	length = samples; // return value
//
//	samples_remaining = FIFO_LENGTH - s->tail;
//
//	if (samples > samples_remaining) {
//		memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples_remaining * sizeof(int16_t) * 2);
//		s->tail = 0;
//		buffer_current += samples_remaining * 2;
//		samples -= samples_remaining;
//	}
//
//	memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples * sizeof(int16_t) * 2);
//	s->tail += (long)samples;
//	if (s->tail >= FIFO_LENGTH)
//		s->tail -= FIFO_LENGTH;
//
//	return(length);
//}
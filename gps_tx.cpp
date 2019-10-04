#define _CRT_SECURE_NO_WARNINGS

#include "gps_tx.h"
#include "real_gps.h"

// for _getch used in Windows runtime.
#ifdef _WIN32
#include <conio.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif

void tx_task(void* arg)
{
	sim_t* s = (sim_t*)arg;
	size_t samples_populated;

	while (true) {
		int16_t* tx_buffer_current = s->tx.buffer;
		unsigned int buffer_samples_remaining = SAMPLES_PER_BUFFER;

		while (buffer_samples_remaining > 0) {

			{
				//* pthread_mutex_lock(&(s->gps.lock));
				std::unique_lock<std::mutex> lck(s->gps.lock);
				while (get_sample_length(s) == 0)
				{
					//* pthread_cond_wait(&(s->fifo_read_ready), &(s->gps.lock));
					s->fifo_read_ready.wait(lck);
				}
				// assert(get_sample_length(s) > 0);

				samples_populated = fifo_read(tx_buffer_current,
					buffer_samples_remaining, s);
				//* pthread_mutex_unlock(&(s->gps.lock));
			}

			//* pthread_cond_signal(&(s->fifo_write_ready));
			s->fifo_write_ready.notify_all();
#if 0
			if (is_fifo_write_ready(s)) {
				/*
				printf("\rTime = %4.1f", s->time);
				s->time += 0.1;
				fflush(stdout);
				*/
			}
			else if (is_finished_generation(s))
			{
				goto out;
			}
#endif
			// Advance the buffer pointer.
			buffer_samples_remaining -= (unsigned int)samples_populated;
			tx_buffer_current += (2 * samples_populated);
		}

		device_send(s);

		if (is_fifo_write_ready(s)) {
			/*
			printf("\rTime = %4.1f", s->time);
			s->time += 0.1;
			fflush(stdout);
			*/
		}
		else if (is_finished_generation(s))	{
			return;
		}
	}
}



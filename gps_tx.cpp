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

#include <assert.h>

void tx_task(void* arg)
{
	printf("Enter Tx Task.\n");

	sim_t* s = (sim_t*)arg;
	size_t samples_populated;

	while (true) {
		int16_t* tx_buffer_current = s->tx.buffer.get();
		unsigned int samples_chunk = SAMPLES_PER_BUFFER;

		{
			std::unique_lock<std::mutex> lck(s->gps.mtx);
			while (fifo_get_sample_length(s) < SAMPLES_PER_BUFFER && !is_finished_generation(s))
			{
				s->fifo_read_ready.wait(lck);
			}
			// assert(get_sample_length(s) > 0);

			samples_populated = fifo_read(tx_buffer_current, samples_chunk, s);
			assert(samples_populated == samples_chunk);
		}

		s->fifo_write_ready.notify_all();

		if (is_finished_generation(s)) {
			goto out;
		}

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

		s->tx.dev->send(s->tx.buffer.get(), SAMPLES_PER_BUFFER, 4, TIMEOUT_MS);
		//bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);

		if (is_fifo_write_ready(s)) {
			/*
			printf("\rTime = %4.1f", s->time);
			s->time += 0.1;
			fflush(stdout);
			*/
		}
	}

out:
	printf("Quit Tx Task.\n");
}



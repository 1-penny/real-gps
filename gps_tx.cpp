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

	while (true) {
		int16_t* tx_buffer_current = s->tx.buffer.get();
		unsigned int samples_chunk = SAMPLES_PER_BUFFER;

		// 等待条件变量，同时解开同步锁.
		{
			std::unique_lock<std::mutex> lck(s->gps.mtx);
			while (s->fifo.get_sample_length() < SAMPLES_PER_BUFFER && !is_finished_generation(s))
			{
				s->fifo_read_ready.wait(lck);
			}
			
			if (is_finished_generation(s)) {
				goto out;
			}
			
			size_t samples_populated = s->fifo.read(tx_buffer_current, samples_chunk);
			assert(samples_populated == samples_chunk);
		}

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

		s->tx.dev->send(s->tx.buffer.get(), SAMPLES_PER_BUFFER, 4, TIMEOUT_MS);

		if (s->fifo.is_write_ready()) {
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



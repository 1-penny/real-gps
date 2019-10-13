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
	printf("Enter Tx Task.\n");

	sim_t* s = (sim_t*)arg;
	size_t samples_populated;

	while (true) {
		int16_t* tx_buffer_current = s->tx_buffer.data();
		unsigned int buffer_samples_remaining = SAMPLES_PER_BUFFER;

		while (buffer_samples_remaining > 0) {
			
			/// 等待 FIFO 可读取.
			{
				std::unique_lock<std::mutex> lck(s->fifo_mtx);
				while (!s->fifo.is_read_ready() && !s->is_finished_generation()) {
					s->fifo_read_ready.wait(lck);
				}

				// 读取 FIFO.
				samples_populated = s->fifo.read(tx_buffer_current, buffer_samples_remaining);
			}

			/// 通知 FIFO 可写入.
			s->fifo_write_ready.notify_all();

			if (s->is_finished_generation()) {
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
			// Advance the buffer pointer.
			buffer_samples_remaining -= (unsigned int)samples_populated;
			tx_buffer_current += (2 * samples_populated);
		}

		s->device->send(s->tx_buffer.data(), SAMPLES_PER_BUFFER, 4, TIMEOUT_MS);
		//bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);

		if (s->fifo.is_write_ready()) {
			/*
			printf("\rTime = %4.1f", s->time);
			s->time += 0.1;
			fflush(stdout);
			*/
		}
		
		if (s->is_finished_generation())	{
			goto out;
		}
	}

out:
	printf("Quit Tx Task.\n");
}



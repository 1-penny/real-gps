#pragma once

#include <stdint.h>
#include <complex>
#include <vector>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "gpssim.h"
#include "device.h"
#include "fifo.h"

#define TX_FREQUENCY	1575420000
#define TX_SAMPLERATE	2600000

#define NUM_IQ_SAMPLES  (TX_SAMPLERATE / 10) // samples in 0.1s
#define FIFO_LENGTH     (NUM_IQ_SAMPLES * 2) // samples in fifo = samples in 0.1s

// Interactive mode directions
#define UNDEF 0
#define NORTH 1
#define SOUTH 2
#define EAST  3
#define WEST  4
#define UP    5
#define DOWN  6
// Interactive keys
#define NORTH_KEY 'w'
#define SOUTH_KEY 's'
#define EAST_KEY  'd'
#define WEST_KEY  'a'
#define UP_KEY    'e'
#define DOWN_KEY  'q'
// Interactive motion
#define MAX_VEL 2.7 // 2.77 m/s = 10 km/h
#define DEL_VEL 0.2

// ∑¬’Ê≈‰÷√.
struct option_t 
{
	option_t();

	char navfile[MAX_CHAR];
	char almfile[MAX_CHAR];
	char umfile[MAX_CHAR];
	int staticLocationMode;
	int nmeaGGA;
	int iduration;
	int verb;
	gpstime_t g0;
	double llh[3];
	int interactive;
	int timeoverwrite;
	int iono_enable;
	int path_loss_enable;
};

// ∑¬’Ê≥°æ∞.
struct sim_t 
{
	sim_t();

	int config(const std::vector<char*>& params);

	bool is_finished_generation();

	bool start_tx_task();
	bool start_gps_task();

	option_t opt;

	std::thread tx_thread;
	std::thread gps_thread;
	
	std::vector<fifo_t::data_type> tx_buffer;

	std::mutex init_mtx;
	std::mutex fifo_mtx;

	std::condition_variable initialization_done;
	
	bool gps_ready;

	int status;
	bool finished;
	fifo_t fifo;
	
	std::unique_ptr<Device> device;

	std::condition_variable fifo_read_ready;
	std::condition_variable fifo_write_ready;

	double time;
};

/// print usage message.
void usage(void);

/// Sim functions


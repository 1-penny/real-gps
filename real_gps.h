#pragma once
#include <stdint.h>

#include <vector>
#include <array>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "gpssim.h"

#define TX_FREQUENCY	1575420000
#define TX_SAMPLERATE	2600000

#define NUM_IQ_SAMPLES  (TX_SAMPLERATE / 10)
#define FIFO_LENGTH     (NUM_IQ_SAMPLES * 2)

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

struct tx_t {
	std::thread thread;
	std::mutex lock;
	//int error;

	struct bladerf* dev;
	std::vector<int16_t> buffer;
};

struct gps_t {
	std::thread thread;
	std::mutex lock;

	int ready;
	std::condition_variable initialization_done;
};

struct sim_t 
{
	sim_t();

	option_t opt;

	tx_t tx;
	gps_t gps;

	int status;
	bool finished;
	std::vector<int16_t> fifo;
	long head, tail;
	size_t sample_length;

	std::condition_variable fifo_read_ready;
	std::condition_variable fifo_write_ready;

	double time;
};


void usage(void);

/// Task functions
int start_tx_task(sim_t* s);
int start_gps_task(sim_t* s);

/// FIFO functions.
size_t get_sample_length(sim_t* s);
size_t fifo_read(int16_t* buffer, size_t samples, sim_t* s);
bool is_finished_generation(sim_t* s);
int is_fifo_write_ready(sim_t* s);

/// Device functions
int device_open(sim_t& s);
void device_close(sim_t& s);
int device_init(sim_t& s);
void device_send(sim_t* s);

/// Sim functions
void sim_init(sim_t* s);
int sim_config(sim_t& s, const std::vector<char*>& params);

#pragma once

#include <stdint.h>

#include <vector>
#include <array>

#include <memory>

#include <thread>
#include <mutex>
#include <condition_variable>

#include "gpssim.h"
#include "device.h"

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

struct tx_t 
{
	tx_t() : dev(nullptr)
	{
	}

	std::thread thread;
	std::mutex mtx;

	std::unique_ptr<Device> dev;
	std::unique_ptr<int16_t[]> buffer; // 长度为 32k个样点. SAMPLES_PER_BUFFER
};

struct gps_t 
{
	gps_t() : ready(false) 
	{
	}

	std::thread thread;
	std::mutex mtx;

	bool ready;
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
	std::unique_ptr<int16_t[]> fifo;
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

// 获取 FIFO 缓冲区当前数据长度. 
size_t fifo_get_sample_length(sim_t* s);

/** 从 FIFO 缓冲区中读取数据.
 * 读取完成后，会调整 FIFO 数据.
 * @return 实际读取的数据数量.
 */
size_t fifo_read(int16_t* buffer, size_t samples, sim_t* s);

/** 向 FIFO 缓冲区写入数据.
 */
size_t fifo_write(const int16_t* buffer, size_t samples, sim_t* s);

/** 
 * FIFO 缓冲区是否可以写入.
 * 判断条件为缓冲区可用长度不小于0.1s
 */
int is_fifo_write_ready(sim_t* s);

bool is_finished_generation(sim_t* s);

/// Sim functions
int sim_config(sim_t& s, const std::vector<char*>& params);
int sim_init(sim_t& s);
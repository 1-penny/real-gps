#define _CRT_SECURE_NO_WARNINGS

#include "real_gps.h"
#include "gps_tx.h"
#include "gpssim.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <fstream>

// for _getch used in Windows runtime.
#ifdef _WIN32
#include <conio.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif

std::string data_file_name = "gpssend.bin";
std::ofstream data_file;

const char* default_args = "-e brdc3540.14n -l 30,120,0 -d 10";

int main(int argc, char* argv[])
{
	if (argc < 3) {
		usage();
		exit(1);
	}
		
	std::vector<char*> params(argc);
	for (int i = 0; i < params.size(); i++) {
		params[i] = argv[i];
	}

	sim_t s;

	// Config simulator.
	s.status = sim_config(s, params);
	if (s.status != 0) {
		fprintf(stderr, "Failed to config sim.\n");
		goto out;
	}

	// Initialize simulator
	sim_init(&s);

	// Allocate TX buffer to hold each block of samples to transmit.
	s.tx.buffer.reset(new int16_t[SAMPLES_PER_BUFFER * 2]);
	//s.tx.buffer.resize(SAMPLES_PER_BUFFER * 2); // for 16-bit I and Q samples

	if (s.tx.buffer == nullptr) {
		fprintf(stderr, "Failed to allocate TX buffer.\n");
		goto out;
	}

	// Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
	s.fifo.reset(new int16_t[FIFO_LENGTH * 2]); // for 16-bit I and Q samples
	if (s.fifo.get() == nullptr) {
		fprintf(stderr, "Failed to allocate I/Q sample buffer.\n");
		goto out;
	}

	// Initializing device.
	printf("Opening and initializing device...\n");

	s.status = device_init(s);
	if (s.status != 0) {
		fprintf(stderr, "Failed to initialize device\n");
		goto out;
	}

	// Start GPS task.
	s.status = start_gps_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start GPS task.\n");
		goto out;
	}
	else
		printf("Creating GPS task...\n");

	// Wait until GPS task is initialized
	{
		std::unique_lock<std::mutex> lck(s.tx.lock);
		//pthread_mutex_lock(&(s.tx.lock));
		while (!s.gps.ready) {
			s.gps.initialization_done.wait(lck);
		}
		//pthread_mutex_unlock(&(s.tx.lock));
	}

	// Fillfull the FIFO.
	if (is_fifo_write_ready(&s)) {
		s.fifo_write_ready.notify_all();
	}

	// open the device
	s.status = device_open(s);
	if (s.status != 0) {
		goto out;
	}

	// Start TX task
	s.status = start_tx_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start TX task.\n");
		goto out;
	}
	else
		printf("Creating TX task...\n");

	// Running...
	printf("Running...\n");
	printf("Press 'q' to quit.\n");

	// Wainting for TX task to complete.
	s.gps.thread.join();
	s.tx.thread.join();

	printf("\nDone!\n");

out:
	device_close(s);
	return 0;
}

void usage(void)
{
	printf("Usage: bladegps [options]\n"
		"Options:\n"
		"  -e <gps_nav>     RINEX navigation file for GPS ephemerides (required)\n"
		"  -y <yuma_alm>    YUMA almanac file for GPS almanacs\n"
		"  -u <user_motion> User motion file (dynamic mode)\n"
		"  -g <nmea_gga>    NMEA GGA stream (dynamic mode)\n"
		"  -l <location>    Lat,Lon,Hgt (static mode) e.g. 35.274,137.014,100\n"
		"  -t <date,time>   Scenario start time YYYY/MM/DD,hh:mm:ss\n"
		"  -T <date,time>   Overwrite TOC and TOE to scenario start time\n"
		"  -d <duration>    Duration [sec] (max: %.0f)\n"
		"  -x <XB number>   Enable XB board, e.g. '-x 200' for XB200\n"
		"  -a <tx_vga1>     TX VGA1 (default: %d)\n"
		"  -i               Interactive mode: North='%c', South='%c', East='%c', West='%c'\n"
		"  -I               Disable ionospheric delay for spacecraft scenario\n"
		"  -p               Disable path loss and hold power level constant\n",
		((double)USER_MOTION_SIZE) / 10.0,
		TX_VGA1,
		NORTH_KEY, SOUTH_KEY, EAST_KEY, WEST_KEY);

	return;
}

int sim_config(sim_t& s, const std::vector<char*> & params)
{
	int argc = params.size();

	char * argv[32];
	for (int i = 0; i < argc; i++) {
		argv[i] = params[i];
	}

	int txvga1 = TX_VGA1;
	int result = 0;
	datetime_t t0;
	int xb_board = 0;
	double duration;

	while ((result = getopt(argc, argv, "e:y:u:g:l:T:t:d:x:a:iIp")) != -1)
	{
		switch (result)
		{
		case 'e':
			strcpy(s.opt.navfile, optarg);
			break;
		case 'y':
			strcpy(s.opt.almfile, optarg);
			break;
		case 'u':
			strcpy(s.opt.umfile, optarg);
			s.opt.nmeaGGA = FALSE;
			s.opt.staticLocationMode = FALSE;
			break;
		case 'g':
			strcpy(s.opt.umfile, optarg);
			s.opt.nmeaGGA = TRUE;
			s.opt.staticLocationMode = FALSE;
			break;
		case 'l':
			// Static geodetic coordinates input mode
			// Added by scateu@gmail.com
			s.opt.nmeaGGA = FALSE;
			s.opt.staticLocationMode = TRUE;
			sscanf(optarg, "%lf,%lf,%lf", &s.opt.llh[0], &s.opt.llh[1], &s.opt.llh[2]);
			s.opt.llh[0] /= R2D; // convert to RAD
			s.opt.llh[1] /= R2D; // convert to RAD
			break;
		case 'T':
			s.opt.timeoverwrite = TRUE;
			if (strncmp(optarg, "now", 3) == 0)
			{
				time_t timer;
				struct tm* gmt;

				time(&timer);
				gmt = gmtime(&timer);

				t0.y = gmt->tm_year + 1900;
				t0.m = gmt->tm_mon + 1;
				t0.d = gmt->tm_mday;
				t0.hh = gmt->tm_hour;
				t0.mm = gmt->tm_min;
				t0.sec = (double)gmt->tm_sec;

				date2gps(&t0, &s.opt.g0);

				break;
			}
		case 't':
			sscanf(optarg, "%d/%d/%d,%d:%d:%lf", &t0.y, &t0.m, &t0.d, &t0.hh, &t0.mm, &t0.sec);
			if (t0.y <= 1980 || t0.m < 1 || t0.m>12 || t0.d < 1 || t0.d>31 ||
				t0.hh < 0 || t0.hh>23 || t0.mm < 0 || t0.mm>59 || t0.sec < 0.0 || t0.sec >= 60.0)
			{
				printf("ERROR: Invalid date and time.\n");
				return -1;
			}
			t0.sec = floor(t0.sec);
			date2gps(&t0, &s.opt.g0);
			break;
		case 'd':
			duration = atof(optarg);
			if (duration<0.0 || duration>((double)USER_MOTION_SIZE) / 10.0)
			{
				printf("ERROR: Invalid duration.\n");
				return -1;
			}
			s.opt.iduration = (int)(duration * 10.0 + 0.5);
			break;
		case 'x':
			xb_board = atoi(optarg);
			break;
		case 'a':
			txvga1 = atoi(optarg);
			if (txvga1 > 0)
				txvga1 *= -1;
			break;
		case 'i':
			s.opt.interactive = TRUE;
			break;
		case 'I':
			s.opt.iono_enable = FALSE; // Disable ionospheric correction
			break;
		case 'p':
			s.opt.path_loss_enable = FALSE; // Disable path loss
			break;
		case ':':
		case '?':
			usage();
			return 1;
		default:
			break;
		}
	}

	if (s.opt.navfile[0] == 0)
	{
		printf("ERROR: GPS ephemeris file is not specified.\n");
		return 1;
	}

	if (s.opt.umfile[0] == 0 && !s.opt.staticLocationMode)
	{
		printf("ERROR: User motion file / NMEA GGA stream is not specified.\n");
		printf("You may use -l to specify the static location directly.\n");
		return 1;
	}

	return 0;
}

// 初始化仿真场景.
void sim_init(sim_t* s)
{
	s->tx.dev = NULL;
	//pthread_mutex_init(&(s->tx.lock), NULL);
	//s->tx.error = 0;

	//pthread_mutex_init(&(s->gps.lock), NULL);
	//s->gps.error = 0;
	s->gps.ready = 0;
	//pthread_cond_init(&(s->gps.initialization_done), NULL);

	//pthread_cond_init(&(s->fifo_write_ready), NULL);
	//pthread_cond_init(&(s->fifo_read_ready), NULL);

}

int device_open(sim_t& s)
{
	int status = 0;

	data_file.open(data_file_name.c_str(), std::ios::binary);
	if (!data_file.is_open()) {
		printf("Failed to open file.\n");
		return -1;
	}
	/*
	// We must always enable the modules *after* calling bladerf_sync_config().
	status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, true);
	if (status != 0) {
		fprintf(stderr, "Failed to enable TX module: %s\n", bladerf_strerror(status));
	}
	*/

	return status;
}

void device_close(sim_t& s)
{
	int status = 0;

	if (data_file.is_open()) {
		data_file.close();
		printf("Closing file...\n");
	}

	/**

	// Disable TX module and shut down underlying TX stream.
	status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, false);
	if (status != 0)
		fprintf(stderr, "Failed to disable TX module: %s\n", bladerf_strerror(s.status));

	printf("Closing device...\n");
	bladerf_close(s.tx.dev);
	*/
}

// 初始化设备.
int device_init(sim_t& s)
{
	int status = 0;

	/** Pickwick:
	char* devstr = NULL;

	status = bladerf_open(&s.tx.dev, devstr);
	if (status != 0) {
		fprintf(stderr, "Failed to open device: %s\n", bladerf_strerror(status));
		return status;
	}

	if (xb_board == 200) {
		printf("Initializing XB200 expansion board...\n");

		status = bladerf_expansion_attach(s.tx.dev, BLADERF_XB_200);
		if (status != 0) {
			fprintf(stderr, "Failed to enable XB200: %s\n", bladerf_strerror(status));
			return status;
		}

		status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_CUSTOM);
		if (status != 0) {
			fprintf(stderr, "Failed to set XB200 TX filterbank: %s\n", bladerf_strerror(status));
			return status;
		}

		status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_BYPASS);
		if (status != 0) {
			fprintf(stderr, "Failed to enable TX bypass path on XB200: %s\n", bladerf_strerror(status));
			return status;
		}

		//For sake of completeness set also RX path to a known good state.
		status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_CUSTOM);
		if (status != 0) {
			fprintf(stderr, "Failed to set XB200 RX filterbank: %s\n", bladerf_strerror(status));
			return status;
		}

		status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_BYPASS);
		if (status != 0) {
			fprintf(stderr, "Failed to enable RX bypass path on XB200: %s\n", bladerf_strerror(status));
			return status;
		}
	}

	if (xb_board == 300) {
		fprintf(stderr, "XB300 does not support transmitting on GPS frequency\n");
		return status;
	}

	status = bladerf_set_frequency(s.tx.dev, BLADERF_MODULE_TX, TX_FREQUENCY);
	if (status != 0) {
		fprintf(stderr, "Faield to set TX frequency: %s\n", bladerf_strerror(status));
		return status;
	}
	else {
		printf("TX frequency: %u Hz\n", TX_FREQUENCY);
	}

	status = bladerf_set_sample_rate(s.tx.dev, BLADERF_MODULE_TX, TX_SAMPLERATE, NULL);
	if (status != 0) {
		fprintf(stderr, "Failed to set TX sample rate: %s\n", bladerf_strerror(status));
		return status;
	}
	else {
		printf("TX sample rate: %u sps\n", TX_SAMPLERATE);
	}

	status = bladerf_set_bandwidth(s.tx.dev, BLADERF_MODULE_TX, TX_BANDWIDTH, NULL);
	if (status != 0) {
		fprintf(stderr, "Failed to set TX bandwidth: %s\n", bladerf_strerror(status));
		return status;
	}
	else {
		printf("TX bandwidth: %u Hz\n", TX_BANDWIDTH);
	}

	if (txvga1 < BLADERF_TXVGA1_GAIN_MIN)
		txvga1 = BLADERF_TXVGA1_GAIN_MIN;
	else if (txvga1 > BLADERF_TXVGA1_GAIN_MAX)
		txvga1 = BLADERF_TXVGA1_GAIN_MAX;

	//status = bladerf_set_txvga1(s.tx.dev, TX_VGA1);
	status = bladerf_set_txvga1(s.tx.dev, txvga1);
	if (status != 0) {
		fprintf(stderr, "Failed to set TX VGA1 gain: %s\n", bladerf_strerror(status));
		return status;
	}
	else {
		//printf("TX VGA1 gain: %d dB\n", TX_VGA1);
		printf("TX VGA1 gain: %d dB\n", txvga1);
	}

	status = bladerf_set_txvga2(s.tx.dev, TX_VGA2);
	if (status != 0) {
		fprintf(stderr, "Failed to set TX VGA2 gain: %s\n", bladerf_strerror(status));
		return status;
	}
	else {
		printf("TX VGA2 gain: %d dB\n", TX_VGA2);
	}

	// Configure the TX module for use with the synchronous interface.
	status = bladerf_sync_config(s.tx.dev,
		BLADERF_MODULE_TX,
		BLADERF_FORMAT_SC16_Q11,
		NUM_BUFFERS,
		SAMPLES_PER_BUFFER,
		NUM_TRANSFERS,
		TIMEOUT_MS);

	if (status != 0) {
		fprintf(stderr, "Failed to configure TX sync interface: %s\n", bladerf_strerror(s.status));
		return status;
	}

	*/

	return 0;
}

void device_send(sim_t* s)
{
	// If there were no errors, transmit the data buffer.
	//bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);
	if (data_file.is_open()) {
		data_file.write((const char *) s->tx.buffer.get(), SAMPLES_PER_BUFFER * 4);
	}
}


size_t get_sample_length(sim_t* s)
{
	long length;

	length = s->head - s->tail;
	if (length < 0)
		length += FIFO_LENGTH;

	return((size_t)length);
}

size_t fifo_read(int16_t* buffer, size_t samples, sim_t* s)
{
	size_t length;
	size_t samples_remaining;
	int16_t* buffer_current = buffer;

	length = get_sample_length(s);

	if (length < samples)
		samples = length;

	length = samples; // return value

	samples_remaining = FIFO_LENGTH - s->tail;

	if (samples > samples_remaining) {
		memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples_remaining * sizeof(int16_t) * 2);
		s->tail = 0;
		buffer_current += samples_remaining * 2;
		samples -= samples_remaining;
	}

	memcpy(buffer_current, &(s->fifo[s->tail * 2]), samples * sizeof(int16_t) * 2);
	s->tail += (long)samples;
	if (s->tail >= FIFO_LENGTH)
		s->tail -= FIFO_LENGTH;

	return(length);
}

bool is_finished_generation(sim_t* s)
{
	return s->finished;
}

int is_fifo_write_ready(sim_t* s)
{
	int status = 0;

	s->sample_length = get_sample_length(s);
	if (s->sample_length < NUM_IQ_SAMPLES)
		status = 1;

	return(status);
}

int start_tx_task(sim_t* s)
{
	int status = 1;

	//status = pthread_create(&(s->tx.thread), NULL, tx_task, s);
	s->tx.thread = std::thread(tx_task, s);

	return(status);
}

int start_gps_task(sim_t* s)
{
	int status = 1;

	//status = pthread_create(&(s->gps.thread), NULL, gps_task, s);
	s->gps.thread = std::thread(gps_task, s);

	return(status);
}




option_t::option_t()
{
	navfile[0] = 0;
	almfile[0] = 0;
	umfile[0] = 0;
	g0.week = -1;
	g0.sec = 0.0;
	iduration = USER_MOTION_SIZE;
	verb = TRUE;
	nmeaGGA = FALSE;
	staticLocationMode = TRUE; // default user motion
	llh[0] = 40.7850916 / R2D;
	llh[1] = -73.968285 / R2D;
	llh[2] = 100.0;
	interactive = FALSE;
	timeoverwrite = FALSE;
	iono_enable = TRUE;
	path_loss_enable = TRUE;
}

sim_t::sim_t()
{
	finished = false;

	status = 0;
	head = 0;
	tail = 0;
	sample_length = 0;

	time = 0.0;
}
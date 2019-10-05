#define _CRT_SECURE_NO_WARNINGS

#include "real_gps.h"
#include "gps_tx.h"
#include "gpssim.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

// for _getch used in Windows runtime.
#ifdef _WIN32
#include <conio.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif

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
		return -1;
	}

	// Initialize simulator.
	s.status = sim_init(s);
	if (s.status != 0) {
		fprintf(stderr, "Failed to init sim.\n");
		return -1;
	}

	// Initializing device.
	printf("Opening and initializing device...\n");
	
	s.tx.dev.reset(new FileDevice());
	if (!s.tx.dev) {
		fprintf(stderr, "Failed to create device\n");
		return -1;
	}

	s.status = s.tx.dev->init();
	if (s.status != 0) {
		fprintf(stderr, "Failed to initialize device\n");
		return -1;
	}

	// open the device
	s.status = s.tx.dev->open();
	if (s.status != 0) {
		return -1;
	}

	

	// Start GPS task.
	s.status = start_gps_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start GPS task.\n");
		return -1;
	}
	else
		printf("Creating GPS task...\n");

	// Wait until GPS task is initialized
	std::unique_lock<std::mutex> lck(s.tx.mtx);
	while (!s.gps.ready) {
		s.gps.initialization_done.wait(lck);
	}
	
	// Fillfull the FIFO.
	if (s.fifo.is_write_ready()) {
		s.fifo_write_ready.notify_all();
	}

	// Start TX task
	s.status = start_tx_task(&s);
	if (s.status < 0) {
		fprintf(stderr, "Failed to start TX task.\n");
		return -1;
	}
	else
		printf("Creating TX task...\n");
	
	// Running...
	printf("Running...\n");
	printf("Press 'q' to quit.\n");

	// Wainting for TX task to complete.
	s.tx.thread.join();
	s.gps.thread.join();

	printf("\nDone!\n");

	s.tx.dev->close();
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

int sim_init(sim_t& s)
{
	// Allocate TX buffer to hold each block of samples to transmit.
	s.tx.buffer.reset(new int16_t[SAMPLES_PER_BUFFER * 2]); // for 16-bit I and Q samples

	if (s.tx.buffer == nullptr) {
		fprintf(stderr, "Failed to allocate TX buffer.\n");
		return -1;
	}

	// Allocate FIFOs to hold 0.1 seconds of I/Q samples each.
	s.fifo.reset(FIFO_LENGTH, 2);// for 16-bit I and Q samples
	if (! s.fifo.ready()) {
		fprintf(stderr, "Failed to allocate I/Q sample buffer.\n");
		return -1;
	}

	return 0;
}

fifo_t::fifo_t()
{
	head = 0;
	tail = 0;
	step = 1;
	size = 0;
}

bool fifo_t::reset(int size2, int step2)
{
	if (size2 > 0 && step2 > 0) {
		this->size = size2;
		this->step = step2;
		fifo.reset(new int16_t[size2 * step2]);
	}

	return ready();
}

bool fifo_t::ready()
{
	return (fifo.get() != nullptr) && (size > 0) && (step > 0);
}

size_t fifo_t::get_sample_length()
{
	long length = head - tail;
	if (length < 0)
		length += FIFO_LENGTH;

	return (size_t) length;
}

size_t fifo_t::read(int16_t* buff, size_t num)
{
	int16_t* buffer_current = buff;
	size_t length = get_sample_length();
	int samples = num;

	if (length < samples)
		samples = length;

	length = samples; // return value

	size_t samples_remaining = FIFO_LENGTH - tail;

	if (samples > samples_remaining) {
		memcpy(buffer_current, &(fifo[tail * step]), samples_remaining * sizeof(int16_t) * step);
		tail = 0;
		buffer_current += samples_remaining * step;
		samples -= samples_remaining;
	}

	memcpy(buffer_current, &(fifo[tail * step]), samples * sizeof(int16_t) * step);
	tail += (long)samples;
	if (tail >= FIFO_LENGTH)
		tail -= FIFO_LENGTH;

	return length;
}

size_t fifo_t::write(const int16_t* buff, size_t samples)
{
	memcpy(&(fifo[head * step]), buff, samples * step * sizeof(short));

	head += (long) samples;
	
	if (head > FIFO_LENGTH)
		head -= FIFO_LENGTH;

	if (head >= FIFO_LENGTH)
		head -= FIFO_LENGTH;

	return samples;
}

bool fifo_t::is_write_ready()
{
	int sample_length = get_sample_length();
	return (sample_length < NUM_IQ_SAMPLES);
}


bool is_finished_generation(sim_t* s)
{
	return s->finished;
}


int start_tx_task(sim_t* s)
{
	s->tx.thread = std::thread(tx_task, s);
	return 1;
}

int start_gps_task(sim_t* s)
{
	s->gps.thread = std::thread(gps_task, s);
	return 1;
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

	time = 0.0;
}
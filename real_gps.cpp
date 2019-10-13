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
#include "device.h"

// for _getch used in Windows runtime.
#ifdef _WIN32
#include <conio.h>
#include "getopt.h"
#else
#include <unistd.h>
#endif

std::string data_file_name = "c:/data/gpssend_7.bin";
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

	/// 配置模拟环境.
	if (s.config(params) != 0) {
		fprintf(stderr, "Failed to config sim.\n");
		return -1;
	}

	/// 初始化和配置 device.
	printf("Opening and initializing device...\n");

	s.device.reset(Device::make("file", { {"verbose", "1"} }));
	if (!s.device) {
		fprintf(stderr, "Failed to create device\n");
		return -1;
	}

	std::map<std::string, std::string> dev_params = {
		{"filename", "c:/data/gpssend_7.bin"},
		{"tx.freq", "1575420000"},
		{"tx.rate", "2600000"},
		{"tx.gain", "40"}
	};
	if (!s.device->set_params(dev_params)) {
		fprintf(stderr, "Failed to initialize device\n");
		return -1;
	}

	/// 启动 GPS 任务.
	if (!s.start_gps_task()) {
		fprintf(stderr, "Failed to start GPS task.\n");
		return -1;
	}
	else {
		printf("Creating GPS task...\n");
	}

	/// 等待 GPS 任务初始化完成.
	std::unique_lock<std::mutex> lock(s.init_mtx);
	while (!s.gps_ready) {
		s.initialization_done.wait(lock);
	}
	lock.unlock();

	// Fillfull the FIFO.
	if (s.fifo.is_write_ready()) {
		s.fifo_write_ready.notify_all();
	}

	// 打开设备.
	if (!s.device->open()) {
		return -1;
	}

	/// 启动发送任务.
	if (! s.start_tx_task()) {
		fprintf(stderr, "Failed to start TX task.\n");
		return -1;
	}
	else {
		printf("Creating TX task...\n");
	}

	// Running...
	printf("Running...\n");
	printf("Press 'q' to quit.\n");

	// 等待发送任务、GPS任务完成.
	s.tx_thread.join();
	s.gps_thread.join();

	printf("\nDone!\n");

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

int sim_t::config(const std::vector<char*>& params)
{
	int argc = params.size();

	char* argv[32];
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
			strcpy(opt.navfile, optarg);
			break;
		case 'y':
			strcpy(opt.almfile, optarg);
			break;
		case 'u':
			strcpy(opt.umfile, optarg);
			opt.nmeaGGA = FALSE;
			opt.staticLocationMode = FALSE;
			break;
		case 'g':
			strcpy(opt.umfile, optarg);
			opt.nmeaGGA = TRUE;
			opt.staticLocationMode = FALSE;
			break;
		case 'l':
			// Static geodetic coordinates input mode
			// Added by scateu@gmail.com
			opt.nmeaGGA = FALSE;
			opt.staticLocationMode = TRUE;
			sscanf(optarg, "%lf,%lf,%lf", &opt.llh[0], &opt.llh[1], &opt.llh[2]);
			opt.llh[0] /= R2D; // convert to RAD
			opt.llh[1] /= R2D; // convert to RAD
			break;
		case 'T':
			opt.timeoverwrite = TRUE;
			if (strncmp(optarg, "now", 3) == 0)
			{
				time_t timer;
				struct tm* gmt;

				::time(&timer);
				gmt = gmtime(&timer);

				t0.y = gmt->tm_year + 1900;
				t0.m = gmt->tm_mon + 1;
				t0.d = gmt->tm_mday;
				t0.hh = gmt->tm_hour;
				t0.mm = gmt->tm_min;
				t0.sec = (double)gmt->tm_sec;

				date2gps(&t0, &opt.g0);

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
			date2gps(&t0, &opt.g0);
			break;
		case 'd':
			duration = atof(optarg);
			if (duration<0.0 || duration>((double)USER_MOTION_SIZE) / 10.0)
			{
				printf("ERROR: Invalid duration.\n");
				return -1;
			}
			opt.iduration = (int)(duration * 10.0 + 0.5);
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
			opt.interactive = TRUE;
			break;
		case 'I':
			opt.iono_enable = FALSE; // Disable ionospheric correction
			break;
		case 'p':
			opt.path_loss_enable = FALSE; // Disable path loss
			break;
		case ':':
		case '?':
			usage();
			return 1;
		default:
			break;
		}
	}

	if (opt.navfile[0] == 0)
	{
		printf("ERROR: GPS ephemeris file is not specified.\n");
		return 1;
	}

	if (opt.umfile[0] == 0 && !opt.staticLocationMode)
	{
		printf("ERROR: User motion file / NMEA GGA stream is not specified.\n");
		printf("You may use -l to specify the static location directly.\n");
		return 1;
	}

	return 0;
}


sim_t::sim_t() : fifo(FIFO_LENGTH, NUM_IQ_SAMPLES)
{
	finished = false;
	status = 0;
	time = 0.0;
	gps_ready = false;

	// Allocate TX buffer to hold each block of samples to transmit.
	tx_buffer.resize(SAMPLES_PER_BUFFER); //NUM_IQ_SAMPLES);//  for 16-bit I and Q samples

	if (tx_buffer.size() > 0) {
		fprintf(stderr, "Failed to allocate TX buffer.\n");
	}
}

bool sim_t::is_finished_generation()
{
	return finished;
}


bool sim_t::start_tx_task()
{
	tx_thread = std::thread(tx_task, this);
	return true;
}

bool sim_t::start_gps_task()
{
	gps_thread = std::thread(gps_task, this);
	return true;
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

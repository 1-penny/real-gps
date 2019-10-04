#ifndef _BLADEGPS_H
#define _BLADEGPS_H

#include <stdlib.h>
#include <stdio.h>
//#include <libbladeRF.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef _WIN32
// To avoid conflict between time.h and pthread.h on Windows
#define HAVE_STRUCT_TIMESPEC
#endif
#include "gpssim.h"


#define TX_BANDWIDTH	2500000
#define TX_VGA1			-25
#define TX_VGA2			0

#define NUM_BUFFERS			32
#define SAMPLES_PER_BUFFER	(32 * 1024)
#define NUM_TRANSFERS		16
#define TIMEOUT_MS			1000


void tx_task(void* arg);

#endif

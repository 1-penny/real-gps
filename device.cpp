#include "device.h"

Device::~Device()
{
	close();
}

//
//int device_open(sim_t& s)
//{
//	int status = 0;
//
//	data_file.open(data_file_name.c_str(), std::ios::binary);
//	if (!data_file.is_open()) {
//		printf("Failed to open file.\n");
//		return -1;
//	}
//	/*
//	// We must always enable the modules *after* calling bladerf_sync_config().
//	status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, true);
//	if (status != 0) {
//		fprintf(stderr, "Failed to enable TX module: %s\n", bladerf_strerror(status));
//	}
//	*/
//
//	return status;
//}
//
//void device_close(sim_t& s)
//{
//	int status = 0;
//
//	if (data_file.is_open()) {
//		data_file.close();
//		printf("Closing file...\n");
//	}
//
//	/**
//
//	// Disable TX module and shut down underlying TX stream.
//	status = bladerf_enable_module(s.tx.dev, BLADERF_MODULE_TX, false);
//	if (status != 0)
//		fprintf(stderr, "Failed to disable TX module: %s\n", bladerf_strerror(s.status));
//
//	printf("Closing device...\n");
//	bladerf_close(s.tx.dev);
//	*/
//}
//
//// 初始化设备.
//int device_init(sim_t& s)
//{
//	int status = 0;
//
//	/** Pickwick:
//	char* devstr = NULL;
//
//	status = bladerf_open(&s.tx.dev, devstr);
//	if (status != 0) {
//		fprintf(stderr, "Failed to open device: %s\n", bladerf_strerror(status));
//		return status;
//	}
//
//	if (xb_board == 200) {
//		printf("Initializing XB200 expansion board...\n");
//
//		status = bladerf_expansion_attach(s.tx.dev, BLADERF_XB_200);
//		if (status != 0) {
//			fprintf(stderr, "Failed to enable XB200: %s\n", bladerf_strerror(status));
//			return status;
//		}
//
//		status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_CUSTOM);
//		if (status != 0) {
//			fprintf(stderr, "Failed to set XB200 TX filterbank: %s\n", bladerf_strerror(status));
//			return status;
//		}
//
//		status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_TX, BLADERF_XB200_BYPASS);
//		if (status != 0) {
//			fprintf(stderr, "Failed to enable TX bypass path on XB200: %s\n", bladerf_strerror(status));
//			return status;
//		}
//
//		//For sake of completeness set also RX path to a known good state.
//		status = bladerf_xb200_set_filterbank(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_CUSTOM);
//		if (status != 0) {
//			fprintf(stderr, "Failed to set XB200 RX filterbank: %s\n", bladerf_strerror(status));
//			return status;
//		}
//
//		status = bladerf_xb200_set_path(s.tx.dev, BLADERF_MODULE_RX, BLADERF_XB200_BYPASS);
//		if (status != 0) {
//			fprintf(stderr, "Failed to enable RX bypass path on XB200: %s\n", bladerf_strerror(status));
//			return status;
//		}
//	}
//
//	if (xb_board == 300) {
//		fprintf(stderr, "XB300 does not support transmitting on GPS frequency\n");
//		return status;
//	}
//
//	status = bladerf_set_frequency(s.tx.dev, BLADERF_MODULE_TX, TX_FREQUENCY);
//	if (status != 0) {
//		fprintf(stderr, "Faield to set TX frequency: %s\n", bladerf_strerror(status));
//		return status;
//	}
//	else {
//		printf("TX frequency: %u Hz\n", TX_FREQUENCY);
//	}
//
//	status = bladerf_set_sample_rate(s.tx.dev, BLADERF_MODULE_TX, TX_SAMPLERATE, NULL);
//	if (status != 0) {
//		fprintf(stderr, "Failed to set TX sample rate: %s\n", bladerf_strerror(status));
//		return status;
//	}
//	else {
//		printf("TX sample rate: %u sps\n", TX_SAMPLERATE);
//	}
//
//	status = bladerf_set_bandwidth(s.tx.dev, BLADERF_MODULE_TX, TX_BANDWIDTH, NULL);
//	if (status != 0) {
//		fprintf(stderr, "Failed to set TX bandwidth: %s\n", bladerf_strerror(status));
//		return status;
//	}
//	else {
//		printf("TX bandwidth: %u Hz\n", TX_BANDWIDTH);
//	}
//
//	if (txvga1 < BLADERF_TXVGA1_GAIN_MIN)
//		txvga1 = BLADERF_TXVGA1_GAIN_MIN;
//	else if (txvga1 > BLADERF_TXVGA1_GAIN_MAX)
//		txvga1 = BLADERF_TXVGA1_GAIN_MAX;
//
//	//status = bladerf_set_txvga1(s.tx.dev, TX_VGA1);
//	status = bladerf_set_txvga1(s.tx.dev, txvga1);
//	if (status != 0) {
//		fprintf(stderr, "Failed to set TX VGA1 gain: %s\n", bladerf_strerror(status));
//		return status;
//	}
//	else {
//		//printf("TX VGA1 gain: %d dB\n", TX_VGA1);
//		printf("TX VGA1 gain: %d dB\n", txvga1);
//	}
//
//	status = bladerf_set_txvga2(s.tx.dev, TX_VGA2);
//	if (status != 0) {
//		fprintf(stderr, "Failed to set TX VGA2 gain: %s\n", bladerf_strerror(status));
//		return status;
//	}
//	else {
//		printf("TX VGA2 gain: %d dB\n", TX_VGA2);
//	}
//
//	// Configure the TX module for use with the synchronous interface.
//	status = bladerf_sync_config(s.tx.dev,
//		BLADERF_MODULE_TX,
//		BLADERF_FORMAT_SC16_Q11,
//		NUM_BUFFERS,
//		SAMPLES_PER_BUFFER,
//		NUM_TRANSFERS,
//		TIMEOUT_MS);
//
//	if (status != 0) {
//		fprintf(stderr, "Failed to configure TX sync interface: %s\n", bladerf_strerror(s.status));
//		return status;
//	}
//
//	*/
//
//	return 0;
//}
//
//void device_send(sim_t* s)
//{
//	// If there were no errors, transmit the data buffer.
//	//bladerf_sync_tx(s->tx.dev, s->tx.buffer, SAMPLES_PER_BUFFER, NULL, TIMEOUT_MS);
//	if (data_file.is_open()) {
//		data_file.write((const char*)s->tx.buffer.get(), SAMPLES_PER_BUFFER * 4);
//	}
//}

int FileDevice::open(void* param)
{
	m_file.close();

	m_file.open("c:/data/gpssend_7.bin", std::ios::binary);
	return m_file.is_open() ? 0 : -1;
}

void FileDevice::close()
{
	if (m_file.is_open()) {
		m_file.flush();
		m_file.close();
	}
}

int FileDevice::send(void* data, unsigned int count, unsigned int elemsize, unsigned int timeout)
{
	if (!m_file.is_open())
		return -1;

	if (!data || count <= 0 || elemsize <= 0)
		return -1;

	m_file.write((const char *) data, count * elemsize);
	return count;
}

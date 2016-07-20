// system headers
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>

// RTL-SDR USB Radio header
#include <rtl-sdr.h>

typedef struct {
  rtlsdr_dev_t *_dev;
  int _devIndex;
  bool _autoGainEnabled;
  int _gain;
  int _freq;
} AppData_t;

AppData_t AppData;

int main() {
	
	if(rtlsdr_open(&AppData._dev, 0) >= 0) {
		std::cout << "it works?";
	}
	else {
	  fprintf(stderr, "Error opening the RTLSDR device: %s\n", strerror(errno));
	}
	
	rtlsdr_close(AppData._dev);
	
	return 0;
}
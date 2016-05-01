/*
 * Arduino Serial2InfluxDB
 * -----------------------
 *
 * Created 1 May 2016
 * Copyleft (c) 2016, Mariano Finochietto, <mariano.fino@gmail.com>
 * More info: <https://github.com/marianofino/arduino-serial2influxdb>
 *
 * Based on the work from:
 *   - CanonicalArduinoRead: <http://www.chrisheydrick.com>
 *     <https://github.com/cheydrick/Canonical-Arduino-Read>
 *   - Tod E. Kurt, <tod@todbot.com> <http://todbot.com/blog/>
 *     <https://github.com/todbot/arduino-serial>
**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <curl/curl.h>
#include <getopt.h>
#include <signal.h>

// Global vars
int fd = 0;
CURL *curl;

// Print usage information
void usage(void);
// Setup serial terminal
int setup_serialport(const char* serialport, int baud);
// Read from fd
double read_serialport(int fd);
// Send to InfluxDB through CURL
void send_data(char * buffer, CURL * curl, char * url);
// Close everything and quit
void quit_program(int signal);


int main(int argc, char *argv[])
{
  int i = 0, n = 0, option_index = 0, opt;
  char serialport[256], measurement[256] = {'\0'}, url[256] = {'\0'};
  static struct option loptions[] = {
      {"help",        no_argument,       0, 'h'},
      {"port",        required_argument, 0, 'p'},
      {"num",         required_argument, 0, 'n'},
      {"measurement", required_argument, 0, 'm'}
  };

  if (argc == 1) {
      usage();
      exit(EXIT_SUCCESS);
  }

  // Catch Ctrl+C
  struct sigaction act;
  act.sa_handler = quit_program;
  sigaction(SIGINT, &act, NULL);

  while(1) {
      opt = getopt_long(argc, argv, "hp:n:m:u:", loptions, &option_index);
      if (opt == -1) break;
      switch (opt) {
        case '0': break;
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
            break;
        case 'p':
            strcpy(serialport, optarg);
            fd = setup_serialport(optarg, B9600);
            if(fd == -1)
              exit(EXIT_FAILURE);
            break;
        case 'n':
            n = strtol(optarg, NULL, 10); // convert string to number
            break;
        case 'm':
            strcpy(measurement, optarg);
            break;
        case 'u':
            strcpy(url, optarg);
            break;
      }
  }

  // Check that we have the min requirements to continue
  if (fd == 0 || measurement[0] == '\0' || url[0] == '\0') {
      usage();
      quit_program(0);
  }

  // Init CURL
  curl = curl_easy_init();

  if(curl) {
    while (i < n || n == 0) {
      // Read from Arduino
      char buffer[50];
      double data = read_serialport(fd);
      sprintf(buffer, "%s value=%f", measurement, data);
      // Send to InfluxDB
      send_data(buffer, curl, url);
      i++;
    }
  }

  exit(EXIT_SUCCESS); 
}

void usage(void) {
    printf("Usage: arduino-serial2influxdb -p <serialport> -m <name,key=value,...> [options]\n"
    "\n"
    "Options:\n"
    "  -h, --help\t\t\t\tPrint this help message\n"
    "  -p, --port=serialport\t\t\tSerial port Arduino is on\n"
    "  -n, --n=number\t\t\tNumber of times to read from\n\t\t\t\t\tArduino; 0 means infinite\n"
    "  -m, --measurement=name,key=value,...\tMeasurement name and tags\n"
    "  -u, --url=url:port/write?db=mydb\tComplete address to write in db\n"
    "\n");
}

int setup_serialport(const char* serialport, int baud) {
  struct termios toptions;
  int fd;

  // Open serial port
  fd = open(serialport, O_RDWR | O_NOCTTY);
  if (fd == -1)  {
      perror("setup_serialport Unable to open port ");
      return -1;
  }

  // Wait for the Arduino to reboot
  usleep(1500000);
  
  // Get current serial port settings
  tcgetattr(fd, &toptions);
  // Set 9600 baud both ways
  cfsetispeed(&toptions, B9600);
  cfsetospeed(&toptions, B9600);
  // 8 bits, no parity, no stop bits
  toptions.c_cflag &= ~PARENB;
  toptions.c_cflag &= ~CSTOPB;
  toptions.c_cflag &= ~CSIZE;
  toptions.c_cflag |= CS8;
  // Canonical mode */
  toptions.c_lflag |= ICANON;
  // Commit the serial port settings
  tcsetattr(fd, TCSANOW, &toptions);

  if( tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
      perror("setup_serialport: Couldn't set term attributes");
      return -1;
  }

  return fd;
}

double read_serialport(int fd) {
  char buf[64] = "temp text";
  int i = read(fd, buf, 64);
  buf[i] = 0;

  return atof(buf);
}

void send_data(char * buffer, CURL * curl, char * url) {
  CURLcode res;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  // Now specify the POST data
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);

  // Perform the request, res will get the return code
  res = curl_easy_perform(curl);
  // Check for errors
  if(res != CURLE_OK) {
    printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    quit_program(0);
  }
}

void quit_program(int signal) {
  curl_easy_cleanup(curl);
  close(fd);
  exit(EXIT_SUCCESS);
}

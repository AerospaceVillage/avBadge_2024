#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/timerfd.h>

#include "ledDaemon.h"

#define FILE_PATH "/var/run/ledFiFo"

static double brigDouble = 1.0;  // Default max brightness
static int interrupted = 0;

static void handler(int sig) {
    switch (sig) {
        case SIGHUP:
            syslog(LOG_INFO, "Received SIGHUP signal.\n");
            // Handle SIGHUP (e.g., reload configuration)
            break;
        case SIGINT:
        case SIGTERM:
            interrupted = 1;
            break;
        default:
            syslog(LOG_ERR, "Unexpected signal: %d\n", sig);
    }
}

static inline int invalid_int(char c) {
    return c < '0' || c > '9';
}

static inline int decode_hexchar(char c, uint8_t* val_out) {
    if (c >= '0' && c <= '9') {
        *val_out = c - '0';
        return 0;
    }
    else if (c >= 'A' && c <= 'F') {
        *val_out = c - 'A' + 10;
        return 0;
    }
    else if (c >= 'a' && c <= 'f') {
        *val_out = c - 'a' + 10;
        return 0;
    }
    else {
        return -1;
    }
}

static inline int decode_hexbyte(const char *c, uint8_t* val_out) {
    uint8_t val;
    uint8_t nibble;
    if (decode_hexchar(c[0], &nibble)) {
        return -1;
    }
    val = nibble << 4;
    if (decode_hexchar(c[1], &nibble)) {
        return -1;
    }
    val |= nibble;
    *val_out = val;
    return 0;
}

static int decode_hex24(const char* str, uint8_t* val_out) {
    // Decodes 24 bit hexidecimal value
    if (decode_hexbyte(str, &val_out[0])) {
        return -1;
    }
    if (decode_hexbyte(str + 2, &val_out[1])) {
        return -1;
    }
    if (decode_hexbyte(str + 4, &val_out[2])) {
        return -1;
    }
    return 0;
}

static void process_command(const char* command) {
    // syslog(LOG_DEBUG, "Got Command: '%s'\n", command);

    if (command[0] == 'C' && command[1] == 'L' &&command[2] == 'E' && command[3] == 'A' &&command[4] == 'R'
        && command[5] == 'A' && command[6] == 'L' && command[7] == 'L' && command[8] == 0){
        // CLEARALL
        if (rgbled_clearallRing()) {
            // Note we're just going to pass this to clearall ring since we are using the battery LEDs
            syslog(LOG_WARNING, "Clear RGB LEDs: %s\n", strerror(errno));
        }
        return;
    }

    if (command[0] == 'C' && command[1] == 'L' &&command[2] == 'E' && command[3] == 'A' && command[4] == 'R'
            && command[5] == 'A' && command[6] == 'L' && command[7] == 'L' && command[8] == 'R' && command[9] == 0){
        // CLEARALLR
        if (rgbled_clearallRing()) {
            syslog(LOG_WARNING, "Clear RGB LED Ring: %s\n", strerror(errno));
        }
        return;
    }

    /*
    // Disable clearing battery LEDs, we're going to directly control them
    if (command[0] == 'C' && command[1] == 'L' &&command[2] == 'E' && command[3] == 'A' && command[4] == 'R'
            && command[5] == 'A' && command[6] == 'L' && command[7] == 'L' && command[8] == 'B' && command[9] == 0){
        // CLEARALLB
        if (rgbled_clearallBat()) {
            syslog(LOG_WARNING, "Clear Batt LEDs: %s\n", strerror(errno));
        }
        continue;
    }
    */

    // Process the command LED...
    if (command[0] == 'L' && command[1] == 'E' && command[2] == 'D'){

        // LEDBRIG
        if(command[3]== 'B' && command[4] == 'R' && command[5] == 'I' && command[6] == 'G'){
            if (invalid_int(command[7])) {
                syslog(LOG_NOTICE, "Bad LED Brightness: '%s'\n", command);
            }
            else if (command[8]) {
                syslog(LOG_NOTICE, "Invalid LED Brightness Command: '%s'\n", command);
            }
            else {
                // LEDBRIG# command
                // Brightness between 0-9
                brigDouble = (double)(command[7] - '0') / 9.0;// Normalize to 0.0 - 1.0
                syslog(LOG_INFO, "Setting brightness to: %.1f\n", brigDouble);
            }
            return;
        }

        // LEDBATEN
        if (command[3] == 'B' && command[4] == 'A' && command[5] == 'T' && command[6] == 'E' && command[7] == 'N') {
            if ((command[8] != '0' && command[8] != '1') || command[9] != 0) {
                syslog(LOG_NOTICE, "Bad LED Battery Enable Command: '%s'\n", command);
            }
            else {
                bool en = (command[8] == '1');
                batt_set_enable(en);
                syslog(LOG_INFO, "%s Battery LEDs\n", (en ? "Enabling" : "Disabling"));
            }
            return;
        }

        // Set LED RGB

        // LED##... commands (where ## is LED number)
        if (invalid_int(command[3]) || invalid_int(command[4])) {
            // Next 2 characters must be numbers
            syslog(LOG_NOTICE, "Bad LED Command: '%s'\n", command);
            return;
        }
        unsigned int ledNum = (command[3] - '0') * 10;
        ledNum += (command[4] - '0');
        if (ledNum >= LED_RING_COUNT) {
            // Invalid LED number
            syslog(LOG_NOTICE, "Bad LED Number: '%d'\n", ledNum);
            return;
        }

        /// if RGB
        if(command[5] == 'R' && command[6] == 'G' && command[7] == 'B'){
            uint8_t rgb8[3];
            if (decode_hex24(&command[8], rgb8) || command[14]) {
                syslog(LOG_NOTICE, "Bad RGB Command: '%s'\n", command);
                return;
            }

            rgb ledColorRGB = {};
            ledColorRGB.r = ((double)rgb8[0] / 255.0) * brigDouble;
            ledColorRGB.g = ((double)rgb8[1] / 255.0) * brigDouble;
            ledColorRGB.b = ((double)rgb8[2] / 255.0) * brigDouble;
            if (rgbled_set(ledNum, ledColorRGB)) {
                syslog(LOG_WARNING, "Failed to set RGB LEDs with command '%s': %s\n", command, strerror(errno));
            }
        }

        // if HSV format
        else if(command[5] == 'H' && command[6] == 'S' && command[7] == 'V'){
            uint8_t hsv8[3];
            if (decode_hex24(&command[8], hsv8) || command[14]) {
                syslog(LOG_NOTICE, "Bad RGB Command: '%s'\n", command);
                return;
            }

            hsv ledColorHSV = {};
            ledColorHSV.h = ((double)hsv8[0] / 255.0) * brigDouble;
            ledColorHSV.s = ((double)hsv8[1] / 255.0) * brigDouble;
            ledColorHSV.v = ((double)hsv8[2] / 255.0) * brigDouble;
            if (rgbled_set(ledNum, hsv2rgb(ledColorHSV))) {
                syslog(LOG_WARNING, "Failed to set RGB LEDs with command '%s': %s\n", command, strerror(errno));
            }
        }
        else {
            syslog(LOG_NOTICE, "Invalid LED Subcommand: '%s'\n", command);
            return;
        }
    }
    else {
        syslog(LOG_NOTICE, "Unknown Command: '%s'\n", command);
    }
}

int main(int argc, char** argv) {
    // Initialize syslog
    openlog("ledDaemon", LOG_PID | LOG_NDELAY, LOG_LOCAL0);

    int make_daemon = 1;

    if (argc >= 2) {
        if (!strcmp(argv[1], "--no-daemon")) {
            make_daemon = 0;
        }
    }

    if (make_daemon) {
        // Create a new daemon process
        int ret = daemon(0, 0);
        if (ret < 0) {
            syslog(LOG_ERR, "daemon() failed: %s\n", strerror(errno));
            exit(1);
        }
    }

    syslog(LOG_INFO, "LED Daemon Started\n");

    // Handle signals
    signal(SIGHUP, handler);
    signal(SIGTERM, handler);
    signal(SIGINT, handler);

    // Open FIFO for communication
    // Open the FIFO in non-blocking mode
    // Needs to be opened read/write so poll doesn't get HUPs on close
    int fifoFd = open(FILE_PATH, O_RDWR | O_NONBLOCK);
    if (fifoFd < 0) {
        if (errno == ENOENT) {
            // FIFO does not exist, wait for it to be created
            if (mkfifo(FILE_PATH, 0666)) {
                syslog(LOG_ERR,"Fail FIFO: %s\n", strerror(errno));
                exit(1);
            }
            syslog(LOG_INFO, "Created new FIFO...\n");
            fifoFd = open(FILE_PATH, O_RDWR | O_NONBLOCK);
        }

        if (fifoFd < 0) {
            syslog(LOG_ERR, "Failed to open FIFO: %s\n", strerror(errno));
            exit(1);
        }
    }

    // Create Timer FD for battery monitoring
    int timerFd = timerfd_create(CLOCK_BOOTTIME, O_NONBLOCK);
    if (timerFd < 0) {
        syslog(LOG_ERR, "Failed to create timer fd: %s\n", strerror(errno));
        exit(1);
    }

    // Start the timer
    struct itimerspec spec = {
        .it_interval = { 0, 750 * 1000000 }, // Runs every 750 ns
        .it_value = { 0, 100 * 1500000 }     // Fires after 150 ms (to clear rgbled_testbatt())
    };
    if (timerfd_settime(timerFd, 0, &spec, NULL)) {
        syslog(LOG_ERR, "Failed to start timer fd: %s\n", strerror(errno));
        exit(1);
    }


    // Initialize LEDs
    if (rgbled_init()) {
        syslog(LOG_ERR, "Failed to initialize LEDs: %s\n", strerror(errno));
        exit(1);
    }

    // Flash test pattern on battery LEDs
    if (regbled_testbatt()) {
        syslog(LOG_ERR, "Failed to run LED test: %s\n", strerror(errno));
        rgbled_cleanup();
        exit(1);
    }


    // Create Poll descriptors
    #define POLLFD_COUNT 2
    struct pollfd fds[POLLFD_COUNT] = {};
    fds[0].fd = fifoFd;
    fds[0].events = POLLIN;
    fds[1].fd = timerFd;
    fds[1].events = POLLIN;

    // Buffers for receiving
    char read_buf[256];
    char command[16];
    int command_idx = 0;
    int command_overflowed = 0;

    while (!interrupted) {
        int ret;

        ret = poll(fds, POLLFD_COUNT, -1);
        if (interrupted) {
            break;
        }

        if (ret < 0) {
            syslog(LOG_ERR, "Poll failed: %s\n", strerror(errno));
            break;
        }

        // Handle timer fd ready
        if (fds[1].revents & POLLIN) {
            ret = read(timerFd, read_buf, sizeof(read_buf));
            if (ret <= 0) {
                syslog(LOG_ERR, "Timer FD read failed: %s\n", strerror(errno));
                break;
            }

            batt_update_leds();
        }

        // Handle fifo FD ready
        if (fds[0].revents & POLLIN) {
            ret = read(fifoFd, read_buf, sizeof(read_buf));
            if (ret > 0) {
                // Process the incoming buffer
                for (int i = 0; i < ret; i++) {
                    char c = read_buf[i];
                    // Allow either null character or newline to terminate commands
                    if (c == 0 || c == '\n') {
                        // Only process command if we didn't overflow the buffer and it's not empty
                        if (!command_overflowed && command_idx > 0) {
                            // Terminate command
                            command[command_idx] = 0;

                            // Process it
                            process_command(command);
                        }
                        else if (command_overflowed) {
                            syslog(LOG_NOTICE, "Command dropped: message too long\n");
                        }

                        // Reset state
                        command_overflowed = 0;
                        command_idx = 0;
                    }
                    else {
                        // Every other character
                        if (command_overflowed || command_idx + 1 >= sizeof(command)) {
                            // Not enough space, mark overflow and to drop this command after separator
                            command_overflowed = 1;
                        }
                        else {
                            // Store character
                            command[command_idx++] = c;
                        }
                    }
                }
            }
        }
    }

    if (interrupted)
        syslog(LOG_NOTICE, "Received SIGTERM/SIGINT signal.\n");

    rgbled_cleanup();
    close(fifoFd);
    syslog(LOG_INFO, "Daemon exiting.\n");
    closelog();
    return 0;
}

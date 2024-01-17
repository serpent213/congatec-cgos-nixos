/* cgexporter
 *
 * Basic Prometheus exporter for CGOS hardware monitor
 *
 * This is just a quick hack, handle it with velvet gloves. And sorry, no IPv6 support.
 *
 * 2024 Steffen Beyer
 *
 * License: Public Domain
 *
 */

#include <arpa/inet.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Cgos.h>
#include <bsd/string.h>
#include <microhttpd.h>

#define METRIC_HELP_TEXT_TABLE_SIZE 4
#define RESPONSE_BUFFER_SIZE (5 * 200)

volatile sig_atomic_t termFlag = 0; // shutdown flag (SIGTERM, SIGINT)
HCGOS hCgos = 0; // global CGOS access

// Response "metrics" generation

struct helpTextEntry {
    const char *metric_name;
    const char *help_text;
};

struct helpTextEntry metricHelpTextTable[METRIC_HELP_TEXT_TABLE_SIZE] = {
    {"congatec_boot_counter", "Number of system boots"},
    {"congatec_operating_hours", "Number of operating hours"},
    {"congatec_temp_celsius", "Hardware monitor for temperature"},
    {"congatec_voltage_volts", "Hardware monitor for voltage"}
};

const char* getHelpText(const char *metric_name) {
    for (int i = 0; i < METRIC_HELP_TEXT_TABLE_SIZE; i++) {
        if (strcmp(metric_name, metricHelpTextTable[i].metric_name) == 0) {
            return metricHelpTextTable[i].help_text;
        }
    }
    return NULL;
}

int generateCounterMetric(char *buffer, size_t buffer_size, const char *metric_name, unsigned long value) {
    const char *help_text = getHelpText(metric_name);
    if (help_text == NULL) return -1;
    snprintf(buffer, buffer_size,
             "# HELP %s %s\n"
             "# TYPE %s counter\n"
             "%s %lu\n\n",
             metric_name, help_text,
             metric_name,
             metric_name, value);
    return 0;
}

int generateGaugeMetric(char *buffer, size_t buffer_size, const char *metric_name, const char *label_name, double value) {
    const char *help_text = getHelpText(metric_name);
    if (help_text == NULL) return -1;
    snprintf(buffer, buffer_size,
             "# HELP %s %s\n"
             "# TYPE %s gauge\n"
             "%s{name=\"%s\"} %.3f\n\n",
             metric_name, help_text,
             metric_name,
             metric_name, label_name, value);
    return 0;
}

char *generateMetrics() {
    char buffer[200];
    char *response = malloc(RESPONSE_BUFFER_SIZE);
    if (response == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    response[0] = 0;

    unsigned int dwBootCounter;
    if (CgosBoardGetBootCounter(hCgos, &dwBootCounter) &&
        generateCounterMetric(buffer, sizeof(buffer), "congatec_boot_counter", dwBootCounter) == 0) {
        strlcat(response, buffer, RESPONSE_BUFFER_SIZE);
    }

    unsigned int dwRunningTime;
    if (CgosBoardGetRunningTimeMeter(hCgos, &dwRunningTime) &&
        generateCounterMetric(buffer, sizeof(buffer), "congatec_operating_hours", dwRunningTime) == 0) {
        strlcat(response, buffer, RESPONSE_BUFFER_SIZE);
    }

    CGOSTEMPERATUREINFO temperatureInfo = {0};
    temperatureInfo.dwSize = sizeof(temperatureInfo);
    unsigned int dwUnit, setting, status;
    unsigned int monCount = CgosTemperatureCount(hCgos);

    if (monCount > 0) {
        for (dwUnit = 0; dwUnit < monCount; dwUnit++) {
            if (!CgosTemperatureGetInfo(hCgos, dwUnit, &temperatureInfo)) continue;
            if (temperatureInfo.dwType != CGOS_TEMP_BOARD || !CgosTemperatureGetCurrent(hCgos, dwUnit, &setting, &status)) continue;
            if (generateGaugeMetric(buffer, sizeof(buffer), "congatec_temp_celsius", "Board", (double)setting / 1000.0) != 0) continue;
            strlcat(response, buffer, RESPONSE_BUFFER_SIZE);
            break; // only one temperature available, so we break early
        }
    }

    CGOSVOLTAGEINFO voltageInfo = {0};
    voltageInfo.dwSize = sizeof(voltageInfo);
    monCount = CgosVoltageCount(hCgos);

    if (monCount > 0) {
        for (dwUnit = 0; dwUnit < monCount; dwUnit++) {
            if (!CgosVoltageGetInfo(hCgos, dwUnit, &voltageInfo)) continue;
            if (voltageInfo.dwType == CGOS_VOLTAGE_5V_S0 && CgosVoltageGetCurrent(hCgos, dwUnit, &setting, &status)) {
                if (generateGaugeMetric(buffer, sizeof(buffer), "congatec_voltage_volts", "5V_S0", (double)setting / 1000.0) != 0) continue;
                strlcat(response, buffer, RESPONSE_BUFFER_SIZE);
            } else if (voltageInfo.dwType == CGOS_VOLTAGE_5V_S5 && CgosVoltageGetCurrent(hCgos, dwUnit, &setting, &status)) {
                if (generateGaugeMetric(buffer, sizeof(buffer), "congatec_voltage_volts", "5V_S5", (double)setting / 1000.0) != 0) continue;
                strlcat(response, buffer, RESPONSE_BUFFER_SIZE);
            }
        }
    }

    return response;
}

// HTTP request handler

static enum MHD_Result
answerToConnection(void *cls, struct MHD_Connection *connection,
                     const char *url, const char *method,
                     const char *version, const char *upload_data,
                     size_t *upload_data_size, void **con_cls) {
    struct MHD_Response *response;
    enum MHD_Result ret;

    const char *metrics_text = generateMetrics();
    response = MHD_create_response_from_buffer(strlen(metrics_text), (void*)metrics_text, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "text/plain; version=0.0.4");
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

// Error handler

void errorExit(const char *message, ...) {
    va_list args;
    char prefixed[200];

    snprintf(prefixed, sizeof(prefixed), "cgexporter: %s", message);
    va_start(args, message);
    vfprintf(stderr, prefixed, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

// CGOS handling

void openCgos() {
    if (!CgosLibInitialize()) {
        errorExit("Could not open CGOS library, aborting.\n");
    }
    if (!CgosBoardOpen(0, 0, 0, &hCgos)) {
        errorExit("Could not open CGOS board, aborting.\n");
        CgosLibUninitialize();
    }
}

void closeCgos() {
    if (hCgos) CgosBoardClose(hCgos);
    CgosLibUninitialize();
}

// SIGTERM handler

void termHandler(int signum) {
    termFlag = 1;
}

// Entry point

int main(int argc, char *argv[]) {
    struct MHD_Daemon *daemon;
    struct sockaddr_in server_addr = {0};
    int port = 9699;
    const char *host = "127.0.0.1";

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("Usage: cgexporter [<options>]\n\n"
                   "Options:\n\n"
                   "    -h, --help      Show this help message and exit\n"
                   "    -p PORT         Set the port number (default 9699)\n"
                   "    -H HOST         Set the server host (default 127.0.0.1)\n");
            return EXIT_SUCCESS;
        } else if (!strcmp(argv[i], "-p") && i < argc - 1) {
            port = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-H") && i < argc - 1) {
            host = argv[++i];
        } else {
            errorExit("Invalid options. See \"%s --help\".\n", argv[0]);
        }
    }

    // Setup HTTP server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    // The following line should be replaced by a line that converts the host string to an IP address (getaddrinfo).
    server_addr.sin_addr.s_addr = inet_addr(host);

    daemon = MHD_start_daemon(MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD,
                              -1, NULL, NULL,
                              &answerToConnection, NULL,
                              MHD_OPTION_SOCK_ADDR, &server_addr,
                              MHD_OPTION_END);
    if (daemon == NULL) {
        errorExit("Failed to start HTTP server.\n");
        return EXIT_FAILURE;
    }

    openCgos();

    // Drop privileges
    struct passwd* pwd;
    if ((pwd = getpwnam("nobody")) == NULL) errorExit("getpwnam() failed");
    if (setgid(pwd->pw_gid) < 0) errorExit("Failed to set gid");
    if (setuid(pwd->pw_uid) < 0) errorExit("Failed to set uid");

    // Handle SIGTERM to allow clean shutdown
    struct sigaction action = {0};
    sigemptyset(&action.sa_mask);
    action.sa_handler = termHandler;
    if (sigaction(SIGTERM, &action, NULL) < 0) errorExit("Cannot register SIGTERM handler\n");
    if (sigaction(SIGINT, &action, NULL) < 0) errorExit("Cannot register SIGINT handler\n");

    // Event loop
    printf("Server running at http://%s:%d/ (uid=%d, gid=%d)\n", host, port, getuid(), getgid());

    while (!termFlag)
        sleep(1);

    closeCgos();
    MHD_stop_daemon(daemon);

    printf("Clean shutdown\n");
    return EXIT_SUCCESS;
}

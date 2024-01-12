/* cgctl
 *
 * Some practical Congatec CGOS functionality (Linux only)
 *
 * 2023 Steffen Beyer
 *
 * License: Public Domain
 *
 */

#define _ISOC99_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <jansson.h>
#include <Cgos.h>

HCGOS hCgos = 0; // global CGOS access

// prototypes

void doHelp();
void doInfo(bool jsonOutput);
void doWatchdog(bool doDisable, bool doTrigger, bool jsonOutput);
void errorExit(const char *message, ...);

void openCgos();
void closeCgos();
void reportBoard(json_t *root);
void reportTemperatures(json_t *root);
void reportVoltages(json_t *root);

// entry point

int main(int argc, char* argv[]) {
    // default command
    char* cmd = "help";

    if (argc > 1) {
        cmd = argv[1];
    }

    if(strcmp(cmd, "help") == 0) {
        doHelp();

    } else if (strcmp(cmd, "info") == 0) {
        bool jsonOutput = false;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-j") == 0) {
                jsonOutput = true;
            } else {
                errorExit("Unknown option for \"info\": %s\n", argv[i]);
            }
        }
        doInfo(jsonOutput);

    } else if (strcmp(cmd, "wdog") == 0) {
        bool doDisable = false, doTrigger = false, jsonOutput = false;
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-d") == 0) {
                doDisable = true;
            } else if (strcmp(argv[i], "-t") == 0) {
                doTrigger = true;
            } else if (strcmp(argv[i], "-j") == 0) {
                jsonOutput = true;
            } else {
                errorExit("Unknown option for \"wdog\": %s\n", argv[i]);
            }
        }
        if (!(doDisable || doTrigger)) {
          fprintf(stderr, "cgctl: Invalid options.\n\n");
          doHelp();
          return 1;
        }
        doWatchdog(doDisable, doTrigger, jsonOutput);

    } else {
        fprintf(stderr, "cgctl: Invalid command.\n\n");
        doHelp();
        return 1;
    }

    return 0;
}

// main command handlers and error handler

void doHelp() {
    printf(
        "Usage: cgctl <command> [option]\n"
        "\n"
        "Commands:\n"
        "\n"
        "    help        Show this help text\n"
        "\n"
        "    info        Get and display various board information\n"
        "\n"
        "                    -j    Output in JSON format (default)\n"
        "\n"
        "    wdog        Watchdog access\n"
        "\n"
        "                    -d    Disable watchdog\n"
        "                    -t    Trigger watchdog\n"
        "                    -j    Output in JSON format (default)\n"
    );
}

void doInfo(bool jsonOutput) {
    json_t *root = json_object();

    openCgos();

    reportBoard(root);
    reportTemperatures(root);
    reportVoltages(root);

    // output will always be JSON, "-j" can be used for forward compatibility
    char *jsonString = json_dumps(root, JSON_INDENT(2) | JSON_REAL_PRECISION(3));
    printf("%s\n", jsonString);

    free(jsonString);
    json_decref(root);

    closeCgos();
}

void doWatchdog(bool doDisable, bool doTrigger, bool jsonOutput) {
    const unsigned int WDOG_UNIT = 0;

    openCgos();
    if (!CgosWDogIsAvailable(hCgos, WDOG_UNIT)) errorExit("No watchdog available, aborting.\n");

    bool success = false;
    if (doDisable) {
        success = CgosWDogDisable(hCgos, WDOG_UNIT);
    } else if (doTrigger) {
        success = CgosWDogTrigger(hCgos, WDOG_UNIT);
    }

    printf("{\"Success\": %s}\n", success ? "true" : "false");
    closeCgos();
}

void errorExit(const char *message, ...) {
    va_list args;
    char prefixed[strlen(message) + 8];

    snprintf(prefixed, sizeof(prefixed), "cgctl: %s", message);
    va_start(args, message);
    vfprintf(stderr, prefixed, args);
    va_end(args);
    exit(1);
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

// report generation (info)

void reportBoard(json_t *root) {
    CGOSBOARDINFOA bi;
    bi.dwSize = sizeof(bi);
    char szBoardName[CGOS_BOARD_MAX_SIZE_ID_STRING];
    unsigned int dwBootCounter;
    unsigned int dwRunningTime;
    json_t *boardData = json_object();

    if (CgosBoardGetNameA(hCgos, szBoardName, sizeof(szBoardName))) {
        json_object_set_new(boardData, "BoardName", json_string(szBoardName));
    }

    if (CgosBoardGetInfoA(hCgos, &bi)) {
        json_object_set_new(boardData, "SerialNumber", json_string(bi.szSerialNumber));
        char mfgDate[18];
        snprintf(mfgDate, sizeof(mfgDate), "%04u/%02u/%02u", bi.stManufacturingDate.wYear, bi.stManufacturingDate.wMonth, bi.stManufacturingDate.wDay);
        json_object_set_new(boardData, "ManufacturingDate", json_string(mfgDate));
    }

    if (CgosBoardGetBootCounter(hCgos, &dwBootCounter)) {
        json_object_set_new(boardData, "BootCounter", json_integer(dwBootCounter));
    }

    if (CgosBoardGetRunningTimeMeter(hCgos, &dwRunningTime)) {
        json_object_set_new(boardData, "OperatingHours", json_integer(dwRunningTime));
    }

    json_object_set_new(root, "Board", boardData);
}

void reportTemperatures(json_t *root) {
    CGOSTEMPERATUREINFO temperatureInfo = {0};
    temperatureInfo.dwSize = sizeof(temperatureInfo);
    unsigned int dwUnit, setting, status;
    unsigned int monCount = CgosTemperatureCount(hCgos);
    json_t *temperatureData = json_object();

    if (monCount > 0) {
        for (dwUnit = 0; dwUnit < monCount; dwUnit++) {
            if (CgosTemperatureGetInfo(hCgos, dwUnit, &temperatureInfo)) {
                if (temperatureInfo.dwType == CGOS_TEMP_BOARD && CgosTemperatureGetCurrent(hCgos, dwUnit, &setting, &status)) {
                    json_t *tempJSON = json_real((double)setting / 1000.0);
                    json_object_set_new(temperatureData, "Board", tempJSON);
                    break; // only one temperature available, so we break early
                }
            }
        }
    }

    json_object_set_new(root, "Temperatures", temperatureData);
}

void reportVoltages(json_t *root) {
  CGOSVOLTAGEINFO voltageInfo = {0};
  voltageInfo.dwSize = sizeof(voltageInfo);
  unsigned int dwUnit, setting, status;
  unsigned int monCount = CgosVoltageCount(hCgos);
  json_t *voltageData = json_object();
  
  if (monCount > 0) {
      for (dwUnit = 0; dwUnit < monCount; dwUnit++) {
          if (CgosVoltageGetInfo(hCgos, dwUnit, &voltageInfo)) {
              if (voltageInfo.dwType == CGOS_VOLTAGE_5V_S0 && CgosVoltageGetCurrent(hCgos, dwUnit, &setting, &status)) {
                  json_t *voltageJSON = json_real((double)setting / 1000.0);
                  json_object_set_new(voltageData, "5V_S0", voltageJSON);
              } else if (voltageInfo.dwType == CGOS_VOLTAGE_5V_S5 && CgosVoltageGetCurrent(hCgos, dwUnit, &setting, &status)) {
                  json_t *voltageJSON = json_real((double)setting / 1000.0);
                  json_object_set_new(voltageData, "5V_S5", voltageJSON);
              }
          }
      }
  }

  json_object_set_new(root, "Voltages", voltageData);
}


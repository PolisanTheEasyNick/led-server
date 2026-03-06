#include "config.h"
#include "../globals/globals.h"
#include "../utils/utils.h"
#include <getopt.h>
#include <libconfig.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int update_config_int(const char *setting_name, int new_value) {
    if (access(config_file, W_OK) != 0) {
        fprintf(stderr, "No write permission for config file: %s\n", config_file);
        return -1;
    }

    config_t cfg;
    config_setting_t *setting;

    config_init(&cfg);

    if (!config_read_file(&cfg, config_file)) {
        fprintf(stderr, "Config Error: %s:%d - %s\n",
                config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    setting = config_lookup(&cfg, setting_name);

    if (setting != NULL) {
        config_setting_set_int(setting, new_value);
    } else {
        setting = config_setting_add(config_root_setting(&cfg), setting_name, CONFIG_TYPE_INT);
        config_setting_set_int(setting, new_value);
    }

    if (!config_write_file(&cfg, config_file)) {
        fprintf(stderr, "Error while writing to file: %s\n", config_file);
        config_destroy(&cfg);
        return -1;
    }

    config_destroy(&cfg);
    return 0;
}

uint8_t parse_config(const char *config_file) {
    config_t cfg;
    config_init(&cfg);

    if (!config_read_file(&cfg, config_file)) {
        logger_debug(PARSER, "Error reading config file: %s\n", config_error_text(&cfg));
        config_destroy(&cfg);
        return 1;
    }
    logger_debug(PARSER, "Opened config file");
#ifndef ORGBCONFIGURATOR
    const char *addr;
    if (!config_lookup_string(&cfg, "PI_ADDR", &addr)) {
        logger(PARSER, "Missing PI_ADDR in config file, using default (NULL)\n");
        PI_ADDR = NULL;
    } else {
        PI_ADDR = malloc(strlen(addr) + 1);
        strncpy(PI_ADDR, addr, strlen(addr));
        PI_ADDR[strlen(addr)] = 0;
    }

    const char *port;
    if (!config_lookup_string(&cfg, "PI_PORT", &port)) {
        logger(PARSER, "Missing PI_PORT in config file, using default (8888)\n");
        PI_PORT = NULL;
    } else {
        PI_PORT = malloc(strlen(port) + 1);
        strncpy(PI_PORT, port, strlen(port));
        PI_PORT[strlen(port)] = 0;
    }

    if (!config_lookup_int(&cfg, "RED_PIN", &RED_PIN)) {
        logger(PARSER, "Missing RED_PIN in config file!\n");
        config_destroy(&cfg);
        exit(EXIT_FAILURE);
    }

    if (!config_lookup_int(&cfg, "GREEN_PIN", &GREEN_PIN)) {
        logger(PARSER, "Missing GREEN_PIN in config file!\n");
        config_destroy(&cfg);
        exit(EXIT_FAILURE);
    }
    if (!config_lookup_int(&cfg, "BLUE_PIN", &BLUE_PIN)) {
        logger(PARSER, "Missing BLUE_PIN in config file!\n");
        config_destroy(&cfg);
        exit(EXIT_FAILURE);
    }
    const char *secret;
    if (!config_lookup_string(&cfg, "SHARED_SECRET", &secret)) {
        logger(PARSER, "Missing SHARED_SECRET in config file\n");
        SHARED_SECRET = NULL;
        return -1;
    }
    SHARED_SECRET = malloc(strlen(secret) + 1);
    strncpy(SHARED_SECRET, secret, strlen(secret));
    SHARED_SECRET[strlen(secret)] = 0;
#endif
    const char *openrgb_addr;
    if (!config_lookup_string(&cfg, "OPENRGB_SERVER", &openrgb_addr)) {
        logger(PARSER, "Missing OPENRGB_SERVER in config file\n");
        OPENRGB_SERVER = NULL;
    } else {
        OPENRGB_SERVER = malloc(strlen(openrgb_addr) + 1);
        strncpy(OPENRGB_SERVER, openrgb_addr, strlen(openrgb_addr));
        OPENRGB_SERVER[strlen(openrgb_addr)] = 0;
    }

    if (!config_lookup_int(&cfg, "OPENRGB_PORT", &OPENRGB_PORT)) {
        logger(PARSER, "Missing OPENRGB_PORT in config file, using default 6742\n");
        OPENRGB_PORT = 6742;
    }

    int default_red = 0, default_green = 0, default_blue = 0;
    if (!config_lookup_int(&cfg, "DEFAULT_RED", &default_red)) {
        logger(PARSER, "Missing DEFAULT_RED in config file, using default 0\n");
    }
    if (!config_lookup_int(&cfg, "#DEFAULT_GREEN", &default_green)) {
        logger(PARSER, "Missing #DEFAULT_GREEN in config file, using default 0\n");
    }
    if (!config_lookup_int(&cfg, "#DEFAULT_BLUE", &default_blue)) {
        logger(PARSER, "Missing #DEFAULT_BLUE in config file, using default 0\n");
    }
    logger(PARSER, "Parsed default color: %i, %i, %i\n", default_red, default_green, default_blue);
    DEFAULT_COLOR.RED = default_red;
    DEFAULT_COLOR.GREEN = default_green;
    DEFAULT_COLOR.BLUE = default_blue;



#ifndef ORGBCONFIGURATOR
    logger(PARSER,
           "Passed config:\nRaspberry Pi address: %s\nPort: %s\nRed pin: %d\nGreen pin: %d\nBlue pin: %d\nShared "
           "secret: %s\nOpenRGB server: %s\nOpenRGB Port: %d\n",
           PI_ADDR, PI_PORT, RED_PIN, GREEN_PIN, BLUE_PIN, SHARED_SECRET, OPENRGB_SERVER, OPENRGB_PORT);
#endif
    config_destroy(&cfg);
    return 0;
}

void parse_args(int argc, char *argv[]) {
    int opt;
    static struct option long_options[] = {{"server", required_argument, 0, 's'},
                                           {"port", required_argument, 0, 'p'},
                                           {"RED", required_argument, 0, 'R'},
                                           {"GREEN", required_argument, 0, 'G'},
                                           {"BLUE", required_argument, 0, 'B'},
                                           {"SHARED_SECRET", required_argument, 0, 'S'},
                                           {"OPENRGB_SERVER", required_argument, 0, 'O'},
                                           {"OPENRGB_PORT", required_argument, 0, 'P'},
                                           {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "c:s:p:R:G:B:S:O:P:", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            snprintf(PI_ADDR, sizeof(PI_ADDR), "%s", optarg);
            logger(PARSER, "RPi Server address set to: %s", PI_ADDR);
            break;
        case 'p':
            snprintf(PI_PORT, sizeof(PI_PORT), "%s", optarg);
            logger(PARSER, "Server port set to: %d", PI_PORT);
            break;
        case 'R':
            RED_PIN = atoi(optarg);
            logger(PARSER, "Red pin set to: %d", RED_PIN);
            break;
        case 'G':
            GREEN_PIN = atoi(optarg);
            logger(PARSER, "Green pin set to: %d", GREEN_PIN);
            break;
        case 'B':
            BLUE_PIN = atoi(optarg);
            logger(PARSER, "Blue pin set to: %d", BLUE_PIN);
            break;
        case 'S':
            snprintf(SHARED_SECRET, sizeof(SHARED_SECRET), "%s", optarg);
            logger(PARSER, "Shared secret set to: %s", SHARED_SECRET);
            break;
        case 'O': {
            snprintf(OPENRGB_SERVER, sizeof(OPENRGB_SERVER), "%s", optarg);
            logger(PARSER, "OpenRGB server address set to: %s", OPENRGB_SERVER);
            break;
        }
        case 'P': {
            OPENRGB_PORT = atoi(optarg);
            logger(PARSER, "OpenRGB Server port set to: %d", OPENRGB_PORT);
            break;
        }
        default:
            logger(PARSER, "Unknown option or missing argument. Exiting.");
            exit(EXIT_FAILURE);
        }
    }
}

uint8_t try_load_config(const char *config_path) {
    logger_debug(PARSER, "Trying to load config from: %s", config_path);
    return parse_config(config_path);
}

int load_config() {
    char *home_path = getenv("HOME");

    if (home_path) {
        snprintf(config_file, sizeof(config_file), "%s%s", home_path, "/.config/piled.conf");
        if (try_load_config(config_file) == 0) {
            return 0;
        }
    }

    if (try_load_config("/etc/piled/piled.conf") != 0) {
        logger(PARSER, "Can't load any of the configs! Aborting.");
        return -1;
    }

    strcpy(config_file, "/etc/piled/piled.conf");


    return 0;
}

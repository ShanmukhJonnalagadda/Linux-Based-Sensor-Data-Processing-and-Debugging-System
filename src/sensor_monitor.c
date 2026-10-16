/*
 * Linux C Sensor Frame Monitor
 * Demo protocol: START(0xAA) | SENSOR_ID | TEMP(int8_t) | HUMIDITY | XOR
 * The XOR checksum is computed over the first four bytes.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define FRAME_LEN 5
#define START_BYTE 0xAA

typedef struct {
    unsigned int valid;
    unsigned int rejected;
} Statistics;

static int parse_frame(const char *line, uint8_t frame[FRAME_LEN])
{
    unsigned int values[FRAME_LEN];
    char extra;
    int matched = sscanf(line, " %x %x %x %x %x %c",
                         &values[0], &values[1], &values[2],
                         &values[3], &values[4], &extra);
    if (matched != FRAME_LEN) {
        return 0;
    }
    for (int i = 0; i < FRAME_LEN; ++i) {
        if (values[i] > UINT8_MAX) {
            return 0;
        }
        frame[i] = (uint8_t)values[i];
    }
    return 1;
}

static uint8_t calculate_checksum(const uint8_t frame[FRAME_LEN])
{
    uint8_t value = 0;
    for (int i = 0; i < FRAME_LEN - 1; ++i) {
        value ^= frame[i];
    }
    return value;
}

/* Use GDB on the buggy build to inspect raw=251 (0xFB). */
static int decode_temperature(uint8_t raw)
{
#ifdef INTRODUCE_BUG
    int temperature_c = (int)raw;           /* BUG: -5 C turns into +251 C */
#else
    int temperature_c = (int)(int8_t)raw;   /* Fix: treat byte as signed */
#endif
    return temperature_c;
}

static void process_frame(const uint8_t frame[FRAME_LEN],
                          unsigned int line_no, Statistics *stats)
{
    if (frame[0] != START_BYTE) {
        printf("ERROR line=%u reason=start-byte\n", line_no);
    } else if (calculate_checksum(frame) != frame[4]) {
        printf("ERROR line=%u reason=checksum\n", line_no);
    } else if (frame[1] == 0) {
        printf("ERROR line=%u reason=sensor-id\n", line_no);
    } else {
        int temperature_c = decode_temperature(frame[2]);
        unsigned int humidity = frame[3];
        if (temperature_c < -40 || temperature_c > 85) {
            printf("ERROR line=%u reason=temperature-range value=%dC\n",
                   line_no, temperature_c);
        } else if (humidity > 100) {
            printf("ERROR line=%u reason=humidity-range\n", line_no);
        } else {
            printf("OK line=%u sensor=%u temp=%dC humidity=%u%%\n",
                   line_no, (unsigned int)frame[1], temperature_c, humidity);
            ++stats->valid;
            return;
        }
    }
    ++stats->rejected;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hex-frame-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        perror("Cannot open frame file");
        return EXIT_FAILURE;
    }

    Statistics stats = {0, 0};
    char line[256];
    unsigned int line_no = 0;
    while (fgets(line, sizeof line, input) != NULL) {
        ++line_no;
        const unsigned char *ptr = (const unsigned char *)line;
        while (isspace(*ptr)) {
            ++ptr;
        }
        if (*ptr == '#' || *ptr == '\0') {
            continue;
        }

        uint8_t frame[FRAME_LEN];
        if (!parse_frame(line, frame)) {
            printf("ERROR line=%u reason=malformed-frame\n", line_no);
            ++stats.rejected;
            continue;
        }
        process_frame(frame, line_no, &stats);
    }
    if (ferror(input)) {
        perror("Error reading frame file");
        fclose(input);
        return EXIT_FAILURE;
    }
    fclose(input);
    printf("Summary: valid=%u rejected=%u\n", stats.valid, stats.rejected);
    return EXIT_SUCCESS;
}

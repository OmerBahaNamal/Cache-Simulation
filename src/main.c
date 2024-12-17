#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
//helper structs
#include "helper_structs/request.h"
#include "helper_structs/result.h"

extern struct Result run_simulation(
        int cycles,
        int directMapped,
        unsigned cacheLines,
        unsigned cacheLineSize,
        unsigned cacheLatency,
        unsigned memoryLatency,
        size_t numRequests,
        struct Request* requests,
        const char* tracefile);

const char *usage_msg =
        "Usage: %s [OPTIONS] <inputFile>   Run cache simulation with given operations in inputFile\n"
        "   or: %s -h                      Show help message and exit\n";

const char *help_msg =
        "Positional arguments:\n"
        "  inputFile   The file to get operations. Must be a .csv file.\n"
        "\n"
        "Optional arguments:                (Default: 32KB directmapped L1 cache)\n"
        "  -c, --cycles <number>            Number of cycles to simulate (Default: 1000000000)\n"
        "      --directmapped               Simulate a direct-mapped cache. Can't set with --fourway option simultaneously (Default: directmapped)\n"
        "      --fourway                    Simulate a four-way associative cache. Can't set with --directmapped option simultaneously (Default: directmapped)\n"
        "      --cacheline-size <number>    Size of a cache line in bytes (Default: 64)\n"
        "      --cachelines <number>        Number of cache lines (Default: 512)\n"
        "      --cache-latency <number>     Cache latency in cycles (Default: 1)\n"
        "      --memory-latency <number>    Memory latency in cycles (Default: 200)\n"
        "      --tf=<filename>              Output trace file with all signals\n"
        "  -h, --help                       Print this help message and exit\n";

void print_usage(const char* progname) {
    fprintf(stderr, usage_msg, progname, progname);
}

void print_help(const char* progname) {
    print_usage(progname);
    fprintf(stderr, "\n%s", help_msg);
}

int convert_unsigned(char *c, unsigned *u) {
    char *endptr;
    errno = 0; // To distinguish success/failure after call

    // Convert string to unsigned long
    unsigned long value = strtoul(c, &endptr, 10);

    // Check for conversion errors
    if (endptr == c) {
        // No digits were found
        fprintf(stderr, "Invalid number: No digits were found in %s.\n", c);
        return 1;
    } else if (*endptr != '\0') {
        // Further characters after the number
        fprintf(stderr, "Invalid number: Further characters were found after the number: %s\n", endptr);
        return 1;
    } else if ((errno == ERANGE && value == ULONG_MAX) || (value > UINT_MAX)) {
        // The value is out of range for unsigned int
        fprintf(stderr, "Invalid number: The number %s is out of range for unsigned int.\n", c);
        return 1;
    } else if (errno != 0 && value == 0) {
        // Other errors
        fprintf(stderr, "Invalid number: %s\n", c);
        return 1;
    }

    // Parsing was successful
    *u = (unsigned int)value;
    return 0;
}

int is_power_of_two(unsigned n) {
    return (ceil(log2(n)) == floor(log2(n)));
}

int convert_hex_to_uint32_t(char *c, uint32_t *u) {
    char *endptr;
    errno = 0; // To distinguish success/failure after call

    // Convert string to unsigned long
    unsigned long value = strtoul(c, &endptr, 16);

    // Check for conversion errors
    if (endptr == c) {
        // No digits were found
        fprintf(stderr, "Invalid number: No digits were found in %s.\n", c);
        return 1;
    } else if (*endptr != '\0' && !isspace(*endptr)) {
        // Further characters after the number
        fprintf(stderr, "Invalid number: Further characters were found after the number: %s\n", endptr);
        return 1;
    } else if ((errno == ERANGE && value == ULONG_MAX) || (value > UINT_MAX)) {
        // The value is out of range for unsigned int
        fprintf(stderr, "Invalid number: The number %s is out of range for unsigned int.\n", c);
        return 1;
    } else if (errno != 0 && value == 0) {
        // Other errors
        fprintf(stderr, "Invalid number: %s\n", c);
        return 1;
    }

    // Parsing was successful
    *u = (uint32_t)value;
    return 0;
}

int convert_dec_to_uint32_t(char *c, uint32_t *u) {
    char *endptr;
    errno = 0; // To distinguish success/failure after call

    // Convert string to unsigned long
    unsigned long value = strtoul(c, &endptr, 10);

    // Check for conversion errors
    if (endptr == c) {
        // No digits were found
        fprintf(stderr, "Invalid number: No digits were found in %s.\n", c);
        return 1;
    } else if (*endptr != '\0' && !isspace(*endptr)) {
        // Further characters after the number
        fprintf(stderr, "Invalid number: Further characters were found after the number: %c\n", *endptr);
        return 1;
    } else if ((errno == ERANGE && value == ULONG_MAX) || (value > UINT_MAX)) {
        // The value is out of range for unsigned int
        fprintf(stderr, "Invalid number: The number %s is out of range for unsigned int.\n", c);
        return 1;
    } else if (errno != 0 && value == 0) {
        // Other errors
        fprintf(stderr, "Invalid number: %s\n", c);
        return 1;
    }

    // Parsing was successful
    *u = (uint32_t)value;
    return 0;
}

int is_csv_file(const char *filename) {
    //get the length of the file and it can be maximum NAME_MAX
    size_t len = strlen(filename);
    // Check if the last four characters are ".csv"
    return (len > 4 && strcmp(filename + len - 4, ".csv") == 0);
}

size_t count_lines(const char *filename) {
    // count the number of lines in the file called filename implementing statically for now

    FILE *fp = fopen(filename,"r");
    size_t ch;
    size_t lines = 0;

    if (!fp) {
        fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    lines++;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\n')
            lines++;
    }

    fclose(fp);
    return lines;
}

int is_empty_line(char* l) {
    for (int i = 0; l[i] != '\0'; i++) {
        if (!isspace(l[i])) {
            return 0;
        }
    }
    return 1;
}


int main(int argc, char *argv[]) {
    //name of the program
    const char* progname = argv[0];

    //just typing the program should show the usage
    if (argc == 1) {
        print_usage(progname);
        return EXIT_FAILURE;
    }

    int opt; // return value for getopt_long
    int option_index = 0;
    // simulation parameters
    int cycles = 1000000000;
    int direct_mapped = 1;
    unsigned cacheline_size = 64;
    unsigned cachelines = 512; // 32 KB L1 cache
    unsigned cache_latency = 1;
    unsigned memory_latency = 200;
    //To control whether the --directmapped and --fourway
    int cache_type_defined = 0;

    const char *tracefile = NULL;

    //required for getopt_long()
    static struct option long_options[] = {
        {"cycles", required_argument, NULL, 'c'},
        {"directmapped", no_argument, NULL, 'd'},
        {"fourway", no_argument, NULL, 'f'},
        {"cacheline-size", required_argument, NULL, 's'},
        {"cachelines", required_argument, NULL, 'n'},
        {"cache-latency", required_argument, NULL, 'l'},
        {"memory-latency", required_argument, NULL, 'L'},
        {"tf", required_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h'},
        {"L2", no_argument, NULL, '2'},
        {"L3", no_argument, NULL, '3'},
        {NULL, 0, NULL, 0}
        //final element has to be all zeros
    };

    //when getopt parses through all arguments it terminates with a return value of -1
    while ((opt = getopt_long(argc, argv, "c:h", long_options, &option_index)) != -1) {
        switch (opt) {
            // cycles
            case 'c':
                if (convert_unsigned(optarg, (unsigned *) &cycles) != 0) {
                    exit(EXIT_FAILURE);
                }
                break;
                // cache line size
            case 's':
                if (convert_unsigned(optarg, &cacheline_size) != 0) {
                    exit(EXIT_FAILURE);
                }
                if (cacheline_size == 0) {
                    fprintf(stderr, "Cache line size can't be 0\n");
                    exit(EXIT_FAILURE);
                } else if (!is_power_of_two(cacheline_size)) {
                    fprintf(stderr, "Cache line size must be power of 2\n");
                    exit(EXIT_FAILURE);
                }
                break;
                // cache lines
            case 'n':
                if (convert_unsigned(optarg, &cachelines) != 0) {
                    exit(EXIT_FAILURE);
                }
                if (cachelines == 0) {
                    fprintf(stderr, "Cache lines can't be 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
                // cache latency
            case 'l':
                if (convert_unsigned(optarg, &cache_latency) != 0) {
                    exit(EXIT_FAILURE);
                }
                break;
                // memory latency
            case 'L':
                if (convert_unsigned(optarg, &memory_latency) != 0) {
                    exit(EXIT_FAILURE);
                }
                break;
                //tracefile
            case 't':
                tracefile = optarg;
                break;
                //directmapped
            case 'd':
                if (cache_type_defined < 0) {
                    //There was --fourway option
                    fprintf(stderr, "Error: A cache can't be four way associative and direct mapped simultaneously\n");
                    print_help(progname);
                    exit(EXIT_FAILURE);

                } else {
                    direct_mapped = 1;
                    cache_type_defined = 1;
                }
                break;
                //fourway
            case 'f':
                if (cache_type_defined > 0) {
                    //There was --directmapped option
                    fprintf(stderr, "Error: A cache can't be four way associative and direct mapped simultaneously\n");
                    print_usage(progname);
                    exit(EXIT_FAILURE);
                } else {
                    direct_mapped = 0;
                    cache_type_defined = -1;
                }
                break;
                //Help
            case 'h':
                print_help(progname);
                exit(EXIT_SUCCESS);
                //L2 cache: 1 MB
            case '2':
                cachelines = 1 << 14;
                cache_latency = 5;
                break;
                // L3 cache: 2 MB
            case '3':
                cachelines = 1 << 15;
                cache_latency = 20;
                break;
        default:
            print_usage(progname);
            exit(EXIT_FAILURE);
        }
    }

    // No inputfile
    if (optind == argc) {
        fprintf(stderr, "Error: Missing positional argument -- <inputFile> is required\n");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if(direct_mapped) {
        if(!is_power_of_two(cachelines)) {
            fprintf(stderr, "Attention: Cache lines of directmapped cache must be power of 2.\n"
                            "           The simulation will be proceeded with %u cache lines\n", (cachelines = (unsigned)pow(2,ceil(log2(cachelines)))));
        }
    } else {
        if (!is_power_of_two(cachelines)){
            fprintf(stderr, "Attention: Cache lines of four-way cache must be at least 4 and power of 2.\n"
                            "           The simulation will be proceeded with %u cache lines\n", (cachelines = (unsigned)pow(2,ceil(log2(cachelines)))));
        } else if (cachelines < 4) {
            fprintf(stderr, "Attention: Cache lines of four-way cache must be at least 4 and power of 2.\n"
                            "           The simulation will be proceeded with %u cache lines\n", (cachelines = 4));
        }
    }

    const char *inputfile = argv[optind];

    // check .csv extension
    if(!is_csv_file(inputfile)) {
        fprintf(stderr, "Not a valid csv fp -- %s\n", inputfile);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Debug output of parsed options
    printf("INPUT:\n");
    printf("Cycles: %d\n", cycles);
    printf("Direct Mapped: %d\n", direct_mapped);
    printf("Cache Line Size: %d\n", cacheline_size);
    printf("Cache Lines: %d\n", cachelines);
    printf("Cache Latency: %d\n", cache_latency);
    printf("Memory Latency: %d\n", memory_latency);
    printf("Trace File: %s\n", tracefile ? tracefile : "None");
    printf("Input File: %s\n\n", inputfile);

    //All lines are counted including blank and invalid lines
    size_t linesCount = count_lines(inputfile);
    size_t emptyLinesCount = 0;

    //Open fp for reading
    FILE *fp = fopen(inputfile, "r");
    if (!fp) {
        fprintf(stderr, "Error opening fp %s: %s\n", inputfile, strerror(errno));
        return 1;
    }
    // Request array the number of requests is <= linesCount
    struct Request* requests = (struct Request*)malloc(sizeof(struct Request) * linesCount);
    if(requests == NULL) {
        fprintf(stderr, "No space in memory: %s\n", strerror(errno));
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    // Buffer for reading line
    char line[256];

    // String of parsed value
    char *column;

    //Request requestCount
    uint32_t requestCount = 0;

    // Reading lines one by one
    while (fgets(line, sizeof(line), fp)) {

        // check if the line empty
        if(is_empty_line(line)) {
            emptyLinesCount++;
            continue;
        }

        // Temp request members
        int we;
        uint32_t address;
        uint32_t data = 0;

        // Boolean for deciding whether the column empty
        int parsed;

        // First column was empty
        if(line[0] == ',') {
            fprintf(stderr, "No operation is given at line %zu\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }


        // Reading first column
        if ((column = strtok(line, ",")) == NULL) {
            fprintf(stderr, "Invalid line at %zu found.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }


        /*
         * Columns are parsed to strings and the legal values
         * are either 'w' or 'r' and doesn't matter capital or
         * not. Before and after letter only white space can be present
         */

        // Boolean to make sure that just 1 operation is parsed
        int found = 0;

        // After parsing, it should be 1
        parsed = 0;

        // Iterating first column
        for (int i = 0; column[i] != '\0'; i++) {
            //If white space then skip
            if (isspace(column[i])) {
                continue;
            } else if ((column[i] == 'W' || column[i] == 'w') && !found) {
                we = 1;
                //Assign found and parsed variables to true and be sure that after the letter just space comes
                found = 1;
                parsed = 1;
            } else if ((column[i] == 'R' || column[i] == 'r') && !found) {
                we = 0;
                //Assign found and parsed variables to true and be sure that after the letter just space comes
                found = 1;
                parsed = 1;
            } else {
                //Either no letter found or more than one
                fprintf(stderr, "Invalid operation at line %zu found: ASCII: %.2x\n", requestCount + emptyLinesCount + 1, column[i]);
                fclose(fp);
                free(requests);
                exit(EXIT_FAILURE);
            }
        }

        // The column was empty
        if (!parsed) {
            fprintf(stderr, "No operation is found in line %zu.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }

        // Reading second column
        if ((column = strtok(NULL,",")) == NULL) {
            fprintf(stderr, "Invalid line at %zu found.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }

        // strtok() skip if two separator next to each other like ",,". So test if there was a ',' before token
        if (column[-1] == ',') {
            fprintf(stderr, "No address is found in line %zu.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }


        // After parsing, it should be 1
        parsed = 0;
        // Iterating second column
        for (int i = 0; column[i] != '\0'; i++) {
            if (isspace(column[i])) {
                continue;
            } else if (column[i] == '0' && (column[i + 1] == 'x' || column[i + 1] == 'X')) {
                // If after '0' a 'X' comes try to convert hex
                if (convert_hex_to_uint32_t(column, &address) != 0) {
                    fclose(fp);
                    free(requests);
                    exit(EXIT_FAILURE);
                }
                // Update parsed variable
                parsed = 1;
                break;
            } else {
                // else try to convert to decimal
                if (convert_dec_to_uint32_t(column, &address) != 0) {
                    fclose(fp);
                    free(requests);
                    exit(EXIT_FAILURE);
                }
                // Update parsed variable
                parsed = 1;
                break;
            }
        }

        // The column was empty
        if (!parsed) {
            fprintf(stderr, "No address is found in line %zu.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }

        /*
         * Reading third column
         * For read operation it can be NULL or after ',' just empty chars
         * but not for write
         */
        if((column = strtok(NULL,",")) == NULL && we == 1) {
            fprintf(stderr, "At line %zu invalid write operation.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }

        /* strtok() skip if two separator next to each other like ",,". So test if there was a ',' before token.
         * If there was a ',' before it means more than 2 commas are used, and it is illegal because it can only
         * have 3 columns */
        if (column != NULL && column[-1] == ',') {
            fprintf(stderr, "Too many arguments for operation at line %zu.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }

        parsed = 0;

        /*
         * If it's not NULL it can have a value or empty. For write it must have
         * a value but for read it must be empty if it's not NULL.
        */
        if (column != NULL) {
            for (int i = 0; column[i] != '\0'; i++) {
                if (isspace(column[i])) {
                    continue;
                } else if (we == 0) {
                    /* This branch shows that the found character is
                     * different from empty chars
                     * and its illegal in a read operation
                     * */
                    fprintf(stderr, "A data (ASCII: %.2x) has been found for read operation at line %zu. Read operation can't have a data\n",
                            column[i], requestCount + emptyLinesCount + 1);
                    fclose(fp);
                    free(requests);
                    exit(EXIT_FAILURE);
                } else if (column[i] == '0' && (column[i + 1] == 'x' || column[i + 1] == 'X')) {
                    if(convert_hex_to_uint32_t(column, &data) != 0) {
                        fclose(fp);
                        free(requests);
                        exit(EXIT_FAILURE);
                    }
                    parsed = 1;
                    break;
                } else {
                    if(convert_dec_to_uint32_t(column, &data) != 0) {
                        fclose(fp);
                        free(requests);
                        exit(EXIT_FAILURE);
                    }
                    parsed = 1;
                    break;
                }
            }
        }

        // The column was empty and not OK for write operation
        if (!parsed && we == 1) {
            fprintf(stderr, "At line %zu the write operation doesn't have a value.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }


        /* If next strtok() doesn't return NULL it means more than
         * 2 commas are used and that's illegal */
        if(column != NULL && strtok(NULL,",") != NULL) {
            fprintf(stderr, "At line %zu too many arguments for operation.\n", requestCount + emptyLinesCount + 1);
            fclose(fp);
            free(requests);
            exit(EXIT_FAILURE);
        }


        // Updating request members
        requests[requestCount].addr = address;
        requests[requestCount].we = we;
        requests[requestCount].data = data;

        // Next request, next line
        requestCount++;
    }
    fclose(fp);


    if(requestCount == 0) {
        fprintf(stderr, "No operation is given. Nothing to run.\n");
        free(requests);
        exit(EXIT_FAILURE);
    }

    struct Result result = run_simulation(cycles, direct_mapped, cachelines, cacheline_size,
                                          cache_latency, memory_latency, requestCount, requests, tracefile);
    printf("OUTPUT:\n"
           "Cycles: %zu\n"
           "Hits: %zu\n"
           "Misses: %zu\n"
           "PrimitiveGate: %zu\n",
           result.cycles, result.hits, result.misses, result.primitiveGateCount);

        free(requests);
        return 0;
    }

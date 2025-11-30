#include "io.h"
#include "simulation_state.h"
#include "grid.h"
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// ============================================================================
// IO.CPP - Level I/O and logging
// ============================================================================

// Helper function to trim trailing whitespace
void trim(char* str) {
    int end = strlen(str) - 1;
    while (end >= 0 && (str[end] == ' ' || str[end] == '\t' || str[end] == '\n' || str[end] == '\r')) end--;
    str[end + 1] = '\0';
}

// ----------------------------------------------------------------------------
// LOAD LEVEL FILE
// ----------------------------------------------------------------------------
// Load a .lvl file into global state.
// ----------------------------------------------------------------------------
bool loadLevelFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open level file " << filename << std::endl;
        return false;
    }

    initializeSimulationState();

    char line[256];
    char section[256] = "";

    while (file.getline(line, sizeof(line))) {
        // Use a trimmed temporary copy for header detection
        char tmp[256];
        strcpy(tmp, line);
        trim(tmp);
        
        // Skip empty lines (except in MAP section)
        if (strlen(tmp) == 0 && strcmp(section, "MAP") != 0) continue;

        // Check for section headers
        if (strcmp(tmp, "NAME:") == 0) {
            strcpy(section, "NAME");
            continue;
        } else if (strcmp(tmp, "ROWS:") == 0) {
            strcpy(section, "ROWS");
            continue;
        } else if (strcmp(tmp, "COLS:") == 0) {
            strcpy(section, "COLS");
            continue;
        } else if (strcmp(tmp, "SEED:") == 0) {
            strcpy(section, "SEED");
            continue;
        } else if (strcmp(tmp, "WEATHER:") == 0) {
            strcpy(section, "WEATHER");
            continue;
        } else if (strcmp(tmp, "MAP:") == 0) {
            strcpy(section, "MAP");
            continue;
        } else if (strcmp(tmp, "SWITCHES:") == 0) {
            strcpy(section, "SWITCHES");
            continue;
        } else if (strcmp(tmp, "TRAINS:") == 0) {
            strcpy(section, "TRAINS");
            continue;
        }

        // Process section data
        if (strcmp(section, "NAME") == 0) {
            // Skip name content or store if needed
            printf("Level Name: %s\n", tmp);
        } else if (strcmp(section, "ROWS") == 0) {
            ROWS = atoi(tmp);
            printf("ROWS set to %d\n", ROWS);
        } else if (strcmp(section, "COLS") == 0) {
            COLS = atoi(tmp);
            printf("COLS set to %d\n", COLS);
        } else if (strcmp(section, "SEED") == 0) {
            seed = atoi(tmp);
            printf("SEED set to %d\n", seed);
        } else if (strcmp(section, "WEATHER") == 0) {
            if (strcmp(tmp, "NORMAL") == 0) weather = WEATHER_NORMAL;
            else if (strcmp(tmp, "RAIN") == 0) weather = WEATHER_RAIN;
            else if (strcmp(tmp, "FOG") == 0) weather = WEATHER_FOG;
            printf("WEATHER set to %s\n", tmp);
        } else if (strcmp(section, "MAP") == 0) {
            // Allocate grid first if not already allocated
            if (grid == nullptr && ROWS > 0 && COLS > 0) {
                printf("Allocating grid: %d rows x %d cols\n", ROWS, COLS);
                grid = new char*[ROWS];
                for (int i = 0; i < ROWS; ++i) {
                    grid[i] = new char[COLS + 1];
                    for (int j = 0; j < COLS; ++j) {
                        grid[i][j] = ' '; // Initialize with spaces
                    }
                    grid[i][COLS] = '\0';
                }
            }
            
            // Read map lines - DO NOT TRIM original line, preserve all spaces
            static int map_row = 0;
            if (map_row < ROWS) {
                // Check if this line is a section header (using trimmed version)
                if (strcmp(tmp, "SWITCHES:") == 0 || strcmp(tmp, "TRAINS:") == 0) {
                    strcpy(section, tmp);
                    map_row = 0; // Reset for next map
                } else {
                    // Copy line to grid, preserving ALL characters including leading/trailing spaces
                    int len = strlen(line);
                    for (int c = 0; c < COLS; ++c) {
                        if (c < len) {
                            grid[map_row][c] = line[c];
                        } else {
                            grid[map_row][c] = ' '; // Pad with spaces if line is shorter
                        }
                    }
                    grid[map_row][COLS] = '\0';
                    
                    printf("Grid row %d (len=%d): '%s'\n", map_row, (int)strlen(grid[map_row]), grid[map_row]);
                    map_row++;
                    
                    if (map_row >= ROWS) {
                        printf("Loaded all %d rows of map\n", map_row);
                        map_row = 0; // Reset
                    }
                }
            }
        } else if (strcmp(section, "SWITCHES") == 0) {
            // Parse switch line: Letter Mode Init K_UP K_RIGHT K_DOWN K_LEFT State0 State1
            char letter, mode_str[20], state0[20], state1[20];
            int init, k[4];
            int parsed = sscanf(tmp, "%c %s %d %d %d %d %d %s %s", 
                               &letter, mode_str, &init, &k[0], &k[1], &k[2], &k[3], state0, state1);
            
            if (parsed == 9) {
                int idx = getSwitchIndex(letter);
                if (idx >= 0 && idx < MAX_SWITCHES) {
                    switches[idx].letter = letter;
                    switches[idx].mode = (strcmp(mode_str, "PER_DIR") == 0) ? MODE_PER_DIR : MODE_GLOBAL;
                    switches[idx].init_state = init;
                    switches[idx].current_state = init;
                    for (int i = 0; i < 4; ++i) {
                        switches[idx].k_values[i] = k[i];
                        switches[idx].counters[i] = 0;
                    }
                    switches[idx].global_counter = 0;
                    switches[idx].flip_queued = false;
                    switches[idx].signal = SIGNAL_GREEN;
                    strcpy(switches[idx].state0, state0);
                    strcpy(switches[idx].state1, state1);
                    
                    if (idx >= num_switches) {
                        num_switches = idx + 1;
                    }
                    
                    printf("Loaded switch %c: mode=%s init=%d k=[%d,%d,%d,%d] states=%s/%s\n",
                           letter, mode_str, init, k[0], k[1], k[2], k[3], state0, state1);
                }
            }
        } else if (strcmp(section, "TRAINS") == 0) {
            // Parse train line: tick x y direction color
            int tick, x, y, dir, col;
            int parsed = sscanf(tmp, "%d %d %d %d %d", &tick, &x, &y, &dir, &col);
            
            if (parsed == 5) {
                // Validate position
                if (!isInBounds(x, y)) {
                    printf("Warning: Train spawn position (%d,%d) is out of bounds\n", x, y);
                    continue;
                }
                
                // Allocate trains array if needed
                if (trains == nullptr) {
                    trains = new Train[MAX_TRAINS];
                    // Initialize all trains
                    for (int i = 0; i < MAX_TRAINS; ++i) {
                        trains[i].state = TRAIN_WAITING;
                        trains[i].spawn_tick = -1;
                        trains[i].id = -1;
                    }
                }
                
                if (num_trains < MAX_TRAINS) {
                    trains[num_trains].id = num_trains;
                    trains[num_trains].spawn_tick = tick;
                    trains[num_trains].x = x;
                    trains[num_trains].y = y;
                    trains[num_trains].next_x = x;
                    trains[num_trains].next_y = y;
                    trains[num_trains].direction = dir;
                    trains[num_trains].next_direction = dir;
                    trains[num_trains].color = col;
                    trains[num_trains].state = TRAIN_WAITING;
                    trains[num_trains].wait_ticks = 0;
                    trains[num_trains].total_wait = 0;
                    
                    // Find nearest destination 'D'
                    bool found_dest = false;
                    int min_dist = 999999;
                    for (int dy = 0; dy < ROWS; ++dy) {
                        for (int dx = 0; dx < COLS; ++dx) {
                            if (grid[dy][dx] == 'D') {
                                int dist = abs(dx - x) + abs(dy - y);
                                if (!found_dest || dist < min_dist) {
                                    trains[num_trains].dest_x = dx;
                                    trains[num_trains].dest_y = dy;
                                    min_dist = dist;
                                    found_dest = true;
                                }
                            }
                        }
                    }
                    
                    if (!found_dest) {
                        printf("Warning: No destination found for train %d\n", num_trains);
                        trains[num_trains].dest_x = x;
                        trains[num_trains].dest_y = y;
                    }
                    
                    printf("Loaded train %d: spawn_tick=%d pos=(%d,%d) dir=%d dest=(%d,%d) tile='%c'\n",
                           num_trains, tick, x, y, dir, 
                           trains[num_trains].dest_x, trains[num_trains].dest_y,
                           grid[y][x]);
                    
                    num_trains++;
                    max_train_id = num_trains - 1;
                } else {
                    printf("Warning: Maximum number of trains (%d) reached\n", MAX_TRAINS);
                }
            }
        }
    }

    file.close();
    
    printf("\nLevel loaded successfully:\n");
    printf("  Grid: %d x %d\n", ROWS, COLS);
    printf("  Trains: %d\n", num_trains);
    printf("  Switches: %d\n", num_switches);
    printf("  Seed: %d\n", seed);
    
    return true;
}

// ----------------------------------------------------------------------------
// INITIALIZE LOG FILES
// ----------------------------------------------------------------------------
// Create/clear CSV logs with headers.
// ----------------------------------------------------------------------------
void initializeLogFiles() {
    std::ofstream trace("trace.csv");
    trace << "Tick,TrainID,X,Y,Direction,State\n";
    trace.close();

    std::ofstream switches_log("switches.csv");
    switches_log << "Tick,Switch,Mode,State\n";
    switches_log.close();

    std::ofstream signals_log("signals.csv");
    signals_log << "Tick,Switch,Signal\n";
    signals_log.close();
}

// ----------------------------------------------------------------------------
// LOG TRAIN TRACE
// ----------------------------------------------------------------------------
// Append tick, train id, position, direction, state to trace.csv.
// ----------------------------------------------------------------------------
void logTrainTrace() {
    std::ofstream trace("trace.csv", std::ios::app);
    for (int i = 0; i < num_trains; ++i) {
        const char* state_str = "MOVING";
        if (trains[i].state == TRAIN_WAITING) state_str = "WAITING";
        else if (trains[i].state == TRAIN_CRASHED) state_str = "CRASHED";
        else if (trains[i].state == TRAIN_DELIVERED) state_str = "DELIVERED";
        trace << current_tick << "," << trains[i].id << "," << trains[i].x << "," << trains[i].y << "," << trains[i].direction << "," << state_str << "\n";
    }
    trace.close();
}

// ----------------------------------------------------------------------------
// LOG SWITCH STATE
// ----------------------------------------------------------------------------
// Append tick, switch id/mode/state to switches.csv.
// ----------------------------------------------------------------------------
void logSwitchState() {
    std::ofstream switches_log("switches.csv", std::ios::app);
    for (int i = 0; i < num_switches; ++i) {
        const char* mode_str = switches[i].mode == MODE_PER_DIR ? "PER_DIR" : "GLOBAL";
        const char* state_str = switches[i].current_state == 0 ? switches[i].state0 : switches[i].state1;
        switches_log << current_tick << "," << switches[i].letter << "," << mode_str << "," << state_str << "\n";
    }
    switches_log.close();
}

// ----------------------------------------------------------------------------
// LOG SIGNAL STATE
// ----------------------------------------------------------------------------
// Append tick, switch id, signal color to signals.csv.
// ----------------------------------------------------------------------------
void logSignalState() {
    std::ofstream signals_log("signals.csv", std::ios::app);
    for (int i = 0; i < num_switches; ++i) {
        const char* signal_str = "GREEN";
        if (switches[i].signal == SIGNAL_YELLOW) signal_str = "YELLOW";
        else if (switches[i].signal == SIGNAL_RED) signal_str = "RED";
        signals_log << current_tick << "," << switches[i].letter << "," << signal_str << "\n";
    }
    signals_log.close();
}

// ----------------------------------------------------------------------------
// WRITE FINAL METRICS
// ----------------------------------------------------------------------------
// Write summary metrics to metrics.txt.
// ----------------------------------------------------------------------------
void writeMetrics() {
    std::ofstream metrics("metrics.txt");
    metrics << "Trains Delivered: " << trains_delivered << "\n";
    metrics << "Trains Crashed: " << trains_crashed << "\n";
    metrics << "Total Wait Ticks: " << total_wait_ticks << "\n";
    metrics << "Average Wait: " << (num_trains > 0 ? (double)total_wait_ticks / num_trains : 0) << "\n";
    metrics << "Switch Flips: " << switch_flips << "\n";
    metrics << "Signal Violations: " << signal_violations << "\n";
    metrics << "Energy Efficiency: " << (num_trains > 0 ? (double)energy_used / num_trains : 0) << "\n";
    metrics.close();
}
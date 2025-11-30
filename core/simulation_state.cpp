#include "simulation_state.h"
#include <cstring>
#include <cstdlib>

// ============================================================================
// SIMULATION_STATE.CPP - Global state definitions
// ============================================================================

// ----------------------------------------------------------------------------
// GRID
// ----------------------------------------------------------------------------
int ROWS = 0;
int COLS = 0;
char** grid = nullptr;

// ----------------------------------------------------------------------------
// TRAINS
// ----------------------------------------------------------------------------
int num_trains = 0;
Train* trains = nullptr;
int max_train_id = 0;

// ----------------------------------------------------------------------------
// SWITCHES
// ----------------------------------------------------------------------------
Switch switches[MAX_SWITCHES];
int num_switches = 0;

// ----------------------------------------------------------------------------
// SPAWN AND DESTINATION POINTS
// ----------------------------------------------------------------------------
SpawnPoint spawn_points[MAX_SPAWNS];
int num_spawns = 0;
DestinationPoint destination_points[MAX_DESTINATIONS];
int num_destinations = 0;

// ----------------------------------------------------------------------------
// SIMULATION PARAMETERS
// ----------------------------------------------------------------------------
int current_tick = 0;
int seed = 0;
int weather = WEATHER_NORMAL;
bool paused = false; // Start unpaused for terminal
bool step_mode = false;

// ----------------------------------------------------------------------------
// METRICS
// ----------------------------------------------------------------------------
int trains_delivered = 0;
int trains_crashed = 0;
int total_wait_ticks = 0;
int switch_flips = 0;
int signal_violations = 0;
int energy_used = 0;

// ----------------------------------------------------------------------------
// EMERGENCY HALT
// ----------------------------------------------------------------------------
bool emergency_halt_active = false;
int emergency_halt_x = -1, emergency_halt_y = -1;
int emergency_halt_timer = 0;

// ============================================================================
// INITIALIZE SIMULATION STATE
// ============================================================================
// ----------------------------------------------------------------------------
// Resets all global simulation state.
// ----------------------------------------------------------------------------
// Called before loading a new level.
// ----------------------------------------------------------------------------
void initializeSimulationState() {
    // Free existing grid
    if (grid != nullptr) {
        for (int i = 0; i < ROWS; ++i) {
            delete[] grid[i];
        }
        delete[] grid;
        grid = nullptr;
    }

    // Free existing trains
    if (trains != nullptr) {
        delete[] trains;
        trains = nullptr;
    }

    // Reset variables
    ROWS = 0;
    COLS = 0;
    num_trains = 0;
    max_train_id = 0;
    num_switches = 0;
    num_spawns = 0;
    num_destinations = 0;
    current_tick = -1;
    seed = 0;
    weather = WEATHER_NORMAL;
    paused = false;
    step_mode = false;
    trains_delivered = 0;
    trains_crashed = 0;
    total_wait_ticks = 0;
    switch_flips = 0;
    signal_violations = 0;
    energy_used = 0;
    emergency_halt_active = false;
    emergency_halt_x = -1;
    emergency_halt_y = -1;
    emergency_halt_timer = 0;

    // Reset switches
    for (int i = 0; i < MAX_SWITCHES; ++i) {
        switches[i].letter = 'A' + i;
        switches[i].mode = MODE_PER_DIR;
        switches[i].init_state = 0;
        for (int j = 0; j < 4; ++j) {
            switches[i].k_values[j] = 1;
            switches[i].counters[j] = 0;
        }
        switches[i].global_counter = 0;
        switches[i].flip_queued = false;
        switches[i].signal = SIGNAL_GREEN;
        strcpy(switches[i].state0, "STATE0");
        strcpy(switches[i].state1, "STATE1");
    }
}
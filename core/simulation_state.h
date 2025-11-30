#ifndef SIMULATION_STATE_H
#define SIMULATION_STATE_H

// ============================================================================
// SIMULATION_STATE.H - Global constants and state
// ============================================================================
// Global constants and arrays used by the game.
// ============================================================================

#include <cstdint>

// ----------------------------------------------------------------------------
// GRID CONSTANTS
// ----------------------------------------------------------------------------
const int MAX_ROWS = 100;
const int MAX_COLS = 100;
const int MAX_SWITCHES = 26; // A-Z
const int MAX_TRAINS = 100;
const int MAX_SPAWNS = 50;
const int MAX_DESTINATIONS = 50;

// Direction constants
#define DIR_UP 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_LEFT 3

// Train states
#define TRAIN_WAITING 0
#define TRAIN_MOVING 1
#define TRAIN_CRASHED 2
#define TRAIN_DELIVERED 3

// Signal states
const int SIGNAL_GREEN = 0;
const int SIGNAL_YELLOW = 1;
const int SIGNAL_RED = 2;

// Weather
const int WEATHER_NORMAL = 0;
const int WEATHER_RAIN = 1;
const int WEATHER_FOG = 2;

// Switch modes
const int MODE_PER_DIR = 0;
const int MODE_GLOBAL = 1;

// ----------------------------------------------------------------------------
// STRUCTURES
// ----------------------------------------------------------------------------
struct Train {
    int id;
    int spawn_tick;
    int x, y;
    int next_x, next_y;        // ✅ ADD THESE if missing
    int direction;
    int next_direction;        // ✅ ADD THIS if missing
    int color;
    int state;
    int wait_ticks;
    int total_wait;
    int dest_x, dest_y;
};

struct Switch {
    char letter;
    int mode;
    int init_state;
    int k_values[4]; // UP, RIGHT, DOWN, LEFT
    char state0[20];
    char state1[20];
    int current_state;
    int counters[4]; // for PER_DIR
    int global_counter; // for GLOBAL
    bool flip_queued;
    int signal;
};

struct SpawnPoint {
    int x, y;
    int direction;
};

struct DestinationPoint {
    int x, y;
};

// ----------------------------------------------------------------------------
// GLOBAL STATE: GRID
// ----------------------------------------------------------------------------
extern int ROWS;
extern int COLS;
extern char** grid;

// ----------------------------------------------------------------------------
// GLOBAL STATE: TRAINS
// ----------------------------------------------------------------------------
extern int num_trains;
extern Train* trains;
extern int max_train_id;

// ----------------------------------------------------------------------------
// GLOBAL STATE: SWITCHES (A-Z mapped to 0-25)
// ----------------------------------------------------------------------------
extern Switch switches[MAX_SWITCHES];
extern int num_switches;

// ----------------------------------------------------------------------------
// GLOBAL STATE: SPAWN POINTS
// ----------------------------------------------------------------------------
extern SpawnPoint spawn_points[MAX_SPAWNS];
extern int num_spawns;

// ----------------------------------------------------------------------------
// GLOBAL STATE: DESTINATION POINTS
// ----------------------------------------------------------------------------
extern DestinationPoint destination_points[MAX_DESTINATIONS];
extern int num_destinations;

// ----------------------------------------------------------------------------
// GLOBAL STATE: SIMULATION PARAMETERS
// ----------------------------------------------------------------------------
extern int current_tick;
extern int seed;
extern int weather;
extern bool paused;
extern bool step_mode;

// ----------------------------------------------------------------------------
// GLOBAL STATE: METRICS
// ----------------------------------------------------------------------------
extern int trains_delivered;
extern int trains_crashed;
extern int total_wait_ticks;
extern int switch_flips;
extern int signal_violations;
extern int energy_used;

// ----------------------------------------------------------------------------
// GLOBAL STATE: EMERGENCY HALT
// ----------------------------------------------------------------------------
extern bool emergency_halt_active;
extern int emergency_halt_x, emergency_halt_y;
extern int emergency_halt_timer;

// ----------------------------------------------------------------------------
// INITIALIZATION FUNCTION
// ----------------------------------------------------------------------------
// Resets all state before loading a new level.
void initializeSimulationState();

#endif

#include "simulation.h"
#include "simulation_state.h"
#include "trains.h"
#include "switches.h"
#include "io.h"
#include <cstdlib>
#include <ctime>
#include <iostream>

// ============================================================================
// SIMULATION.CPP - Implementation of main simulation logic
// ============================================================================

// Helper to print grid
void printGrid();

// ----------------------------------------------------------------------------
// INITIALIZE SIMULATION
// ----------------------------------------------------------------------------

void initializeSimulation(const char* level_file) {
    // Initialize state
    initializeSimulationState();
    
    // Load level
    if (!loadLevelFile(level_file)) {
        std::cerr << "ERROR: Failed to load level file" << std::endl;
        return;
    }
    
    // Initialize random seed
    srand(seed);
    
    // Initialize logs
    initializeLogFiles();
    
    // Update initial signals
    updateSignalLights();
}

// ----------------------------------------------------------------------------
// SIMULATE ONE TICK
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// SIMULATE ONE TICK
// ----------------------------------------------------------------------------

void simulateOneTick() {
    current_tick++;
    
    printf("\n━━━━━━━━━━ TICK %d ━━━━━━━━━━\n", current_tick);
    
    spawnTrainsForTick();
    determineAllRoutes();
    queueSwitchFlips();
    moveAllTrains();
    checkArrivals();
    applyDeferredFlips();
    updateSignalLights();
    updateEmergencyHalt();
    logTrainTrace();
    printGrid();
    
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

// ----------------------------------------------------------------------------
// CHECK IF SIMULATION IS COMPLETE
// ----------------------------------------------------------------------------

bool isSimulationComplete() {
    for (int i = 0; i < num_trains; ++i) {
        if (trains[i].state == TRAIN_MOVING || trains[i].state == TRAIN_WAITING) {
            return false;
        }
    }
    return true;
}

// Helper to print grid
void printGrid() {
    std::cout << "Tick: " << current_tick << std::endl;
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            bool has_train = false;
            int train_id = -1;
            for (int t = 0; t < num_trains; ++t) {
                if (trains[t].x == x && trains[t].y == y) {
                    has_train = true;
                    train_id = trains[t].id;
                    break;
                }
            }
            if (has_train) {
                std::cout << train_id;
            } else {
                std::cout << grid[y][x];
            }
        }
        std::cout << std::endl;
    }
    // Print train info
    for (int i = 0; i < num_trains; ++i) {
        if (trains[i].state == TRAIN_MOVING || trains[i].state == TRAIN_WAITING) {
            std::cout << "Train " << trains[i].id << " at (" << trains[i].x << "," << trains[i].y << ") moving ";
            const char* dir_str = "UP";
            if (trains[i].direction == DIR_RIGHT) dir_str = "RIGHT";
            else if (trains[i].direction == DIR_DOWN) dir_str = "DOWN";
            else if (trains[i].direction == DIR_LEFT) dir_str = "LEFT";
            std::cout << dir_str << std::endl;
        }
    }
}

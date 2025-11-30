#include "app.h"
#include "../core/simulation_state.h"
#include "../core/simulation.h"
#include "../core/trains.h"
#include "../core/switches.h"
#include "../core/io.h"
#include "../core/grid.h"
#include <iostream>
#include <cstdlib>

// ============================================================================
// MAIN.CPP - Entry point of the application (NO CLASSES)
// ============================================================================

// ----------------------------------------------------------------------------
// MAIN ENTRY POINT
// ----------------------------------------------------------------------------
// This function is the main entry point of the application. It handles command
// line arguments to specify the level file to load, loads the level file using
// loadLevelFile, initializes the simulation system, initializes the SFML
// application window, prints control instructions to the console, runs the
// main application loop, cleans up resources, and prints final simulation
// statistics. Returns 0 on success, 1 on error (e.g., failed to load level
// file or initialize application).
// ----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    const char* levelFile = "data/levels/easy_level.lvl"; // Default
    if (argc > 1) {
        levelFile = argv[1];
    }

    std::cout << "========================================" << std::endl;
    std::cout << "   SWITCHBACK RAILS - Railway Simulation" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Loading level: " << levelFile << std::endl;
    std::cout << std::endl;

    // Initialize simulation
    if (!loadLevelFile(levelFile)) {
        std::cerr << "ERROR: Failed to load level file: " << levelFile << std::endl;
        return 1;
    }
    
    // Initialize random seed
    srand(seed);
    
    // Initialize log files
    initializeLogFiles();

    std::cout << std::endl;
    std::cout << "Level loaded successfully!" << std::endl;
    std::cout << "Grid size: " << ROWS << " x " << COLS << std::endl;
    std::cout << "Number of trains: " << num_trains << std::endl;
    std::cout << "Number of switches: " << num_switches << std::endl;
    std::cout << "Random seed: " << seed << std::endl;
    std::cout << std::endl;

    // Initialize app (SFML window)
    if (!initializeApp()) {
        std::cerr << "Failed to initialize SFML application" << std::endl;
        std::cerr << "Continuing in terminal-only mode..." << std::endl;
        
        // Run simulation in terminal-only mode
        std::cout << "\n========================================" << std::endl;
        std::cout << "   RUNNING IN TERMINAL MODE" << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        int max_ticks = 100;
        for (int tick = 0; tick < max_ticks; ++tick) {
            simulateOneTick();
            
            // Check if simulation is complete
            if (isSimulationComplete()) {
                std::cout << "\n=== SIMULATION COMPLETE AT TICK " << current_tick << " ===" << std::endl;
                break;
            }
            
            // Optional: pause between ticks for better visibility
            // Uncomment the next line if you want to slow down the simulation
            // usleep(100000); // 0.1 second delay (Linux/Mac)
            // Sleep(100); // 100ms delay (Windows - need to include <windows.h>)
        }
        
        // Write final metrics
        writeMetrics();
        
        // Print final stats
        std::cout << "\n========================================" << std::endl;
        std::cout << "   FINAL STATISTICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Total ticks: " << current_tick << std::endl;
        std::cout << "Trains delivered: " << trains_delivered << std::endl;
        std::cout << "Trains crashed: " << trains_crashed << std::endl;
        std::cout << "Total wait ticks: " << total_wait_ticks << std::endl;
        std::cout << "Average wait per train: " 
                  << (num_trains > 0 ? (double)total_wait_ticks / num_trains : 0) 
                  << std::endl;
        std::cout << "Switch flips: " << switch_flips << std::endl;
        std::cout << "Signal violations: " << signal_violations << std::endl;
        std::cout << "Energy efficiency: " 
                  << (num_trains > 0 ? (double)energy_used / num_trains : 0) 
                  << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    }

    // SFML mode - print controls
    std::cout << "========================================" << std::endl;
    std::cout << "   CONTROLS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  SPACE: Pause/Resume" << std::endl;
    std::cout << "  . (period): Step one tick" << std::endl;
    std::cout << "  Left click: Place/remove safety tile" << std::endl;
    std::cout << "  Right click: Toggle switch" << std::endl;
    std::cout << "  Middle drag: Pan" << std::endl;
    std::cout << "  Mouse wheel: Zoom" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Run app (SFML event loop)
    runApp();

    // Cleanup
    cleanupApp();
    
    // Write final metrics
    writeMetrics();

    // Print final stats
    std::cout << "\n========================================" << std::endl;
    std::cout << "   FINAL STATISTICS" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total ticks: " << current_tick << std::endl;
    std::cout << "Trains delivered: " << trains_delivered << std::endl;
    std::cout << "Trains crashed: " << trains_crashed << std::endl;
    std::cout << "Total wait ticks: " << total_wait_ticks << std::endl;
    std::cout << "Average wait per train: " 
              << (num_trains > 0 ? (double)total_wait_ticks / num_trains : 0) 
              << std::endl;
    std::cout << "Switch flips: " << switch_flips << std::endl;
    std::cout << "Signal violations: " << signal_violations << std::endl;
    std::cout << "Energy efficiency: " 
              << (num_trains > 0 ? (double)energy_used / num_trains : 0) 
              << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Cleanup grid memory
    if (grid != nullptr) {
        for (int i = 0; i < ROWS; ++i) {
            delete[] grid[i];
        }
        delete[] grid;
        grid = nullptr;
    }
    
    // Cleanup trains memory
    if (trains != nullptr) {
        delete[] trains;
        trains = nullptr;
    }

    return 0;
}
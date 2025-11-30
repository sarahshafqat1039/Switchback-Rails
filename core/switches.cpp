#include "switches.h"
#include "simulation_state.h"
#include "grid.h"
#include "io.h"
#include <cstdlib>
#include <iostream>

// ============================================================================
// SWITCHES.CPP - Switch management
// ============================================================================

// ----------------------------------------------------------------------------
// UPDATE SWITCH COUNTERS
// ----------------------------------------------------------------------------
// Increment counters for trains entering switches.
// ----------------------------------------------------------------------------
void updateSwitchCounters(int switch_idx, int entry_dir) {
    if (switch_idx < 0 || switch_idx >= MAX_SWITCHES) return;
    Switch& sw = switches[switch_idx];
    if (sw.mode == MODE_PER_DIR) {
        sw.counters[entry_dir]++;
        std::cout << "  Switch " << sw.letter << " counter[" << entry_dir << "] = " << sw.counters[entry_dir] << " (k=" << sw.k_values[entry_dir] << ")" << std::endl;
    } else {
        sw.global_counter++;
        std::cout << "  Switch " << sw.letter << " global_counter = " << sw.global_counter << " (k=" << sw.k_values[0] << ")" << std::endl;
    }
}

// ----------------------------------------------------------------------------
// QUEUE SWITCH FLIPS
// ----------------------------------------------------------------------------
// Queue flips when counters hit K.
// ----------------------------------------------------------------------------
void queueSwitchFlips() {
    for (int i = 0; i < num_switches; ++i) {
        Switch& sw = switches[i];
        bool should_flip = false;
        if (sw.mode == MODE_PER_DIR) {
            for (int d = 0; d < 4; ++d) {
                if (sw.counters[d] >= sw.k_values[d]) {
                    should_flip = true;
                    break;
                }
            }
        } else {
            if (sw.global_counter >= sw.k_values[0]) {
                should_flip = true;
            }
        }
        if (should_flip) {
            sw.flip_queued = true;
        }
    }
}

// ----------------------------------------------------------------------------
// APPLY DEFERRED FLIPS
// ----------------------------------------------------------------------------
// Apply queued flips after movement.
// CRITICAL: This happens AFTER all trains have moved, so trains that triggered
// the flip in this tick are NOT affected by it - they used the OLD state.
// This ensures deterministic behavior.
// ----------------------------------------------------------------------------
void applyDeferredFlips() {
    for (int i = 0; i < num_switches; ++i) {
        Switch& sw = switches[i];
        if (sw.flip_queued) {
            int old_state = sw.current_state;
            sw.current_state = 1 - sw.current_state;
            sw.flip_queued = false;
            
            // Reset counters after flip
            if (sw.mode == MODE_PER_DIR) {
                for (int d = 0; d < 4; ++d) sw.counters[d] = 0;
            } else {
                sw.global_counter = 0;
            }
            
            switch_flips++;
            
            printf("Switch %c flipped from state %d to %d at END of tick %d\n",
                   sw.letter, old_state, sw.current_state, current_tick);
            
            logSwitchState();
        }
    }
}

// ----------------------------------------------------------------------------
// UPDATE SIGNAL LIGHTS
// ----------------------------------------------------------------------------
// Update signal colors for switches.
// ----------------------------------------------------------------------------
void updateSignalLights() {
    for (int i = 0; i < num_switches; ++i) {
        Switch& sw = switches[i];
        // Find switch position
        int sx = -1, sy = -1;
        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                if (grid[y][x] == sw.letter) {
                    sx = x;
                    sy = y;
                    goto found_switch;
                }
            }
        }
        found_switch:;
        if (sx == -1) continue;

        // Check if next tile is free
        // For simplicity, assume next tile based on current state
        // This is simplified; in full implementation, need to check route
        int next_x = sx, next_y = sy;
        if (sw.current_state == 0) {
            // Assume state0 is straight, etc. Simplified.
            next_x += 1; // Assume right
        } else {
            next_y += 1; // Assume down
        }
        bool next_free = isInBounds(next_x, next_y) && isTrackTile(next_x, next_y);
        // Check for trains within 2 tiles
        bool train_near = false;
        for (int t = 0; t < num_trains; ++t) {
            int dx = abs(trains[t].x - sx);
            int dy = abs(trains[t].y - sy);
            if (dx + dy <= 2) {
                train_near = true;
                break;
            }
        }
        if (!next_free) {
            sw.signal = SIGNAL_RED;
        } else if (train_near) {
            sw.signal = SIGNAL_YELLOW;
        } else {
            sw.signal = SIGNAL_GREEN;
        }
    }
    logSignalState();
}

// ----------------------------------------------------------------------------
// TOGGLE SWITCH STATE (Manual)
// ----------------------------------------------------------------------------
// Manually toggle a switch state.
// ----------------------------------------------------------------------------
void toggleSwitchState(int switch_idx) {
    if (switch_idx < 0 || switch_idx >= MAX_SWITCHES) return;
    switches[switch_idx].current_state = 1 - switches[switch_idx].current_state;
    switch_flips++;
    logSwitchState();
}

// ----------------------------------------------------------------------------
// GET SWITCH STATE FOR DIRECTION
// ----------------------------------------------------------------------------
// Return the state for a given direction.
// ----------------------------------------------------------------------------
int getSwitchStateForDirection(int switch_idx, int dir) {
    if (switch_idx < 0 || switch_idx >= MAX_SWITCHES) return 0;
    return switches[switch_idx].current_state;
}

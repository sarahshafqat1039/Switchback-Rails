#include "trains.h"
#include "simulation_state.h"
#include "grid.h"
#include "switches.h"
#include <cstdlib>
#include <iostream>

// ============================================================================
// TRAINS.CPP - Train logic
// ============================================================================

// Storage for planned moves (for collisions).
struct PlannedMove {
    int train_id;
    int from_x, from_y;
    int to_x, to_y;
    int direction;
    int priority; // Manhattan distance to dest
};

PlannedMove planned_moves[MAX_TRAINS];
int num_planned_moves = 0;

// Previous positions (to detect switch entry).
int prev_x[MAX_TRAINS];
int prev_y[MAX_TRAINS];

// ----------------------------------------------------------------------------
// SPAWN TRAINS FOR CURRENT TICK
// ----------------------------------------------------------------------------
void spawnTrainsForTick() {
    for (int i = 0; i < num_trains; ++i) {
        if (trains[i].spawn_tick == current_tick && trains[i].state == TRAIN_WAITING) {
            // Check if spawn position is free
            bool occupied = false;
            for (int j = 0; j < num_trains; ++j) {
                if (i != j && trains[j].state == TRAIN_MOVING && 
                    trains[j].x == trains[i].x && trains[j].y == trains[i].y) {
                    occupied = true;
                    break;
                }
            }
            
            if (!occupied) {
                // ALWAYS spawn - crashes happen during movement phase
                trains[i].state = TRAIN_MOVING;
                printf("Train %d spawned at (%d,%d) direction=%d\n",
                       i, trains[i].x, trains[i].y, trains[i].direction);
            } else {
                // Retry next tick
                trains[i].spawn_tick++;
                printf("Train %d spawn delayed - position occupied\n", i);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// GET SWITCH OUTPUT DIRECTION
// ----------------------------------------------------------------------------
int getSwitchOutputDirection(int switch_idx, int entry_dir, int state) {
    if (switch_idx < 0 || switch_idx >= MAX_SWITCHES) return entry_dir;
    
    if (state == 0) {
        return entry_dir; // Straight through
    } else {
        // Turn counter-clockwise 90 degrees (LEFT turn)
        if (entry_dir == DIR_UP) return DIR_LEFT;      // Changed from RIGHT
        else if (entry_dir == DIR_RIGHT) return DIR_UP;   // Changed from DOWN
        else if (entry_dir == DIR_DOWN) return DIR_RIGHT; // Changed from LEFT
        else if (entry_dir == DIR_LEFT) return DIR_DOWN;  // Changed from UP
    }
    return entry_dir;
}

// ----------------------------------------------------------------------------
// SMART ROUTING AT CROSSING
// ----------------------------------------------------------------------------
int getSmartDirectionAtCrossing(int x, int y, int current_dir) {
    for (int i = 0; i < num_trains; ++i) {
        if (trains[i].x == x && trains[i].y == y && trains[i].state == TRAIN_MOVING) {
            int dest_x = trains[i].dest_x;
            int dest_y = trains[i].dest_y;
            
            int best_dir = current_dir;
            int best_dist = abs(x - dest_x) + abs(y - dest_y);
            
            int dirs[] = {DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT};
            int dx[] = {0, 1, 0, -1};
            int dy[] = {-1, 0, 1, 0};
            
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d];
                int ny = y + dy[d];
                if (isInBounds(nx, ny) && isTrackTile(nx, ny)) {
                    int dist = abs(nx - dest_x) + abs(ny - dest_y);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_dir = dirs[d];
                    }
                }
            }
            return best_dir;
        }
    }
    return current_dir;
}

// ----------------------------------------------------------------------------
// GET NEXT DIRECTION - determines direction AFTER moving to next tile
// ----------------------------------------------------------------------------
int getNextDirection(int next_x, int next_y, int current_dir) {
    if (!isInBounds(next_x, next_y)) return current_dir;
    
    char next_tile = grid[next_y][next_x];
    
    // For horizontal/vertical tracks and safety buffers, maintain direction
    if (next_tile == '-' || next_tile == '=' || next_tile == 'S') {
        if (current_dir == DIR_LEFT || current_dir == DIR_RIGHT) {
            return current_dir;
        }
        return current_dir; // Will crash if incompatible
    }
    
    if (next_tile == '|') {
        if (current_dir == DIR_UP || current_dir == DIR_DOWN) {
            return current_dir;
        }
        return current_dir; // Will crash if incompatible
    }
    
    // Curves change direction
    if (next_tile == '/') {
        if (current_dir == DIR_RIGHT) return DIR_UP;
        else if (current_dir == DIR_UP) return DIR_RIGHT;
        else if (current_dir == DIR_DOWN) return DIR_LEFT;
        else if (current_dir == DIR_LEFT) return DIR_DOWN;
    } else if (next_tile == '\\') {
        if (current_dir == DIR_RIGHT) return DIR_DOWN;
        else if (current_dir == DIR_DOWN) return DIR_RIGHT;
        else if (current_dir == DIR_LEFT) return DIR_UP;
        else if (current_dir == DIR_UP) return DIR_LEFT;
    }
    
    // Crossing - smart routing
    if (next_tile == '+') {
        return getSmartDirectionAtCrossing(next_x, next_y, current_dir);
    }
    
    // Switch - use current state
    if (next_tile >= 'A' && next_tile <= 'Z') {
        int idx = getSwitchIndex(next_tile);
        int state = switches[idx].current_state;
        return getSwitchOutputDirection(idx, current_dir, state);
    }
    
    // Destination
    if (next_tile == 'D') {
        return current_dir;
    }
    
    return current_dir;
}

// ----------------------------------------------------------------------------
// DETERMINE NEXT POSITION
// ----------------------------------------------------------------------------
bool determineNextPosition(int train_idx) {
    Train& t = trains[train_idx];
    if (t.state != TRAIN_MOVING) return false;

    int x = t.x, y = t.y, dir = t.direction;
    
    if (!isInBounds(x, y)) {
        printf("ERROR: Train %d at invalid position (%d,%d)\n", train_idx, x, y);
        t.state = TRAIN_CRASHED;
        trains_crashed++;
        return false;
    }
    
    char tile = grid[y][x];

    printf("  Train %d at (%d,%d) tile='%c' dir=%d\n", train_idx, x, y, tile, dir);

    // Calculate next position
    int dx = 0, dy = 0;
    if (dir == DIR_RIGHT) dx = 1;
    else if (dir == DIR_LEFT) dx = -1;
    else if (dir == DIR_DOWN) dy = 1;
    else if (dir == DIR_UP) dy = -1;

    int nx = x + dx, ny = y + dy;

    printf("    -> Next position: (%d,%d)\n", nx, ny);
    
    if (!isInBounds(nx, ny)) {
        printf("  Train %d: OUT OF BOUNDS - will crash\n", train_idx);
        t.state = TRAIN_CRASHED;
        trains_crashed++;
        return false;
    }

    printf("    -> Next tile: '%c' valid=%d\n", grid[ny][nx], isTrackTile(nx, ny));
    
    if (!isTrackTile(nx, ny)) {
        printf("  Train %d: NOT A TRACK at (%d,%d) tile='%c' - will crash\n", 
               train_idx, nx, ny, grid[ny][nx]);
        t.state = TRAIN_CRASHED;
        trains_crashed++;
        return false;
    }

    // Determine direction AFTER moving to next tile
    int new_dir = getNextDirection(nx, ny, dir);

    t.next_x = nx;
    t.next_y = ny;
    t.next_direction = new_dir;
    
    return true;
}

// ----------------------------------------------------------------------------
// DETERMINE ALL ROUTES
// ----------------------------------------------------------------------------
void determineAllRoutes() {
    num_planned_moves = 0;
    
    for (int i = 0; i < num_trains; ++i) {
        if (trains[i].state != TRAIN_MOVING) continue;
        
        prev_x[i] = trains[i].x;
        prev_y[i] = trains[i].y;
        
        if (determineNextPosition(i)) {
            // Check if ENTERING a switch (next tile is switch)
            if (isSwitchTile(trains[i].next_x, trains[i].next_y)) {
                int idx = getSwitchIndex(grid[trains[i].next_y][trains[i].next_x]);
                // Entry direction = current direction
                updateSwitchCounters(idx, trains[i].direction);
            }
            
            // Add to planned moves
            planned_moves[num_planned_moves].train_id = i;
            planned_moves[num_planned_moves].from_x = trains[i].x;
            planned_moves[num_planned_moves].from_y = trains[i].y;
            planned_moves[num_planned_moves].to_x = trains[i].next_x;
            planned_moves[num_planned_moves].to_y = trains[i].next_y;
            planned_moves[num_planned_moves].direction = trains[i].next_direction;
            
            // Priority = distance to destination
            int dx = abs(trains[i].next_x - trains[i].dest_x);
            int dy = abs(trains[i].next_y - trains[i].dest_y);
            planned_moves[num_planned_moves].priority = dx + dy;
            
            num_planned_moves++;
        }
    }
}

// ----------------------------------------------------------------------------
// DETECT COLLISIONS
// ----------------------------------------------------------------------------
void detectCollisions() {
    // Same-tile collisions
    for (int i = 0; i < num_planned_moves; ++i) {
        if (planned_moves[i].train_id == -1) continue;
        
        for (int j = i + 1; j < num_planned_moves; ++j) {
            if (planned_moves[j].train_id == -1) continue;
            
            PlannedMove& p1 = planned_moves[i];
            PlannedMove& p2 = planned_moves[j];
            
            if (p1.to_x == p2.to_x && p1.to_y == p2.to_y) {
                int t1_id = p1.train_id;
                int t2_id = p2.train_id;
                
                if (p1.priority > p2.priority) {
                    p2.train_id = -1;
                    trains[t2_id].total_wait++;
                    total_wait_ticks++;
                } else if (p2.priority > p1.priority) {
                    p1.train_id = -1;
                    trains[t1_id].total_wait++;
                    total_wait_ticks++;
                } else {
                    p1.train_id = -1;
                    p2.train_id = -1;
                    trains[t1_id].state = TRAIN_CRASHED;
                    trains[t2_id].state = TRAIN_CRASHED;
                    trains_crashed += 2;
                    printf("CRASH: Trains %d and %d (equal priority %d)\n", t1_id, t2_id, p1.priority);
                }
            }
        }
    }

    // Head-on swap collisions
    for (int i = 0; i < num_planned_moves; ++i) {
        if (planned_moves[i].train_id == -1) continue;
        
        for (int j = i + 1; j < num_planned_moves; ++j) {
            if (planned_moves[j].train_id == -1) continue;
            
            PlannedMove& p1 = planned_moves[i];
            PlannedMove& p2 = planned_moves[j];
            
            if (p1.from_x == p2.to_x && p1.from_y == p2.to_y &&
                p2.from_x == p1.to_x && p2.from_y == p1.to_y) {
                
                int t1_id = p1.train_id;
                int t2_id = p2.train_id;
                
                if (p1.priority > p2.priority) {
                    p2.train_id = -1;
                    trains[t2_id].total_wait++;
                    total_wait_ticks++;
                } else if (p2.priority > p1.priority) {
                    p1.train_id = -1;
                    trains[t1_id].total_wait++;
                    total_wait_ticks++;
                } else {
                    p1.train_id = -1;
                    p2.train_id = -1;
                    trains[t1_id].state = TRAIN_CRASHED;
                    trains[t2_id].state = TRAIN_CRASHED;
                    trains_crashed += 2;
                    printf("SWAP CRASH: Trains %d and %d\n", t1_id, t2_id);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// MOVE ALL TRAINS
// ----------------------------------------------------------------------------
void moveAllTrains() {
    detectCollisions();
    
    for (int i = 0; i < num_planned_moves; ++i) {
        PlannedMove& pm = planned_moves[i];
        if (pm.train_id != -1) {
            Train& t = trains[pm.train_id];
            
            t.x = pm.to_x;
            t.y = pm.to_y;
            t.direction = pm.direction; // Update to new direction
            
            // Safety buffer
            if (grid[t.y][t.x] == '=') {
                t.wait_ticks = 1;
                t.state = TRAIN_WAITING;
            }
            
            // Weather: Rain
            if (weather == WEATHER_RAIN && (rand() % 10) < 2) {
                t.wait_ticks = 1;
                t.state = TRAIN_WAITING;
            }
            
            // Emergency halt
            if (emergency_halt_active) {
                int dx = abs(t.x - emergency_halt_x);
                int dy = abs(t.y - emergency_halt_y);
                if (dx <= 1 && dy <= 1) {
                    t.wait_ticks = 3;
                    t.state = TRAIN_WAITING;
                }
            }
            
            energy_used++;
        }
    }
    
    // Update wait ticks
    for (int i = 0; i < num_trains; ++i) {
        if (trains[i].wait_ticks > 0) {
            trains[i].wait_ticks--;
            trains[i].total_wait++;
            total_wait_ticks++;
            if (trains[i].wait_ticks == 0 && trains[i].state == TRAIN_WAITING) {
                trains[i].state = TRAIN_MOVING;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// CHECK ARRIVALS
// ----------------------------------------------------------------------------
void checkArrivals() {
    for (int i = 0; i < num_trains; ++i) {
        if ((trains[i].state == TRAIN_MOVING || trains[i].state == TRAIN_WAITING) &&
            trains[i].x == trains[i].dest_x && trains[i].y == trains[i].dest_y) {
            trains[i].state = TRAIN_DELIVERED;
            trains_delivered++;
            printf("✓ Train %d DELIVERED at (%d,%d)\n", i, trains[i].dest_x, trains[i].dest_y);
        }
    }
}

// ----------------------------------------------------------------------------
// APPLY EMERGENCY HALT
// ----------------------------------------------------------------------------
void applyEmergencyHalt(int switch_idx) {
    if (switch_idx < 0 || switch_idx >= MAX_SWITCHES) return;
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            if (grid[y][x] == switches[switch_idx].letter) {
                emergency_halt_x = x;
                emergency_halt_y = y;
                emergency_halt_active = true;
                emergency_halt_timer = 3;
                return;
            }
        }
    }
}

// ----------------------------------------------------------------------------
// UPDATE EMERGENCY HALT
// ----------------------------------------------------------------------------
void updateEmergencyHalt() {
    if (emergency_halt_active) {
        emergency_halt_timer--;
        if (emergency_halt_timer <= 0) {
            emergency_halt_active = false;
        }
    }
}
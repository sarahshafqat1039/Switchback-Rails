#include "grid.h"
#include "simulation_state.h"

// ============================================================================
// GRID.CPP - Grid utilities
// ============================================================================

// ----------------------------------------------------------------------------
// Check if a position is inside the grid.
// ----------------------------------------------------------------------------
// Returns true if x,y are within bounds.
// ----------------------------------------------------------------------------
bool isInBounds(int x, int y) {
    return x >= 0 && x < COLS && y >= 0 && y < ROWS;
}

// ----------------------------------------------------------------------------
// Check if a tile is a track tile.
// ----------------------------------------------------------------------------
// Returns true if the tile can be traversed by trains.
// ----------------------------------------------------------------------------
bool isTrackTile(int x, int y) {
    if (!isInBounds(x, y)) return false;
    char tile = grid[y][x];
    return tile == '-' || tile == '|' || tile == '/' || tile == '\\' || 
           tile == '+' || tile == '=' ||
           (tile >= 'A' && tile <= 'Z') || 
           tile == 'S' || tile == 'D';
}

// ----------------------------------------------------------------------------
// Check if a tile is a switch.
// ----------------------------------------------------------------------------
// Returns true if the tile is 'A'..'Z'.
// ----------------------------------------------------------------------------
bool isSwitchTile(int x, int y) {
    if (!isInBounds(x, y)) return false;
    char tile = grid[y][x];
    return tile >= 'A' && tile <= 'Z';
}

// ----------------------------------------------------------------------------
// Get switch index from character.
// ----------------------------------------------------------------------------
// Maps 'A'..'Z' to 0..25, else -1.
// ----------------------------------------------------------------------------
int getSwitchIndex(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    return -1;
}

// ----------------------------------------------------------------------------
// Check if a position is a spawn point.
// ----------------------------------------------------------------------------
// Returns true if x,y is a spawn.
// ----------------------------------------------------------------------------
bool isSpawnPoint(int x, int y) {
    if (!isInBounds(x, y)) return false;
    return grid[y][x] == 'S';
}

// ----------------------------------------------------------------------------
// Check if a position is a destination.
// ----------------------------------------------------------------------------
// Returns true if x,y is a destination.
// ----------------------------------------------------------------------------
bool isDestinationPoint(int x, int y) {
    if (!isInBounds(x, y)) return false;
    return grid[y][x] == 'D';
}

// ----------------------------------------------------------------------------
// Toggle a safety tile.
// ----------------------------------------------------------------------------
// Returns true if toggled successfully.
// ----------------------------------------------------------------------------
bool toggleSafetyTile(int x, int y) {
    if (!isInBounds(x, y)) return false;
    if (grid[y][x] == '=') {
        grid[y][x] = '-'; // Assume it was on a track
    } else if (grid[y][x] == '-') {
        grid[y][x] = '=';
    } else {
        return false; // Can only toggle on horizontal tracks
    }
    return true;
}

#ifndef SWITCHES_H
#define SWITCHES_H

// ============================================================================
// SWITCHES.H - Switch logic
// ============================================================================

// ----------------------------------------------------------------------------
// SWITCH COUNTER UPDATE
// ----------------------------------------------------------------------------
// Increment counters when trains enter switches.
void updateSwitchCounters(int switch_idx, int entry_dir);

// ----------------------------------------------------------------------------
// FLIP QUEUE
// ----------------------------------------------------------------------------
// Queue flips when counters reach K.
void queueSwitchFlips();

// ----------------------------------------------------------------------------
// DEFERRED FLIP
// ----------------------------------------------------------------------------
// Apply queued flips after movement.
void applyDeferredFlips();

// ----------------------------------------------------------------------------
// SIGNAL CALCULATION
// ----------------------------------------------------------------------------
// Update switch signal colors.
void updateSignalLights();

// ----------------------------------------------------------------------------
// SWITCH TOGGLE (for manual control / editing)
// ----------------------------------------------------------------------------
// Manually toggle a switch state.
void toggleSwitchState(int switch_idx);

// ----------------------------------------------------------------------------
// HELPER FUNCTIONS
// ----------------------------------------------------------------------------
// Get state for a given direction.
int getSwitchStateForDirection(int switch_idx, int dir);

#endif

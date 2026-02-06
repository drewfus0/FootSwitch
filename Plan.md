# FootSwitch Multi-Switch Implementation Plan

## Phase 1: Firmware Architecture Refactoring
The goal is to move pin management and "action logic" out of `main.cpp` and the global scope into a self-contained Class.

- [x] **Step 1: Create `Switch` Class**
- [x] **Step 2: Refactor `ConfigKeyboard`**
- [x] **Step 3: Update `main.cpp`**
- [x] **Step 4: NVS & Protocol Updates**

## Phase 3: Web App Interface

- [x] **Step 5: Web App Dashboard**
    - Create a "Manager" view to list active switches.
    - Add "Add Switch" UI with Pin Selector (Safe Pins: 4, 13-19, 21-23, 25-27, 32-33).
    - Add "Configure" and "Delete" actions.

## Completed
All steps for Multi-Switch support are implemented.


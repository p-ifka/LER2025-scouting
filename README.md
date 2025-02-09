# LER 2025 scouting app 


## progress
- [x] produce output given information about a match including:
  - each time the team's robot scores, how they scored, when, and whether it was in auto
  - each time the team got a foul
  - whether the team:
    - participated in the match
    - won
    - left the starting area during auto
    - played defense
    - was 'parked' in climb area at end of match
    - climbed a 'deep' cage at the end of match
    - climbed a 'shallow' cage at the end of match
- [] have a program able to parse output
  - [] be able to store output in a database/spreadsheet
- [] have an android compatible ui for the collector

## output
the current program will output a string of binary characters formated as such
<!-- | match# | team# | checkbox_ | checkbox_ | checkbox_end | action_type | time | action_type | time | -->
<!-- |:-------|-------|-----------|-----------|--------------|-------------|------|-------------|------| -->
<!-- | 8      | 16    | 3         | 3         | 3            | 4           | 32   | 4           | 32   | -->

| match# | team# | penalty# | checkbox_ | checkbox_ | checkbox_end | action_type | action_type |
|:-------|-------|----------|-----------|-----------|--------------|-------------|-------------|
| 8      | 16    | 8        | 3         | 3         | 3            | 4           | 4           |
  	   	 


> bottom row is # size in bits



## checkbox data format
the data for checkboxes is recorded as a series of 3-bit sequences, each indicates a different value that is true, the sequence 000 is used to indicate the end of the list of checkbox values
* 000 : END (placed after last condition, marks start of action list)
* 111 : free
* 001 : didn't play match
* 010 : won match
* 100 : played defense
* 011 : had auto
* 110 : did deep climb
* 101 : did shallow climb
* 010 : did park

## action data format
the data for actions is recorded as a 4-bit sequence indicating the type of action it was, and a 32-bit integer indicating the time it was recorded (time elapsed since app opened)
* 0000 : END (no longer used)
* 0001 : L1 teleop
* 0010 : L1 auto
* 0100 : L2 teleop
* 1000 : L2 auto
* 0011 : L3 teleop
* 1100 : L3 auto
* 1001 : L4 teleop
* 0110 : L4 auto
* 1010 : net teleop
* 0101 : net auto
* 0111 : proc teleop
* 1110 : proc auto
* 1011 : algae teleop
* 1101 : algae auto
* 1111 : 

## binds:
- app: q; quit, t; toggle auto mode, g; generate output
- actions: 1234; coral levels, n; net, m; removed algae, p; processor
- conditions: w; win, a; had auto, s; shallow climb, d; deep climb, r; park

press above buttons to add action/conditions to list, press g to generate output,
 
press t to toggle auto mode, if auto mode is on the action will be recorded as done in autonomous
if applicable

 

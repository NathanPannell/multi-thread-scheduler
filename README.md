<i>Completed as part of CSC360: Operating Systems</i>

# DESIGN

## Threads
Consistent with original design. I used a thread per train, with an additional 'conductor' thread.

## Mutexes
Differs from original design. 
- **Planned**:
  - 5 Mutexes:
    - 4 Priority Queues
    - 1 Main Track
- **Actual**:
  - 6 Mutexes:
    - 2 Priority Queues
    - 1 Main Track
    - 1 Thread Status Array
    - 1 Start Loading
    - 1 STDOUT Consistency

## Data Structures
Consistent with original design. I knew I would need a priority queue and I did need to make one.

## Convars
Differs from original design.
- **Planned**:
  - 5 Convars:
    - 4 Priority Queues
    - 1 Main Track
- **Actual**:
  - 3 Convars:
    - 1 Main Track
    - 1 Thread Status Array
    - 1 Start Loading

# CODE STRUCTURE

## Overall Code Structure
Generally consistent with original design. My code uses the following structure.

### In main thread:
1. Open/Parse input file
2. Create worker threads (1 per train)
3. Initialize queues, mutexes, convars
4. While there are outstanding worker threads, loop and select the next train to cross
5. Eventually exit once all trains have crossed

### In worker thread:
1. Wait for signal to start loading
2. Start loading, indicate when finished
3. Wait for signal to cross
4. Start crossing, indicate when finished
5. Exit when finished crossing

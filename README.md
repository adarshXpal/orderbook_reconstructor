# MBP-10 Orderbook Reconstruction from MBO Data

This C++ program reconstructs MBP-10 (Market By Price) orderbook snapshots from MBO (Market By Order) data.

## Implementation Overview

The program processes MBO data sequentially and maintains an in-memory orderbook state to generate MBP snapshots. Here's how it works:

1. **Data Parsing**: Parses CSV MBO data into structured objects for efficient processing
2. **Orderbook State Management**: Maintains separate bid and ask orderbooks using ordered maps
3. **Action Processing**: Handles different MBO actions (Add, Cancel, Trade, Fill, Clear)
4. **MBP Generation**: Converts orderbook state into MBP-10 format with 10 levels of depth

## Key Features

- **Efficient Data Structures**: Uses `std::map` for price levels and `std::set` for orders at each price
- **Memory Optimization**: Maintains order lookup for O(1) order operations
- **Correct Action Handling**: Properly processes all MBO action types
- **MBP-10 Compliance**: Generates output matching the expected MBP format

## Optimization Steps

1. **Data Structure Choice**:
   - Used `std::map` for price levels (ordered by price)
   - Used `std::set` for orders at each price level (ordered by order_id)
   - Maintained separate order lookup map for O(1) order access
2. **Memory Management**:
   - Efficient string handling with minimal copying
   - Proper cleanup of empty price levels
   - Stream-based CSV processing to handle large files
3. **Performance Optimizations**:
   - Compile with `-O3` optimization flag
   - Minimized string allocations
   - Used references where possible to avoid copying
4. **Algorithm Efficiency**:
   - O(log n) operations for orderbook modifications
   - Linear time MBP snapshot generation
   - Single-pass processing of MBO data

## Special Considerations

1. **Initial Clear Action**: The first row (clear action) is handled specially to initialize an empty orderbook
2. **Trade Actions**: F actions are converted to T actions in MBP output; C actions remain C
3. **Side Handling**: Trades with side 'N' don't alter the orderbook state
4. **Price Level Aggregation**: Orders at the same price are aggregated with total size and count

## Limitations and Assumptions

1. **Trade Matching**: This implementation uses a simplified trade matching model. In a real system, trades would need to match against existing orders more precisely.
2. **Order Priority**: The current implementation doesn't handle order priority within price levels (FIFO, etc.)
3. **Partial Fills**: Trade actions are assumed to fully consume orders or leave remaining size
4. **Data Validation**: Limited validation of input data format and consistency

## Potential Improvements

1. **Enhanced Trade Matching**: Implement more sophisticated trade matching algorithms
2. **Order Priority**: Add support for order priority within price levels
3. **Data Validation**: Add comprehensive input validation
4. **Performance Profiling**: Add performance monitoring and optimization
5. **Unit Testing**: Add comprehensive unit tests for correctness verification
6. **Memory Pooling**: Implement custom memory allocators for better performance
7. **Parallel Processing**: Consider parallel processing for very large datasets

## How to Run

To build and run the program and generate the output file:

### 1. Build the executable (choose one of the following)

**Using make:**

```sh
make
```

**Or directly with g++:**

```sh
g++ -std=c++11 -O3 -Wall -Wextra -o reconstruction_adarsh orderbook_reconstructor.cpp
```

### 2. Run the reconstruction (this will generate `mbp_output.csv`)

```sh
./reconstruction_adarsh mbo.csv
```

The output file `mbp_output.csv` will be created in the current directory.

## File Structure

- `orderbook_reconstructor.cpp`: Main implementation
- `Makefile`: Build configuration
- `README.md`: This documentation
- `mbo.csv`: Input MBO data
- `mbp_output.csv`: Generated MBP output

## Thought Process

The implementation follows these design principles:

1. **Simplicity**: Start with a clear, readable implementation
2. **Correctness**: Ensure accurate orderbook state management
3. **Performance**: Use appropriate data structures for efficiency
4. **Maintainability**: Write clean, well-documented code
5. **Extensibility**: Design for future enhancements

The core challenge was maintaining an accurate orderbook state while efficiently generating MBP snapshots. The solution uses ordered containers to naturally maintain price levels and provides fast access for order modifications.

For high-frequency trading applications, this implementation provides a solid foundation that can be further optimized based on specific performance requirements.

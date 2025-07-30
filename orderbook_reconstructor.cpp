#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <cstring>

// Structure to represent an order
struct Order {
    std::string order_id;
    double price;
    int size;
    char side;

    Order() : price(0.0), size(0), side('N') {}
    Order(const std::string& id, double p, int s, char sd)
        : order_id(id), price(p), size(s), side(sd) {}

    // Comparison operator for use in std::set
    bool operator<(const Order& other) const {
        return order_id < other.order_id;
    }
};

// Structure to represent MBO data row
struct MBORow {
    std::string ts_recv;
    std::string ts_event;
    int rtype;
    int publisher_id;
    int instrument_id;
    char action;
    char side;
    double price;
    int size;
    int channel_id;
    std::string order_id;
    int flags;
    int ts_in_delta;
    int sequence;
    std::string symbol;
};

// Structure to represent MBP data row
struct MBPRow {
    std::string ts_recv;
    std::string ts_event;
    int rtype;
    int publisher_id;
    int instrument_id;
    char action;
    char side;
    int depth;
    double price;
    int size;
    int flags;
    int ts_in_delta;
    int sequence;
    std::vector<double> bid_prices;
    std::vector<int> bid_sizes;
    std::vector<int> bid_counts;
    std::vector<double> ask_prices;
    std::vector<int> ask_sizes;
    std::vector<int> ask_counts;
    std::string symbol;
    std::string order_id;
};

class OrderbookReconstructor {
private:
    // Orderbook state: price -> set of orders at that price
    std::map<double, std::set<Order>, std::greater<double>> bids;  // Descending order for bids
    std::map<double, std::set<Order>, std::less<double>> asks;     // Ascending order for asks

    // Order lookup: order_id -> order details
    std::map<std::string, Order> order_lookup;

    // Current row number for output
    int current_row = 0;

public:
    // Parse CSV line into MBORow
    MBORow parseMBOLine(const std::string& line) {
        std::stringstream ss(line);
        std::string token;
        MBORow row;

        std::getline(ss, row.ts_recv, ',');
        std::getline(ss, row.ts_event, ',');
        std::getline(ss, token, ','); row.rtype = std::stoi(token);
        std::getline(ss, token, ','); row.publisher_id = std::stoi(token);
        std::getline(ss, token, ','); row.instrument_id = std::stoi(token);
        std::getline(ss, token, ','); row.action = token[0];
        std::getline(ss, token, ','); row.side = token[0];
        std::getline(ss, token, ','); row.price = token.empty() ? 0.0 : std::stod(token);
        std::getline(ss, token, ','); row.size = std::stoi(token);
        std::getline(ss, token, ','); row.channel_id = std::stoi(token);
        std::getline(ss, row.order_id, ',');
        std::getline(ss, token, ','); row.flags = std::stoi(token);
        std::getline(ss, token, ','); row.ts_in_delta = std::stoi(token);
        std::getline(ss, token, ','); row.sequence = std::stoi(token);
        std::getline(ss, row.symbol, ',');

        return row;
    }

    // Process MBO action and update orderbook
    void processAction(const MBORow& row) {
        switch (row.action) {
            case 'A': // Add order
                addOrder(row);
                break;
            case 'C': // Cancel order
                cancelOrder(row);
                break;
            case 'T': // Trade
            case 'F': // Fill
                tradeOrder(row);
                break;
            case 'R': // Clear (reset orderbook)
                clearOrderbook();
                break;
        }
    }

    // Add order to orderbook
    void addOrder(const MBORow& row) {
        if (row.side == 'B') {
            Order order(row.order_id, row.price, row.size, row.side);
            bids[row.price].insert(order);
            order_lookup[row.order_id] = order;
        } else if (row.side == 'A') {
            Order order(row.order_id, row.price, row.size, row.side);
            asks[row.price].insert(order);
            order_lookup[row.order_id] = order;
        }
    }

    // Cancel order from orderbook
    void cancelOrder(const MBORow& row) {
        auto it = order_lookup.find(row.order_id);
        if (it != order_lookup.end()) {
            Order& order = it->second;
            if (order.side == 'B') {
                auto& price_level = bids[order.price];
                price_level.erase(order);
                if (price_level.empty()) {
                    bids.erase(order.price);
                }
            } else if (order.side == 'A') {
                auto& price_level = asks[order.price];
                price_level.erase(order);
                if (price_level.empty()) {
                    asks.erase(order.price);
                }
            }
            order_lookup.erase(it);
        }
    }

    // Handle trade/fill order
    void tradeOrder(const MBORow& row) {
        // If side is 'N', don't alter the orderbook
        if (row.side == 'N') {
            return;
        }

        // For trades, we need to reduce the size of existing orders
        // This is a simplified implementation - in reality, we'd need to match orders
        // For now, we'll just update the orderbook state
        auto it = order_lookup.find(row.order_id);
        if (it != order_lookup.end()) {
            Order& order = it->second;
            int trade_size = std::min(row.size, order.size);
            order.size -= trade_size;

            if (order.size <= 0) {
                // Remove order completely
                if (order.side == 'B') {
                    auto& price_level = bids[order.price];
                    price_level.erase(order);
                    if (price_level.empty()) {
                        bids.erase(order.price);
                    }
                } else if (order.side == 'A') {
                    auto& price_level = asks[order.price];
                    price_level.erase(order);
                    if (price_level.empty()) {
                        asks.erase(order.price);
                    }
                }
                order_lookup.erase(it);
            }
        }
    }

    // Clear the entire orderbook
    void clearOrderbook() {
        bids.clear();
        asks.clear();
        order_lookup.clear();
    }

    // Generate MBP row from current orderbook state
    MBPRow generateMBPRow(const MBORow& mbo_row) {
        MBPRow mbp_row;
        mbp_row.ts_recv = mbo_row.ts_event;  // Use ts_event for both fields
        mbp_row.ts_event = mbo_row.ts_event;
        mbp_row.rtype = 10; // MBP type
        mbp_row.publisher_id = mbo_row.publisher_id;
        mbp_row.instrument_id = mbo_row.instrument_id;

        // Combine F actions into T action, but keep C actions as C
        if (mbo_row.action == 'F') {
            mbp_row.action = 'T';
        } else {
            mbp_row.action = mbo_row.action;
        }

        mbp_row.side = mbo_row.side;
        mbp_row.depth = 0; // Will be calculated
        mbp_row.price = mbo_row.price;
        mbp_row.size = mbo_row.size;
        mbp_row.flags = mbo_row.flags;
        mbp_row.ts_in_delta = mbo_row.ts_in_delta;
        mbp_row.sequence = mbo_row.sequence; // Use original sequence
        mbp_row.symbol = mbo_row.symbol;
        mbp_row.order_id = mbo_row.order_id;

        // Initialize price level vectors
        mbp_row.bid_prices.resize(10, 0.0);
        mbp_row.bid_sizes.resize(10, 0);
        mbp_row.bid_counts.resize(10, 0);
        mbp_row.ask_prices.resize(10, 0.0);
        mbp_row.ask_sizes.resize(10, 0);
        mbp_row.ask_counts.resize(10, 0);

        // Fill bid levels (top 10)
        int bid_level = 0;
        for (const auto& price_level : bids) {
            if (bid_level >= 10) break;

            mbp_row.bid_prices[bid_level] = price_level.first;
            int total_size = 0;
            for (const auto& order : price_level.second) {
                total_size += order.size;
            }
            mbp_row.bid_sizes[bid_level] = total_size;
            mbp_row.bid_counts[bid_level] = price_level.second.size();
            bid_level++;
        }

        // Fill ask levels (top 10)
        int ask_level = 0;
        for (const auto& price_level : asks) {
            if (ask_level >= 10) break;

            mbp_row.ask_prices[ask_level] = price_level.first;
            int total_size = 0;
            for (const auto& order : price_level.second) {
                total_size += order.size;
            }
            mbp_row.ask_sizes[ask_level] = total_size;
            mbp_row.ask_counts[ask_level] = price_level.second.size();
            ask_level++;
        }

        // Calculate depth based on the action and side
        if (mbo_row.action == 'A') {
            if (mbo_row.side == 'B') {
                // Find the position of this price in bid levels
                int depth = 0;
                for (const auto& price_level : bids) {
                    if (price_level.first == mbo_row.price) {
                        mbp_row.depth = depth;
                        break;
                    }
                    depth++;
                }
            } else if (mbo_row.side == 'A') {
                // Find the position of this price in ask levels
                int depth = 0;
                for (const auto& price_level : asks) {
                    if (price_level.first == mbo_row.price) {
                        mbp_row.depth = depth;
                        break;
                    }
                    depth++;
                }
            }
        } else if (mbo_row.action == 'C') {
            // For cancel, find the depth of the order being cancelled
            auto it = order_lookup.find(mbo_row.order_id);
            if (it != order_lookup.end()) {
                const Order& order = it->second;
                if (order.side == 'B') {
                    int depth = 0;
                    for (const auto& price_level : bids) {
                        if (price_level.first == order.price) {
                            mbp_row.depth = depth;
                            break;
                        }
                        depth++;
                    }
                } else if (order.side == 'A') {
                    int depth = 0;
                    for (const auto& price_level : asks) {
                        if (price_level.first == order.price) {
                            mbp_row.depth = depth;
                            break;
                        }
                        depth++;
                    }
                }
            }
        }

        return mbp_row;
    }

    // Convert MBP row to CSV string
    std::string mbpRowToCSV(const MBPRow& row) {
        std::stringstream ss;

        // Add row number
        ss << current_row++ << ",";

        // Header fields
        ss << row.ts_recv << "," << row.ts_event << "," << row.rtype << ","
           << row.publisher_id << "," << row.instrument_id << "," << row.action << ","
           << row.side << "," << row.depth << ",";

        // Price formatting - use 2 decimal places for non-zero prices
        if (row.price > 0.0) {
            ss << std::fixed << std::setprecision(2) << row.price;
        }
        ss << "," << row.size << "," << row.flags << "," << row.ts_in_delta << "," << row.sequence;

        // Bid and ask levels
        for (int i = 0; i < 10; i++) {
            // Bid price
            if (row.bid_prices[i] > 0.0) {
                ss << "," << std::fixed << std::setprecision(2) << row.bid_prices[i];
            } else {
                ss << ",";
            }
            ss << "," << row.bid_sizes[i] << "," << row.bid_counts[i];

            // Ask price
            if (row.ask_prices[i] > 0.0) {
                ss << "," << std::fixed << std::setprecision(2) << row.ask_prices[i];
            } else {
                ss << ",";
            }
            ss << "," << row.ask_sizes[i] << "," << row.ask_counts[i];
        }

        ss << "," << row.symbol << "," << row.order_id;

        return ss.str();
    }

    // Helper: compare two MBP-10 snapshots (top 10 levels)
    bool mbpSnapshotChanged(const MBPRow& a, const MBPRow& b) {
        for (int i = 0; i < 10; ++i) {
            if (a.bid_prices[i] != b.bid_prices[i] ||
                a.bid_sizes[i] != b.bid_sizes[i] ||
                a.bid_counts[i] != b.bid_counts[i] ||
                a.ask_prices[i] != b.ask_prices[i] ||
                a.ask_sizes[i] != b.ask_sizes[i] ||
                a.ask_counts[i] != b.ask_counts[i]) {
                return true;
            }
        }
        return false;
    }

    // Process MBO file and generate MBP output
    void processFile(const std::string& input_file, const std::string& output_file) {
        std::ifstream infile(input_file);
        std::ofstream outfile(output_file);

        if (!infile.is_open()) {
            std::cerr << "Error: Could not open input file " << input_file << std::endl;
            return;
        }

        if (!outfile.is_open()) {
            std::cerr << "Error: Could not open output file " << output_file << std::endl;
            return;
        }

        std::string line;
        bool first_line = true;

        // Write header
        outfile << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence";
        for (int i = 0; i < 10; i++) {
            outfile << ",bid_px_" << std::setfill('0') << std::setw(2) << i
                   << ",bid_sz_" << std::setfill('0') << std::setw(2) << i
                   << ",bid_ct_" << std::setfill('0') << std::setw(2) << i
                   << ",ask_px_" << std::setfill('0') << std::setw(2) << i
                   << ",ask_sz_" << std::setfill('0') << std::setw(2) << i
                   << ",ask_ct_" << std::setfill('0') << std::setw(2) << i;
        }
        outfile << ",symbol,order_id" << std::endl;

        MBPRow prev_snapshot; // previous MBP-10 snapshot
        bool prev_snapshot_valid = false;
        while (std::getline(infile, line)) {
            if (first_line) {
                first_line = false;
                continue; // Skip header
            }

            MBORow mbo_row = parseMBOLine(line);

            // Skip the initial clear action
            if (mbo_row.action == 'R' && current_row == 0) {
                MBPRow mbp_row = generateMBPRow(mbo_row);
                outfile << mbpRowToCSV(mbp_row) << std::endl;
                prev_snapshot = mbp_row;
                prev_snapshot_valid = true;
                continue;
            }

            // Process the action
            processAction(mbo_row);

            // Generate MBP snapshot
            MBPRow mbp_row = generateMBPRow(mbo_row);
            // Only write if MBP-10 snapshot changed, or if this is the first snapshot
            if (!prev_snapshot_valid || mbpSnapshotChanged(mbp_row, prev_snapshot)) {
                outfile << mbpRowToCSV(mbp_row) << std::endl;
                prev_snapshot = mbp_row;
                prev_snapshot_valid = true;
            }
        }

        infile.close();
        outfile.close();
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mbo_input_file>" << std::endl;
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = "mbp_output.csv";

    OrderbookReconstructor reconstructor;
    reconstructor.processFile(input_file, output_file);

    std::cout << "Orderbook reconstruction completed. Output saved to " << output_file << std::endl;

    return 0;
}

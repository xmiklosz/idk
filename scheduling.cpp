#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>

using namespace std;

// Simple job structure
struct JobData {
    int number;
    int deadline;
    int profit;
};

// Sort by profit (highest first)
bool sortByProfit(const JobData &a, const JobData &b) {
    return a.profit > b.profit;
}

// ============================================
// PART 1: Basic Algorithm (Algorithm 4.4)
// ============================================

// Check if a sequence of jobs is feasible
// Feasible means: when sorted by deadline, each job at position p has deadline >= p
bool checkFeasible(vector<JobData> sequence) {
    // Sort by deadline first
    sort(sequence.begin(), sequence.end(), [](const JobData &a, const JobData &b) {
        return a.deadline < b.deadline;
    });

    // Check if each job can meet its deadline
    for (size_t position = 0; position < sequence.size(); position++) {
        // Position is 0-indexed, but time slots are 1-indexed
        int timeSlot = position + 1;
        if (timeSlot > sequence[position].deadline) {
            return false;  // Job would miss its deadline
        }
    }

    return true;
}

// Main scheduling algorithm - Algorithm 4.4 from textbook
void scheduleBasic(int n, const int deadlines[], const int profits[],
                   vector<int> &finalSequence, int &totalProfit) {

    // Create array of jobs
    vector<JobData> jobs(n);
    for (int i = 0; i < n; i++) {
        jobs[i].number = i + 1;
        jobs[i].deadline = deadlines[i];
        jobs[i].profit = profits[i];
    }

    // Sort jobs by profit (descending) as per algorithm requirement
    sort(jobs.begin(), jobs.end(), sortByProfit);

    // Algorithm 4.4 implementation
    // J starts with first job
    vector<JobData> J;
    J.push_back(jobs[0]);

    // Try adding each remaining job
    for (int i = 1; i < n; i++) {
        // Create K = J with job i added
        vector<JobData> K = J;
        K.push_back(jobs[i]);

        // Check if K is feasible
        if (checkFeasible(K)) {
            // Accept the new job
            J = K;
        }
        // Otherwise reject it (J stays unchanged)
    }

    // Sort final result by deadline for execution order
    sort(J.begin(), J.end(), [](const JobData &a, const JobData &b) {
        return a.deadline < b.deadline;
    });

    // Convert to job numbers and calculate total profit
    finalSequence.clear();
    totalProfit = 0;
    for (const auto &job : J) {
        finalSequence.push_back(job.number);
        totalProfit += job.profit;
    }
}

// ============================================
// PART 2: Optimized with Disjoint Sets
// ============================================

// Disjoint Set (Union-Find) Data Structure with smallest element tracking
class DisjointSet {
private:
    vector<int> parent;
    vector<int> smallest;  // Track smallest element in each set

public:
    DisjointSet(int n) {
        parent.resize(n);
        smallest.resize(n);
        // Initialize: each element is its own parent and smallest
        for (int i = 0; i < n; i++) {
            parent[i] = i;
            smallest[i] = i;
        }
    }

    // Find with path compression
    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]);  // Path compression
        }
        return parent[x];
    }

    // Union: merge set containing y into set containing x
    // After merge, smallest of the combined set = smallest of set containing x
    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);

        if (rootX == rootY) return;

        // Make rootX the parent of rootY
        parent[rootY] = rootX;
        // The smallest element of the merged set is min of both
        smallest[rootX] = min(smallest[rootX], smallest[rootY]);
    }

    // Find smallest member in the set containing x
    int small(int x) {
        int root = find(x);
        return smallest[root];
    }
};

// Optimized scheduling with disjoint sets
void scheduleOptimized(int n, const int deadlines[], const int profits[],
                       vector<int> &finalSequence, int &totalProfit) {

    // Create array of jobs
    vector<JobData> jobs(n);
    for (int i = 0; i < n; i++) {
        jobs[i].number = i + 1;
        jobs[i].deadline = deadlines[i];
        jobs[i].profit = profits[i];
    }

    // Sort jobs by profit (descending)
    sort(jobs.begin(), jobs.end(), sortByProfit);

    // Find maximum deadline
    int d = 0;
    for (int i = 0; i < n; i++) {
        d = max(d, deadlines[i]);
    }

    // Initialize disjoint sets for time slots 0, 1, 2, ..., d
    DisjointSet ds(d + 1);

    // Schedule to store job assignments
    vector<int> schedule(d + 1, 0);  // schedule[i] = job number scheduled at time i (0 means empty)

    // Process each job in order of decreasing profit
    for (int i = 0; i < n; i++) {
        // Find the set containing min(deadline, d)
        int slot = min(jobs[i].deadline, d);

        // Get the smallest available time in this set
        int t = ds.small(slot);

        if (t > 0) {
            // Schedule job at time t
            schedule[t] = jobs[i].number;

            // Merge set containing t with set containing t-1
            ds.unite(t - 1, t);
        }
        // If t == 0, reject the job
    }

    // Build final sequence from schedule
    finalSequence.clear();
    totalProfit = 0;
    for (int t = 1; t <= d; t++) {
        if (schedule[t] > 0) {
            finalSequence.push_back(schedule[t]);
            totalProfit += profits[schedule[t] - 1];
        }
    }
}

// ============================================
// Main Program
// ============================================

int main() {
    // Test with Table 1.1 from assignment
    const int n = 7;
    int deadlines[] = {2, 4, 3, 2, 3, 1, 1};
    int profits[] = {40, 15, 60, 20, 10, 45, 55};

    vector<int> result1, result2;
    int totalProfit1, totalProfit2;

    cout << "==========================================\n";
    cout << "   SCHEDULING WITH DEADLINES\n";
    cout << "   AZA 2025/26 Assignment\n";
    cout << "==========================================\n\n";

    // Show input (Table 1.1)
    cout << "Input Jobs (Table 1.1):\n";
    cout << "Job | Deadline | Profit\n";
    cout << "----+----------+-------\n";
    for (int i = 0; i < n; i++) {
        cout << " " << (i + 1) << "  |    " << deadlines[i]
             << "     |   " << profits[i] << "\n";
    }
    cout << "\n";

    // ==========================================
    // PART 1: Basic Algorithm
    // ==========================================
    cout << "==========================================\n";
    cout << "PART 1: Basic Algorithm (Algorithm 4.4)\n";
    cout << "Time Complexity: O(n²)\n";
    cout << "==========================================\n\n";

    scheduleBasic(n, deadlines, profits, result1, totalProfit1);

    cout << "Optimal Schedule:\n";
    cout << "Time | Job | Profit\n";
    cout << "-----+-----+-------\n";

    for (size_t time = 0; time < result1.size(); time++) {
        int jobNum = result1[time];
        int jobProfit = profits[jobNum - 1];

        cout << "  " << (time + 1) << "  |  " << jobNum << "  |   "
             << jobProfit << "\n";
    }

    cout << "-----+-----+-------\n";
    cout << "Total Profit: " << totalProfit1 << "\n\n";

    cout << "Job Sequence: [";
    for (size_t i = 0; i < result1.size(); i++) {
        cout << result1[i];
        if (i < result1.size() - 1) cout << ", ";
    }
    cout << "]\n\n";

    // ==========================================
    // PART 2: Optimized with Disjoint Sets
    // ==========================================
    cout << "==========================================\n";
    cout << "PART 2: Optimized with Disjoint Sets\n";
    cout << "Time Complexity: O(n α(n)) ≈ O(n)\n";
    cout << "==========================================\n\n";

    scheduleOptimized(n, deadlines, profits, result2, totalProfit2);

    cout << "Optimal Schedule:\n";
    cout << "Time | Job | Profit\n";
    cout << "-----+-----+-------\n";

    for (size_t time = 0; time < result2.size(); time++) {
        int jobNum = result2[time];
        int jobProfit = profits[jobNum - 1];

        cout << "  " << (time + 1) << "  |  " << jobNum << "  |   "
             << jobProfit << "\n";
    }

    cout << "-----+-----+-------\n";
    cout << "Total Profit: " << totalProfit2 << "\n\n";

    cout << "Job Sequence: [";
    for (size_t i = 0; i < result2.size(); i++) {
        cout << result2[i];
        if (i < result2.size() - 1) cout << ", ";
    }
    cout << "]\n\n";

    // Verify both give same result
    if (totalProfit1 == totalProfit2) {
        cout << "✓ Both algorithms produce same total profit: " << totalProfit1 << "\n\n";
    } else {
        cout << "⚠ Warning: Algorithms produced different results!\n\n";
    }

    // Complexity analysis
    cout << "==========================================\n";
    cout << "Complexity Analysis:\n";
    cout << "==========================================\n";
    cout << "Basic Algorithm (Part 1):\n";
    cout << "  - Sorting: O(n log n)\n";
    cout << "  - Main loop: n iterations\n";
    cout << "  - Feasibility check per iteration: O(n log n)\n";
    cout << "  - Overall: O(n²·log n)\n\n";

    cout << "Optimized Algorithm (Part 2):\n";
    cout << "  - Sorting: O(n log n)\n";
    cout << "  - Main loop: n iterations\n";
    cout << "  - Disjoint set operations: O(α(n)) ≈ O(1)\n";
    cout << "  - Overall: O(n log n)\n";
    cout << "  - Significant improvement for large n!\n";

    return 0;
}

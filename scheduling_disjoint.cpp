#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

// Disjoint Set Data Structure III - Based on textbook pseudocode
class DisjointSetIII {
private:
    struct nodetype {
        int parent;
        int depth;
        int smallest;
    };

    vector<nodetype> U;
    int n;

public:
    DisjointSetIII(int size) : n(size) {
        U.resize(n + 1);
    }

    void makeset(int i) {
        U[i].parent = i;
        U[i].depth = 0;
        U[i].smallest = i;
    }

    // Find with path compression
    int find(int i) {
        if (U[i].parent != i) {
            U[i].parent = find(U[i].parent);
        }
        return U[i].parent;
    }

    // Merge two sets - union by rank with smallest tracking
    void merge(int p, int q) {
        int rootP = find(p);
        int rootQ = find(q);

        if (rootP == rootQ) return;

        // Union by depth/rank
        if (U[rootP].depth == U[rootQ].depth) {
            U[rootQ].parent = rootP;
            U[rootP].depth++;
            U[rootP].smallest = min(U[rootP].smallest, U[rootQ].smallest);
        } else if (U[rootP].depth < U[rootQ].depth) {
            U[rootP].parent = rootQ;
            U[rootQ].smallest = min(U[rootP].smallest, U[rootQ].smallest);
        } else {
            U[rootQ].parent = rootP;
            U[rootP].smallest = min(U[rootP].smallest, U[rootQ].smallest);
        }
    }

    // Get smallest element in the set containing i
    int small(int i) {
        int root = find(i);
        return U[root].smallest;
    }
};

// Job representation
struct JobInfo {
    int jobNumber;
    int dueBy;
    int value;
};

// Comparator to sort by profit descending
bool profitComparator(const JobInfo &first, const JobInfo &second) {
    return first.value > second.value;
}

// Main scheduling function using disjoint sets
void schedule(int numJobs, const int deadlines[], const int profits[],
              vector<int> &result, int &totalProfit) {

    // Step 1: Create job list with all info
    vector<JobInfo> allJobs;
    int largestDeadline = 0;

    for (int i = 0; i < numJobs; i++) {
        JobInfo job;
        job.jobNumber = i + 1;
        job.dueBy = deadlines[i];
        job.value = profits[i];
        allJobs.push_back(job);

        if (deadlines[i] > largestDeadline) {
            largestDeadline = deadlines[i];
        }
    }

    // Step 2: Sort by profit (greedy - take highest paying first)
    sort(allJobs.begin(), allJobs.end(), profitComparator);

    // Step 3: Setup disjoint set for time slots 0 to largestDeadline
    DisjointSetIII timeslots(largestDeadline + 1);
    for (int slot = 0; slot <= largestDeadline; slot++) {
        timeslots.makeset(slot);
    }

    // Step 4: Try to schedule each job
    result.clear();
    totalProfit = 0;
    vector<int> scheduledAt;  // Track which time slot each job uses

    for (int i = 0; i < numJobs; i++) {
        JobInfo currentJob = allJobs[i];

        // Find the latest free time slot before or at deadline
        int deadline = min(currentJob.dueBy, largestDeadline);
        int freeSlot = timeslots.small(deadline);

        // If we found a valid slot (not 0), schedule it
        if (freeSlot > 0) {
            result.push_back(currentJob.jobNumber);
            scheduledAt.push_back(freeSlot);
            totalProfit += currentJob.value;

            // Mark this slot as used by merging with previous slot
            timeslots.merge(freeSlot - 1, freeSlot);
        }
    }

    // Step 5: Sort output by time slot for display
    vector<pair<int, int>> finalSchedule;
    for (size_t i = 0; i < result.size(); i++) {
        finalSchedule.push_back(make_pair(scheduledAt[i], result[i]));
    }
    sort(finalSchedule.begin(), finalSchedule.end());

    // Update result with sorted order
    result.clear();
    for (size_t i = 0; i < finalSchedule.size(); i++) {
        result.push_back(finalSchedule[i].second);
    }
}

int main() {
    // Test data from assignment Table 1.1
    const int n = 7;
    int deadlines[] = {2, 4, 3, 2, 3, 1, 1};
    int profits[] = {40, 15, 60, 20, 10, 45, 55};

    vector<int> scheduledJobs;
    int totalProfit;

    cout << "==========================================\n";
    cout << "   SCHEDULING WITH DEADLINES\n";
    cout << "   Disjoint Set Data Structure (DS III)\n";
    cout << "   AZA 2025/26 Assignment - PART 2\n";
    cout << "==========================================\n\n";

    // Display input
    cout << "Input Jobs (Table 1.1):\n";
    cout << "Job | Deadline | Profit\n";
    cout << "----+----------+-------\n";
    for (int i = 0; i < n; i++) {
        cout << " " << (i+1) << "  |    " << deadlines[i]
             << "     |   " << profits[i] << "\n";
    }
    cout << "\n";

    // Run scheduling algorithm
    schedule(n, deadlines, profits, scheduledJobs, totalProfit);

    // Display results
    cout << "Optimal Schedule (in time order):\n";
    cout << "Time | Job | Profit\n";
    cout << "-----+-----+-------\n";

    for (size_t t = 0; t < scheduledJobs.size(); t++) {
        int jobNum = scheduledJobs[t];
        int jobProfit = profits[jobNum - 1];

        cout << "  " << (t+1) << "  |  " << jobNum << "  |   "
             << jobProfit << "\n";
    }

    cout << "-----+-----+-------\n";
    cout << "Total Profit: " << totalProfit << "\n\n";

    // Show the sequence
    cout << "Job Sequence: [";
    for (size_t i = 0; i < scheduledJobs.size(); i++) {
        cout << scheduledJobs[i];
        if (i < scheduledJobs.size() - 1) {
            cout << ", ";
        }
    }
    cout << "]\n\n";

    // Complexity discussion
    cout << "==========================================\n";
    cout << "Algorithm Complexity Analysis:\n";
    cout << "==========================================\n";
    cout << "Using Disjoint Set with:\n";
    cout << "- Path compression in find()\n";
    cout << "- Union by rank in merge()\n";
    cout << "- Smallest element tracking\n\n";

    cout << "Time Complexity:\n";
    cout << "- Sorting jobs: O(n log n)\n";
    cout << "- Initializing d+1 sets: O(d)\n";
    cout << "- Processing n jobs: O(n × α(d))\n";
    cout << "  where α is inverse Ackermann function\n";
    cout << "- Overall: O(n log n + n × α(d))\n";
    cout << "  ≈ O(n log n) for practical purposes\n\n";

    cout << "For this problem:\n";
    cout << "- n = " << n << " jobs\n";
    cout << "- d = " << *max_element(deadlines, deadlines + n)
         << " max deadline\n";
    cout << "- α(d) ≈ 1-2 (nearly constant)\n\n";

    cout << "Space Complexity: O(d) for disjoint set\n\n";

    cout << "Algorithm Steps:\n";
    cout << "1. Sort jobs by profit (descending)\n";
    cout << "2. Create disjoint sets for time slots 0..d\n";
    cout << "3. For each job (highest profit first):\n";
    cout << "   a. Find latest available slot ≤ deadline\n";
    cout << "   b. If slot > 0: schedule job, merge with slot-1\n";
    cout << "   c. Otherwise: reject job\n";
    cout << "4. Return scheduled jobs\n\n";

    cout << "Key Optimization:\n";
    cout << "- Disjoint sets efficiently find available slots\n";
    cout << "- After using slot t, merge with t-1\n";
    cout << "- Future queries skip over used slots automatically\n";
    cout << "- Much faster than O(n²) feasibility checking!\n";

    return 0;
}

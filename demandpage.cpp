// Imports
#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <functional>
#include <queue>
#include <stdexcept>   
using namespace std;

// Global variables
int PAGE_SIZE = 200;     // Page size and page frame size
int TOTAL_MEMORY = 2000; // Total memory available 

// For thread safety
mutex mtx; 

// Struct for each page of a job
struct Page {
    int size_of_content;
    int page_no;
    int job_number;
};

// Struct for each Job
struct Job {
    int number;
    int size;
    vector<Page> pages;
};

// Struct for each page frame in physical memory
struct PageFrame {
    int size_of_content;
    int page_frame_no;
    bool is_occupied = false;
    int job_no = -1;
    int page_no = -1;
    time_t time_loaded = 0; 
    time_t last_used = 0;    
};

// Struct for Page Map Table entry for virtual → physical mapping
struct PageMapTableEntry {
    int page_no, page_frame_no = -1;
    bool modified = false, referenced = false, status = false;
    time_t time_loaded = 0;
    time_t last_used = 0;
};

// Struct for Memory Map Table entry
struct MemoryMapTableEntry {
    int page_frame_no;
    bool is_occupied;
};

// Struct for Job Table entry 
struct JobTableEntry {
    int job_no, PMT_ID;
};

// Function declarations
void acceptJobs(int n, vector<Job>& jobs);
void moveJobsToPages(vector<Job>& jobs, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
void processJobs(vector<Job>& jobs, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
void processIndividualJob(Job& job, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables, const string& algorithm);
PageMapTableEntry& getPageMapTableEntryByPageNumber(int page_no, vector<PageMapTableEntry>& pageMapTable);
int findFrameToReplace(vector<PageFrame>& pageFrames, const string& algorithm);
void addressResolution(int logical_addr, int page_size, int frame_no);
void printMemoryState(const vector<PageFrame>& pageFrames);

// Main program
int main() {
    vector<Job> jobs;
    int n;

    cout << "Enter the number of jobs: ";
    cin >> n;

    acceptJobs(n, jobs);

    int num_page_frames = ceil((float)TOTAL_MEMORY / PAGE_SIZE);
    vector<PageFrame> pageFrames(num_page_frames);
    vector<JobTableEntry> jobTable(n);
    vector<MemoryMapTableEntry> memoryMapTable;
    vector<vector<PageMapTableEntry>> pageMapTables;

    // Create memory frames and initialize times
    for (int i = 0; i < num_page_frames; ++i) {
        pageFrames[i].page_frame_no = i;
        pageFrames[i].is_occupied = false;
        pageFrames[i].time_loaded = 0;
        pageFrames[i].last_used = 0;

        MemoryMapTableEntry entry;
        entry.page_frame_no = i;
        entry.is_occupied = false;
        memoryMapTable.push_back(entry);
    }

    moveJobsToPages(jobs, jobTable, pageMapTables);
    processJobs(jobs, pageFrames, memoryMapTable, jobTable, pageMapTables);

    return 0;
}

// Accept Jobs
void acceptJobs(int n, vector<Job>& jobs) {
    for (int i = 0; i < n; ++i) {
        int size;
        Job job;
        cout << "Enter the size of job " << i + 1 << ": ";
        cin >> size;
        job.number = i;
        job.size = size;
        jobs.push_back(job);
    }
}

// Divide jobs into pages and create PMT and Job Table
void moveJobsToPages(vector<Job>& jobs, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables) {
    int pageNo = 0;
    for (int i = 0; i < (int)jobs.size(); ++i) {
        auto& job = jobs[i];
        JobTableEntry jobTableEntry;
        jobTableEntry.job_no = job.number;
        jobTableEntry.PMT_ID = i;
        // Save to jobTable 
        if (i < (int)jobTable.size()) {
            jobTable[i] = jobTableEntry;
        } else {
            jobTable.push_back(jobTableEntry);
        }

        vector<PageMapTableEntry> pageMapTable;

        int num_pages = ceil((float)job.size / PAGE_SIZE);
        if (num_pages == 0) num_pages = 1;
        for (int j = 0; j < num_pages; ++j) {
            PageMapTableEntry PMT_Entry;
            Page newPage;
            newPage.job_number = job.number;
            newPage.page_no = pageNo;
            newPage.size_of_content = (j == num_pages - 1) ? (job.size % PAGE_SIZE == 0 ? PAGE_SIZE : job.size % PAGE_SIZE) : PAGE_SIZE;
            job.pages.push_back(newPage);

            PMT_Entry.page_no = pageNo;
            PMT_Entry.page_frame_no = -1;
            PMT_Entry.status = false;
            PMT_Entry.modified = false;
            PMT_Entry.referenced = false;
            PMT_Entry.time_loaded = 0;
            PMT_Entry.last_used = 0;

            pageMapTable.push_back(PMT_Entry);
            pageNo++;
        }
        pageMapTables.push_back(pageMapTable);
    }
}

// Finding page in PMT
PageMapTableEntry& getPageMapTableEntryByPageNumber(int page_no, vector<PageMapTableEntry>& pageMapTable) {
    for (auto& row : pageMapTable) {
        if (row.page_no == page_no)
            return row;
    }
    throw runtime_error("Page not found in PMT!");
}

// Process all jobs
void processJobs(vector<Job>& jobs, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables) {
    cout << "\nChoose page replacement algorithm (FIFO/LRU): ";
    string algorithm;
    cin >> algorithm;

    cout << "Using algorithm: " << algorithm << "\n";

    vector<thread> threads;

    for (auto& job : jobs) {
        threads.push_back(thread(processIndividualJob, ref(job), ref(pageFrames), ref(memoryMapTable), ref(jobTable), ref(pageMapTables), cref(algorithm)));
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Process individual job
void processIndividualJob(Job& job, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables, const string& algorithm) {

    // lock_guard to avoid data races on shared structures 
    lock_guard<mutex> lock(mtx);
    srand((unsigned)time(0) + job.number);

    cout << "\nJob " << job.number + 1 << " is running...\n";
    if (job.pages.empty()) {
        cout << "Job " << job.number << " has no pages (size 0). Skipping.\n";
        return;
    }

    for (int access = 0; access < (int)job.pages.size(); ++access) {
        vector<PageMapTableEntry>& pageMapTable = pageMapTables[jobTable[job.number].PMT_ID];
        int randomPageIndex = rand() % job.pages.size();

        Page requestedPage = job.pages[randomPageIndex];
        cout << "Requesting Page " << requestedPage.page_no << " of Job " << job.number + 1 << endl;

        // Find page in PMT
        PageMapTableEntry& row = getPageMapTableEntryByPageNumber(requestedPage.page_no, pageMapTable);

        if (row.status && row.page_frame_no >= 0) {
            // Page is already loaded
            cout << " -> Page already in memory (Frame " << row.page_frame_no << ")\n";
            row.referenced = true;
            row.last_used = time(0);

            // update frame's last_used
            if (row.page_frame_no < (int)pageFrames.size()) {
                pageFrames[row.page_frame_no].last_used = time(0);
            }
        } else {
            cout << " -> Page Fault occurred!\n";

            // Find free frame
            int free_frame_no = -1;
            for (auto& frame : pageFrames) {
                if (!frame.is_occupied) {
                    free_frame_no = frame.page_frame_no;
                    break;
                }
            }

            // If no free frame, perform replacement
            if (free_frame_no == -1) {
                free_frame_no = findFrameToReplace(pageFrames, algorithm);
                cout << " -> Replacing Frame " << free_frame_no << " using " << algorithm << endl;

                // If old frame had a page, mark that page as not in memory (update PMT)
                int oldJob = pageFrames[free_frame_no].job_no;
                int oldPage = pageFrames[free_frame_no].page_no;
                if (oldPage != -1) {
                    // find the old job's PMT and clear the entry
                    for (auto& jt : jobTable) {
                        if (jt.job_no == oldJob) {
                            vector<PageMapTableEntry>& oldPMT = pageMapTables[jt.PMT_ID];
                            for (auto& entry : oldPMT) {
                                if (entry.page_no == oldPage) {
                                    entry.status = false;
                                    entry.page_frame_no = -1;
                                    entry.last_used = 0;
                                    entry.time_loaded = 0;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
            }

            // Load page into the frame
            pageFrames[free_frame_no].is_occupied = true;
            pageFrames[free_frame_no].job_no = job.number;
            pageFrames[free_frame_no].page_no = requestedPage.page_no;
            pageFrames[free_frame_no].time_loaded = time(0);
            pageFrames[free_frame_no].last_used = time(0);

            // Update PMT
            row.status = true;
            row.page_frame_no = free_frame_no;
            row.time_loaded = time(0);
            row.last_used = time(0);
        }

        // Perform address resolution using the frame that now contains the page
        int logical_address = rand() % job.size;
        if (row.page_frame_no >= 0)
            addressResolution(logical_address, PAGE_SIZE, row.page_frame_no);
        else
            addressResolution(logical_address, PAGE_SIZE, -1);

        // Print memory state for clarity
        printMemoryState(pageFrames);
    }
}

// FIFO or LRU to find frame to replace
int findFrameToReplace(vector<PageFrame>& pageFrames, const string& algorithm) {
    // if some frames are free, return the first free as a fallback
    for (int i = 0; i < (int)pageFrames.size(); ++i) {
        if (!pageFrames[i].is_occupied) return i;
    }

    if (algorithm == "FIFO") {
        // choose the frame with the earliest time_loaded (oldest)
        time_t oldest = pageFrames[0].time_loaded;
        int victim = 0;
        for (int i = 1; i < (int)pageFrames.size(); ++i) {
            if (pageFrames[i].time_loaded < oldest) {
                oldest = pageFrames[i].time_loaded;
                victim = i;
            }
        }
        return victim;
    } else {
        // LRU: choose the frame with the earliest last_used
        time_t oldest = pageFrames[0].last_used;
        int victim = 0;
        for (int i = 1; i < (int)pageFrames.size(); ++i) {
            if (pageFrames[i].last_used < oldest) {
                oldest = pageFrames[i].last_used;
                victim = i;
            }
        }
        return victim;
    }
}

// Address resolution from logical to physical
void addressResolution(int logical_addr, int page_size, int frame_no) {
    if (frame_no < 0) {
        cout << " -> Address resolution failed: page not in memory\n";
        return;
    }
    int page_no = logical_addr / page_size;
    int offset = logical_addr % page_size;
    int physical_addr = frame_no * page_size + offset;
    cout << " -> Logical Address " << logical_addr << " => Page " << page_no << ", Offset " << offset
         << " => Physical Address " << physical_addr << endl;
}

// Print a simple snapshot of memory frames
void printMemoryState(const vector<PageFrame>& pageFrames) {
    cout << " Memory frames snapshot:\n";
    for (const auto& f : pageFrames) {
        if (f.is_occupied) {
            cout << "  Frame " << f.page_frame_no << ": Job " << f.job_no + 1 << ", Page " << f.page_no << "\n";
        } else {
            cout << "  Frame " << f.page_frame_no << ": [Empty]\n";
        }
    }
    cout << endl;
}

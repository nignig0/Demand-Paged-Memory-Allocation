#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <functional>
#include <chrono>
#include <climits>
using namespace std;

int PAGE_SIZE = 200; //Page size and page frame size
int TOTAL_MEMORY = 20000; //total memory available


// Thread-safe statistics
mutex mtx;
int page_faults = 0, page_hits = 0, page_replacements = 0;


struct Page {
    int size_of_content;
    int page_no;
    int job_number;
};


struct Job {
    int number;
    int size;
    vector<Page> pages;
};




struct PageFrame {
    int size_of_content;
    int page_frame_no;
    int page_no = -1; // Added missing member
};


struct PageMapTableEntry {
    int page_no, page_frame_no = -1;
    bool modified = false, referenced = false, status = false;
    //add a timestamp here so we can know when last something was used 
    //add a timestamp here so we can know when something first got into memory
    //the timestamps will make it easier to implement FIFO and LRU
    long long last_access_time = 0; // timestamp for LRU
    long long load_time = 0; // Added missing timestamp for load
};


struct MemoryMapTableEntry {
    int page_frame_no;
    bool is_occupied;
    int page_no = -1; // which page is in this frame
};


struct JobTableEntry {
    int job_no, PMT_ID;
};


// Function declarations
void acceptJobs(int n, vector<Job>& jobs);
void moveJobsToPages(vector<Job>& jobs, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
void processJobs(vector<Job>& jobs, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
void processIndividualJob(Job& job, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
PageMapTableEntry* getPageMapTableEntryByPageNumber(int page_no, vector<PageMapTableEntry>& pageMapTable);
int findLRUVictim(vector<PageMapTableEntry>& pageMapTable); //
long long getCurrentTimestamp();//


int main(){
    vector<Job> jobs;
    int n;

    cout<<"Enter the number of jobs: "<<endl;
    cin>> n;

    acceptJobs(n, jobs);

    int num_page_frames = ceil((double)TOTAL_MEMORY / PAGE_SIZE);
    vector<PageFrame> pageFrames;

    vector<JobTableEntry> jobTable(n);
    vector<MemoryMapTableEntry> memoryMapTable;
    
    vector<vector<PageMapTableEntry>> pageMapTables;
    //a vector that holds all the page map tables

    // Seed RNG once before threads!
    srand(static_cast<unsigned>(time(nullptr)));

    //creating pageFrames!!!
    for(int i = 0; i< num_page_frames; ++i){
        PageFrame pageFrame;
        pageFrame.size_of_content = 0;
        pageFrame.page_frame_no = i;


        pageFrame.page_no = -1;//
 
        pageFrames.push_back(pageFrame);

        MemoryMapTableEntry memoryMapTableEntry;
        memoryMapTableEntry.page_frame_no = i;
        memoryMapTableEntry.is_occupied = false;
        memoryMapTableEntry.page_no = -1;//
        memoryMapTable.push_back(memoryMapTableEntry);
    }

    moveJobsToPages(jobs, jobTable, pageMapTables);
    processJobs(jobs, pageFrames, memoryMapTable, jobTable, pageMapTables);

    // Printing the statistics
    cout << "\nTHE STATISTICS" << endl;
    cout << "Page Faults: " << page_faults << endl;
    cout << "Page Hits: " << page_hits << endl;
    cout << "Page Replacements: " << page_replacements << endl;
    cout << "Total Page Requests: " << (page_faults + page_hits) << endl;
    if (page_faults + page_hits > 0) {
        cout << "Hit Rate: " << (100.0 * page_hits / (page_faults + page_hits)) << "%" << endl;
    }

    return 0;
}


void acceptJobs(int n, vector<Job>& jobs){
    for(int i = 0; i<n; ++i){
        int size;
        Job job;
        cout<<"Enter the size of job "<<i<<endl; // Fixed index increment issue
        cin>>size;
        job.number = i;
        job.size = size;

        jobs.push_back(job);
    }
}


void moveJobsToPages(vector<Job>& jobs, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables){
    int pageNo = 0;
    for(int i = 0; i<jobs.size(); ++i){
        auto& job = jobs[i];
        
        JobTableEntry jobTableEntry;
        jobTableEntry.job_no = job.number;
        jobTableEntry.PMT_ID = i;
        jobTable[i] = jobTableEntry;
        vector<PageMapTableEntry> pageMapTable;        

        int num_pages = ceil((double)job.size/PAGE_SIZE);
        for(int j = 0; j< num_pages; ++j){
            PageMapTableEntry PMT_Entry;

            Page newPage;
            newPage.job_number = job.number;
            newPage.page_no = pageNo;
            newPage.size_of_content = (j == num_pages - 1) ? job.size % PAGE_SIZE : PAGE_SIZE;
            //if we are on the last page for the job, then the size of 
            if (newPage.size_of_content == 0) newPage.size_of_content = PAGE_SIZE;
            job.pages.push_back(newPage);

            PMT_Entry.page_no = pageNo;
            PMT_Entry.page_frame_no = -1;
            PMT_Entry.modified = false;
            PMT_Entry.referenced = false;
            PMT_Entry.status = false;
            PMT_Entry.last_access_time = 0;
            PMT_Entry.load_time = 0;
            pageMapTable.push_back(PMT_Entry);

            pageNo++;
        }
        pageMapTables.push_back(pageMapTable);
    }
}


// Return a pointer to ensure updates reflect in table
PageMapTableEntry* getPageMapTableEntryByPageNumber(int page_no, vector<PageMapTableEntry>& pageMapTable){
    for(auto& row: pageMapTable){
        if(row.page_no == page_no) return &row;
    }
    return nullptr;

    // PageMapTableEntry invalid;
    // invalid.page_no = -1;
    // invalid.page_frame_no = -1;
    // invalid.modified = false;
    // invalid.referenced = false;
    // invalid.status = false;

    // cout<<"Error!"<<endl;
    // return invalid;
}
//////
long long getCurrentTimestamp() {
    return chrono::duration_cast<chrono::milliseconds>(
        chrono::system_clock::now().time_since_epoch()
    ).count();
}
// LRU: Find the page with the oldest /smallest last_access_time
int findLRUVictim(vector<PageMapTableEntry>& pageMapTable) {
    long long min_time = LLONG_MAX;
    int victim_page_no = -1;

    for (auto& entry : pageMapTable) {
        if (entry.status && entry.last_access_time < min_time) {
            min_time = entry.last_access_time;
            victim_page_no = entry.page_no;
        }
    }

    return victim_page_no;
}



void processJobs(vector<Job>& jobs, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables){
    //processing jobs sequentially by spinning up threads

    cout<<"There are "<<jobs.size()<<" jobs"<<endl;
    ////////////
    vector<thread> threads;
    for(auto& job: jobs){
        threads.push_back(
            thread(
                processIndividualJob, ref(job), ref(pageFrames), ref(memoryMapTable),
        ref(jobTable), ref(pageMapTables)));
    }

    for(auto& t: threads){
        t.join();
    }
    //////////
}


void processIndividualJob(Job& job, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables){
    /*
    use a random number generator to pick which job we want to process
    move that job into a page frame if one is available 
    update all tables in the process

    If we find that there is no space then we call the deallocation algorithm that makes use of FIFO or LIFO
    */

    cout<<"Processing job "<<job.number<<endl;

    for(int i = 0; i< job.pages.size(); ++i){
        // Protect each full page request
        lock_guard<mutex> lock(mtx);

        vector<PageMapTableEntry>& pageMapTable =  pageMapTables[jobTable[job.number].PMT_ID];
        int randomJobIndex = (rand()%job.pages.size());

        Page requestedPage = job.pages[randomJobIndex];
        cout<<"Page "<<requestedPage.page_no<<" has been requested";

        //check if the requested page is already loaded
        PageMapTableEntry* row = getPageMapTableEntryByPageNumber(requestedPage.page_no, pageMapTable);
        if(!row){
            cout<<"Invalid row"<<endl;
            return;
        }
        if(row->status){
            ////////////page hit

            cout<<" -- HIT (already loaded)"<<endl;
            ////////// 
            page_hits++;

            //we have been loaded to memory
            row->referenced = true;
            /////
            row->last_access_time = getCurrentTimestamp(); // Update LRU timestamp
            
            //randomly decide if we want to modify this 
            bool modify = (rand()%2) == 1;
            row->modified = modify;

            continue;
        }
        ////////PAGE FAULT---Page not in memory
        cout << " -- MISS (page fault)" << endl;
        page_faults++;

        //if we are here then the page has not been loaded into memory
        //we first need to check if there is space
        bool foundSpace = false;
        int free_frame_no = -1;
        
        for(auto& pageFrame : memoryMapTable){
            if(!pageFrame.is_occupied){
                pageFrame.is_occupied = true;
                foundSpace = true;
                free_frame_no = pageFrame.page_frame_no;
                break;
            }
        }

        if(!foundSpace){
            /////////
            // No free frames - Apply LRU page replacement
            cout << "    No free frames. Applying LRU replacement..." << endl;
            page_replacements++;

            int victim_page_no = findLRUVictim(pageMapTable);

            if (victim_page_no == -1) {
                cout << "    ERROR: Could not find LRU victim!" << endl;
                continue;
            }
            PageMapTableEntry* victim = getPageMapTableEntryByPageNumber(victim_page_no, pageMapTable);
            cout << "    Evicting Page " << victim_page_no << " from frame " << victim->page_frame_no;

            if (victim->modified) {
                cout << " (modified - writing back to disk)";
            }
            cout << endl;

            ///// Free the frame used by victim
            free_frame_no = victim->page_frame_no;

            ///// Update victim's PMT entry
            victim->status = false;
            victim->page_frame_no = -1;
            victim->referenced = false;
            victim->modified = false;

            //cout<<"Deallocation with LRU or FIFO happens here oo"<<endl;
            //return;

            memoryMapTable[free_frame_no].page_no = requestedPage.page_no;
        }

        // Load the requested page into the free frame
        cout << "    Loading Page " << requestedPage.page_no << " into frame " << free_frame_no << endl;

        //updated the memory map table
        //now to update the pageMapTable
        row->status = true;
        row->page_frame_no = free_frame_no;
        row->referenced = true;
        row->last_access_time = getCurrentTimestamp();
        row->load_time = getCurrentTimestamp();
        
        pageFrames[free_frame_no].page_no = requestedPage.page_no;
        pageFrames[free_frame_no].size_of_content = requestedPage.size_of_content;
        memoryMapTable[free_frame_no].page_no = requestedPage.page_no;
        memoryMapTable[free_frame_no].is_occupied = true;
    }
}

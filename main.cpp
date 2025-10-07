#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <functional>
using namespace std;

int PAGE_SIZE = 200; //Page size and page frame size
int TOTAL_MEMORY = 20000; //total memory available

mutex mtx;

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
};

struct PageMapTableEntry {
    int page_no, page_frame_no = -1;
    bool modified = false, referenced = false, status = false;
    //add a timestamp here so we can know when last something was used 
    //add a timestamp here so we can know when something first got into memory
    //the timestamps will make it easier to implement FIFO and LRU
};

struct MemoryMapTableEntry {
    int page_frame_no;
    bool is_occupied;
};

struct JobTableEntry {
    int job_no, PMT_ID;
};

void acceptJobs(int n, vector<Job>& jobs);
void moveJobsToPages(vector<Job>& jobs, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
void processJobs(vector<Job>& jobs, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
void processIndividualJob(Job& job, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables);
PageMapTableEntry getPageMapTableEntryByPageNumber(int page_no, vector<PageMapTableEntry>& pageMapTable);

int main(){
    vector<Job> jobs;
    int n;

    cout<<"Enter the number of jobs: "<<endl;
    cin>> n;

    acceptJobs(n, jobs);

    int num_page_frames = ceil(TOTAL_MEMORY / PAGE_SIZE);
    vector<PageFrame> pageFrames;

    vector<JobTableEntry> jobTable(n);
    vector<MemoryMapTableEntry> memoryMapTable;
    
    vector<vector<PageMapTableEntry>> pageMapTables;
    //a vector that holds all the page map tables

    //creating pageFrames!!!
    for(int i = 0; i< num_page_frames; ++i){
        PageFrame pageFrame;
        pageFrame.size_of_content = 0;
        pageFrame.page_frame_no = i;

        pageFrames.push_back(pageFrame);

        MemoryMapTableEntry memoryMapTableEntry;
        memoryMapTableEntry.page_frame_no = i;
        memoryMapTableEntry.is_occupied = false;
        memoryMapTable.push_back(memoryMapTableEntry);
    }

    moveJobsToPages(jobs, jobTable, pageMapTables);
    processJobs(jobs, pageFrames, memoryMapTable, jobTable, pageMapTables);



    return 0;
}

void acceptJobs(int n, vector<Job>& jobs){
    for(int i = 0; i<n; ++i){
        int size;
        Job job;
        cout<<"Enter the size of job "<<i++<<endl;
        cin>>size;
        job.number = i;
        job.size = size;

        jobs.push_back(job);
    }
}

void moveJobsToPages(vector<Job>& jobs, vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables){
    int pageNo = 0;
    for(int i = 0; i< jobs.size(); ++i){
        auto& job = jobs[i];
        
        JobTableEntry jobTableEntry;
        jobTableEntry.job_no = job.number;
        jobTableEntry.PMT_ID = i;
        vector<PageMapTableEntry> pageMapTable;        

        int num_pages = ceil(job.size/PAGE_SIZE);
        for(int i = 0; i< num_pages; ++i){
            PageMapTableEntry PMT_Entry;

            Page newPage;
            newPage.job_number = job.number;
            newPage.page_no = pageNo;
            newPage.size_of_content = (i == num_pages - 1) ? job.size % PAGE_SIZE : PAGE_SIZE;
            //if we are on the last page for the job, then the size of 
            job.pages.push_back(newPage);

            PMT_Entry.page_no = pageNo;
            pageMapTable.push_back(PMT_Entry);

            pageNo++;
        }
        pageMapTables.push_back(pageMapTable);
    }
}

PageMapTableEntry getPageMapTableEntryByPageNumber(int page_no, vector<PageMapTableEntry>& pageMapTable){
    for(auto& row: pageMapTable){
        if(row.page_no == page_no) return row;
    }
    PageMapTableEntry invalid;
    invalid.page_no = -1;
    invalid.page_frame_no = -1;
    invalid.modified = false;
    invalid.referenced = false;
    invalid.status = false;

    cout<<"Error!"<<endl;
    return invalid;
}

void processJobs(vector<Job>& jobs, vector<PageFrame>& pageFrames, vector<MemoryMapTableEntry>& memoryMapTable,
vector<JobTableEntry>& jobTable, vector<vector<PageMapTableEntry>>& pageMapTables){
    //processing jobs sequentially by spinning up threads

    cout<<"There are "<<jobs.size()<<" jobs"<<endl;
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

   lock_guard<mutex> lock(mtx);

   srand(time(0));
   for(int i = 0; i< job.pages.size(); ++i){
        vector<PageMapTableEntry>& pageMapTable =  pageMapTables[jobTable[job.number].PMT_ID];
        int randomJobIndex = (rand()%job.pages.size());

        Page requestedPage = job.pages[randomJobIndex];
        cout<<"Page "<<requestedPage.page_no<<" has been requested";

        //check if the requested page is already loaded
        PageMapTableEntry row = getPageMapTableEntryByPageNumber(requestedPage.page_no, pageMapTable);
        if(row.page_no == -1){
            cout<<"Invalid row"<<endl;
            return;
        }
        if(row.status){

            cout<<"Page "<<requestedPage.page_no<<" has been loaded to memoryh already"<<endl;
            //we have been loaded to memory
            row.referenced = true;
            
            //randomly decide if we want to modify this 
            bool modify = (rand()%2) == 1;
            row.modified = modify;

            return;
        }
        //if we are here then the page has not been loaded into memory
        //we first need to check if there is space
        bool foundSpace = false;
        int free_frame_no = -1;
        
        for(auto& pageFrame : memoryMapTable){
            if(!pageFrame.is_occupied){
                pageFrame.is_occupied = true;
                foundSpace = true;
                free_frame_no = pageFrame.page_frame_no;
            }
        }

        if(!foundSpace){
            cout<<"Deallocation with LRU or FIFO happens here oo"<<endl;
            return;
        }

        //updated the memory map table
        //now to update the pageMapTable
        row.status = true;
        row.page_frame_no = free_frame_no;
        

        
   }
   


}

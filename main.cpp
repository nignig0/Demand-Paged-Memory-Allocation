#include <iostream>
#include <vector>
#include <cmath>
using namespace std;

int PAGE_SIZE = 200; //Page size and page frame size
int TOTAL_MEMORY = 20000; //total memory available

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

struct PageMapTable {
    int id;
    int page_no, page_frame_no;
    bool modified, referenced, status;
};

struct MemoryMapTable {
    int page_frame_no;
    bool is_occupied;
};

struct JobTable {
    int job_no, PMT_ID;
};

void acceptJobs(int n, vector<Job>& jobs);
void moveJobsToPages(vector<Job>& jobs);
void processJobs(vector<Job>& jobs);
void processIndividualJob(Job& job);

int main(){
    vector<Job> jobs;
    int n;

    cout<<"Enter the number of jobs: "<<endl;
    cin>> n;

    acceptJobs(n, jobs);

    int num_page_frames = ceil(TOTAL_MEMORY / PAGE_SIZE);
    vector<PageFrame> pageFrames;

    vector<JobTable> jobTable(n);
    vector<MemoryMapTable> memoryMapTable;
    
    //creating pageFrames!!!
    for(int i = 0; i< num_page_frames; ++i){
        PageFrame pageFrame;
        pageFrame.size_of_content = 0;
        pageFrame.page_frame_no = i;

        pageFrames.push_back(pageFrame);

        MemoryMapTable memoryMapTableEntry;
        memoryMapTableEntry.page_frame_no = i;
        memoryMapTableEntry.is_occupied = false;
    }

    //moveJobsToPages(jobs);



    return 0;
}

void acceptJobs(int n, vector<Job>& jobs){
    for(int i = 0; i<n; ++i){
        int size;
        Job job;
        cout<<"Enter the size of job "<<i++<<endl;
        cin>>size;
        job.number = i+1;
        job.size = size;

        jobs.push_back(job);
    }
}

void moveJobsToPages(vector<Job>& jobs){
    int pageNo = 0;
    for(auto& job : jobs){
        int num_pages = ceil(job.size/PAGE_SIZE);
        for(int i = 0; i< num_pages; ++i){
            Page newPage;
            newPage.job_number = job.number;
            newPage.page_no = pageNo;
            newPage.size_of_content = (i == num_pages - 1) ? job.size % PAGE_SIZE : PAGE_SIZE;
            //if we are on the last page for the job, then the size of 
            job.pages.push_back(newPage);
            pageNo++;
        }

    }
}


void processJobs(vector<Job>& jobs){
    //processing jobs sequentially by spinning up threads
}

void processIndividualJob(Job& job){
    /*
    use a random number generator to pick which job we want to process
    move that job into a page frame if one is available 
    update all tables in the process
    sleep for 2 seconds (arbitrary time) 
    remove the job
    update the tables
    */
}

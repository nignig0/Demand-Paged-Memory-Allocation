#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <ctime>

using namespace std;

// Struct to represent a a Job
struct Job {
    int jobID;
    int jobSize;
    vector<int> pageNumbers;
    vector<int> pageFrameNumbers;
};

// Struct for page table entry
struct PageTableEntry {
    int pageNumber;
    int pageFrameNumber;
};

// Struct for page frame
struct PageFrame {
    int frameNumber;
    bool isFree;
    int jobID;
    int pageNumber;
};

class PagedMemoryManager {
private:
    int PAGE_SIZE;
    int MEMORY_SIZE;
    int NUM_PAGE_FRAMES;
    vector<PageFrame> pageFrames;
    Job currentJob;

public:
    PagedMemoryManager(int pageSize, int memorySize) {
        PAGE_SIZE = pageSize;
        MEMORY_SIZE = memorySize;
        NUM_PAGE_FRAMES = MEMORY_SIZE / PAGE_SIZE;

        // Initialize page frames
        for (int i =0; i < NUM_PAGE_FRAMES; i++) {
            PageFrame pf;
            pf.frameNumber = i;
            pf.isFree = true;
            pf.jobID = -1;
            pf.pageNumber = -1;
            pageFrames.push_back(pf);
        }

        srand(time(0));
    }

    void acceptJob() {
        cout << "\nACCEPT JOB" << endl;
        cout << "Enter Job ID: ";
        cin >> currentJob.jobID;
        cout << "Enter Job Size (in bytes): ";
        cin >> currentJob.jobSize;

        currentJob.pageNumbers.clear();
        currentJob.pageFrameNumbers.clear();
    }

    void divideIntoPages() {
        cout << "\nDIVIDE JOB INTO PAGES" << endl;

        // Calculate number of pages
        int numPages = (currentJob.jobSize + PAGE_SIZE - 1) / PAGE_SIZE;

        cout << "Job Size: " << currentJob.jobSize << " bytes" << endl;
        cout << "Page Size: " << PAGE_SIZE << " bytes" << endl;
        cout << "Number of pages required: " << numPages << endl;

        // Calculate internal fragmentation
        int lastPage = currentJob.jobSize % PAGE_SIZE;
        int internalFragmentation = 0;

        if (lastPage != 0) {
            internalFragmentation = PAGE_SIZE - lastPage;
        }

        for (int i = 0; i < numPages; i++) {
            currentJob.pageNumbers.push_back(i);

            int bytesInPage = PAGE_SIZE;
            if (i == numPages -1 && lastPage != 0) {
                bytesInPage = lastPage;
            }

            cout << "Page " << i << ": " << bytesInPage << " bytes used";

            if (i == numPages - 1 && internalFragmentation > 0) {
                cout << " (" << internalFragmentation << " bytes wasted - Internal Fragmentation)";
            }
            cout << endl;
        }
        if (internalFragmentation > 0) {
            cout << "\n** Internal Fragmentation: " << internalFragmentation << " bytes **" << endl;
        } else {
            cout << "\n** No Internal Fragmentation **" << endl;
        }
    }

    bool loadIntoPageFrames() {
        cout << "\nLOAD PAGES INTO PAGE FRAMES" << endl;

        // Check if enough free frames are available
        int freeFrames = 0;
        for (int i = 0; i < NUM_PAGE_FRAMES; i++) {
            if (pageFrames[i].isFree) {
                freeFrames++;
            }
        }

        if (freeFrames < currentJob.pageNumbers.size()) {
            cout << "Not enough free page frames available to load the job." << endl;
            cout << "Required: " << currentJob.pageNumbers.size() << ", Available: " << freeFrames << endl;
            return false;
        }

        // Load pages into random free frames
        cout << "Loading Job " << currentJob.jobID << " into memory..." << endl;
        cout << "\n--- Page Map Table (PMT) for Job " << currentJob.jobID << " ---" << endl;
        cout << setw(15) << "Page Number" << setw(20) << "Page Frame Number" << endl;
        cout << string(35, '-') << endl;
        
        for (int pageNum : currentJob.pageNumbers) {
            int frameIndex;
            do {
                frameIndex = rand() % NUM_PAGE_FRAMES;
            } while (!pageFrames[frameIndex].isFree);

            // Assign page to frame
            pageFrames[frameIndex].isFree = false;
            pageFrames[frameIndex].jobID = currentJob.jobID;
            pageFrames[frameIndex].pageNumber = pageNum;

            currentJob.pageFrameNumbers.push_back(frameIndex);

            cout << setw(15) << pageNum << setw(20) << frameIndex << endl;
        }
        return true;
    }

    void performAddressResolution() {
        cout << "\nADDRESS RESOLUTION" << endl;

        int byteLocation;
        cout << "Enter byte location to access (0 to " << currentJob.jobSize - 1 << "): ";
        cin >> byteLocation;

        if (byteLocation < 0 || byteLocation >= currentJob.jobSize) {
            cout << "Invalid byte location." << endl;
            return;
        }

        // Calculate page number and offset
        int pageNumber = byteLocation / PAGE_SIZE;
        int offset = byteLocation % PAGE_SIZE;

        cout << "Byte Location: " << byteLocation << endl;
        cout << "Page Number: " << pageNumber << endl;
        cout << "Offset within Page: " << offset << endl;

        // Find page frame from PMT
        int pageFrameNumber = currentJob.pageFrameNumbers[pageNumber];

        cout << "Page " << pageNumber << " is in Frame " << pageFrameNumber << endl;

        // Calculate starting address of page frame
        int pageFrameAddress = pageFrameNumber * PAGE_SIZE;

        cout << "Starting Address of Frame " << pageFrameNumber << ": " << pageFrameAddress << endl;

        // Calculate physical address
        int physicalAddress = pageFrameAddress + offset;
        cout << "Physical Address: " << physicalAddress << endl;

        cout << "\nByte " << byteLocation << " of Job " << currentJob.jobID << " is located at Physical Address " << physicalAddress << endl;

    }

    void displayMemoryStatus(){
        cout << "\nMEMORY STATUS" << endl;
        cout << "Total Memory Size: " << MEMORY_SIZE << " bytes" << endl;
        cout << "Page Frame Size: " << PAGE_SIZE << " bytes" << endl;
        cout << "Number of Page Frames: " << NUM_PAGE_FRAMES << endl;

        int usedFrames = 0;
        for (int i = 0; i < NUM_PAGE_FRAMES; i++) {
            if (!pageFrames[i].isFree) {
                usedFrames++;
            }
        }

        cout << "Used Page Frames: " << usedFrames << endl;
        cout << "Free Page Frames: " << (NUM_PAGE_FRAMES - usedFrames) << endl;

        cout << "\nPage Frame Table:" << endl;
        cout << setw(15) << "Frame Number" << setw(15) << "Status" << setw(12) << "Job ID" << setw(15) << "Page Number" << endl;
        cout << string(57, '-') << endl;
        
        for (int i = 0; i < NUM_PAGE_FRAMES; i++) {
            cout << setw(15) << pageFrames[i].frameNumber;
            cout << setw(15) << (pageFrames[i].isFree ? "Free" : "Occupied");
            
            if (pageFrames[i].isFree) {
                cout << setw(12) << "-" << setw(15) << "-";
            } else {
                cout << setw(12) << pageFrames[i].jobID << setw(15) << pageFrames[i].pageNumber;
            }
            cout << endl;
        }
    }

    void run() {
        cout << "\nPAGED MEMORY MANAGEMENT SIMULATION\n" << endl;

        int pageSize, memorySize;
        cout << "\nEnter Page Size (in bytes): ";
        cin >> pageSize;
        cout << "Enter Total Memory Size (in bytes): ";
        cin >> memorySize;

        PAGE_SIZE = pageSize;
        MEMORY_SIZE = memorySize;
        NUM_PAGE_FRAMES = MEMORY_SIZE / PAGE_SIZE;

        pageFrames.clear();
        for (int i =0; i < NUM_PAGE_FRAMES; i++) {
            PageFrame pf;
            pf.frameNumber = i;
            pf.isFree = true;
            pf.jobID = -1;
            pf.pageNumber = -1;
            pageFrames.push_back(pf);
        }

        cout << "\nMemory initialized with " << NUM_PAGE_FRAMES << " page frames of " << PAGE_SIZE << " bytes each." << endl;

        char continueSimulation = 'y';
        while (continueSimulation == 'y' || continueSimulation == 'Y') {
            acceptJob();
            divideIntoPages();
            if (loadIntoPageFrames()) {
                performAddressResolution();
            }
            displayMemoryStatus();

            cout << "\nDo you want to simulate another job? (y/n): ";
            cin >> continueSimulation;
        }
        cout << "\nExiting Paged Memory Management Simulation." << endl;
    }

};

int main() {
    PagedMemoryManager pmm(100, 1000);
    pmm.run();
    return 0;
    }
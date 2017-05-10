// Password Cracker: Something Awesome
// Dominic He z5024963

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <pthread.h>
#include <mutex>
#include <iomanip>

#include "sha1.h"

#define DUMP_SIZE 6458020
#define NUM_THREADS 4
typedef unsigned char byte;
using namespace std;

struct container {
    // Last 17 bytes of sha1
    byte sha1[17];
    // Comparison of sha1, using for sorting and searching
    bool operator<(const container &y) const {
        int i = 0;
        while (i < 17 && this->sha1[i] == y.sha1[i]) i++;
        return this->sha1[i] < y.sha1[i];
    }
    // Comparison of sha1
    bool operator==(const container &y) const {
        int i = 0;
        while (i < 17) {
            if (this->sha1[i] != y.sha1[i]) return false;
            i++;
        }
        return true;
    }
};

// bool cmp(const container &a, const container &b) {
//     int i = 0;
//     while (i < 17 && a.sha1[i] == b.sha1[i]) i++;
//     return a.sha1[i] < b.sha1[i];
// }

// Pointer to vector in heap containing dump
vector<container> *dump;
// Mutex lock to cout/stdout
std::mutex cout_mutex;
vector<string> words;

void hexToBin(string &hex, byte *dest);
void searchHash(string &str, SHA1Context *context);
void printSHA1(container &c);
void *crack_thread(void *t); // thread function

int main(int argc, char *argv[]) {
    // Initialise dump
    std::ifstream dumpFile;
    dumpFile.open(argv[1]);
    dump = new vector<container>(DUMP_SIZE);
    for (int i = 0; i < DUMP_SIZE; i++) {
        string sha1hex;
        dumpFile >> sha1hex;
        hexToBin(sha1hex, (*dump)[i].sha1);
        if ((*dump)[i].sha1[0] == 228 && (*dump)[i].sha1[16] == 216) {
            // printSHA1((*dump)[i]);
        }
    }
    dumpFile.close();
    cout << "Dump loaded.\n";
    sort(dump->begin(), dump->end());
    cout << "Dump sorted.\nLoading dictionary...";
    // Loads words from dict into queues for each thread;
    vector<string> threadQueues[NUM_THREADS];

    ifstream dictFile;
    dictFile.open(argv[2]);
    words.reserve(600000);

    string word, wordNum;
    SHA1Context con;
    int threadNum = 0;
    while (!dictFile.eof()) {
        dictFile >> word;
        threadQueues[threadNum].push_back(word);
        words.push_back(word);
        threadNum = (threadNum + 1) % NUM_THREADS;
    }
    dictFile.close();
    
    ///////////////////////////////
    /// START MULTITHREADING
    ///////////////////////////////
    // Start threads
    int sig;
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        cout << "Starting thread " << i << endl;
        
        sig = pthread_create(&threads[i], NULL, crack_thread, (void*) &threadQueues[i]);
        if (sig) { cout << "Error creating thread\n"; }
    }

    std::string twoWord;
    // Try combinations of two words
    // for (std::string &w1 : words) {
    //     for (std::string &w2 : words) {
    //         twoWord = w1 + w2;
    //         searchHash(twoWord, &con);
    //     }
    // }

    // Wait for threads
    void *status;
    for (int i = 0; i < NUM_THREADS; i++) {
        sig = pthread_join(threads[i], &status);
    }

    delete dump;
    return 0;
}

// Convert 40 char hex of sha1 into byte array
void hexToBin(string &hex, byte *dest) {
    int mapping[128];
    mapping['0'] = 0;
    mapping['1'] = 1;
    mapping['2'] = 2;
    mapping['3'] = 3;
    mapping['4'] = 4;
    mapping['5'] = 5;
    mapping['6'] = 6;
    mapping['7'] = 7;
    mapping['8'] = 8;
    mapping['9'] = 9;
    mapping['a'] = 10;
    mapping['b'] = 11;
    mapping['c'] = 12;
    mapping['d'] = 13;
    mapping['e'] = 14;
    mapping['f'] = 15;
    for (int i = 0; i < 17; i++) {
        dest[i] = mapping[hex[6+i*2]] * 16 + mapping[hex[6+i*2+1]];
    }
}

// finds a hash of a string in the dump
void searchHash(string &str, SHA1Context *context) {
    SHA1Reset(context);
    SHA1Input(context, (unsigned char *) str.c_str(), str.length());
    unsigned char hash[20];
    SHA1Result(context, hash);
    // Copy last 17 bytes to container
    container hashContainer;
    for (int i = 0; i < 17; i++) {
        hashContainer.sha1[i] = hash[i+3];
    }
    // cout << (int)hashContainer.sha1[0] << " " << (int)hashContainer.sha1[16] << endl;
    auto finder = std::lower_bound(dump->begin(), dump->end(), hashContainer);
    if (*finder == hashContainer) {
        // hash found in dump: print to stdout
        cout_mutex.lock();
        std::cout << "Found: ";
        printSHA1(*finder);
        std::cout << " \t" << str << std::endl;
        cout_mutex.unlock();
    }
}

// Print out 17 bytes of hash
void printSHA1(container &c) {
    std::cout << std::hex << (int) c.sha1[0];
    for (int i = 1; i < 17; i++) {
        std::cout << setfill('0') << setw(2) << std::hex << (int) c.sha1[i];
    }
}

// Thread that takes list of words and tries cracking
void *crack_thread(void *t) {
    SHA1Context con;
    vector<string> *threadWords = (vector<string> *) t;
    string wordNum, twoWords;
    
    for (string &word : *threadWords) {
        if (word.length() >= 6) searchHash(word, &con);
        // Concatenate up to 4 digit numbers
        for (int i = 0; i <= 9999; i++) {
            wordNum = word + to_string(i);
            searchHash(wordNum, &con);
        }
        // Two words
        // for (string &word2 : words) {
        //     twoWords = word + word2;
        //     searchHash(word, &con);
        // }
    }
    pthread_exit(NULL);
}
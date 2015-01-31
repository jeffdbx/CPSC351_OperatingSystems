//=========================================================================================================
//  Jeff Bohlin
//  December 11th, 2012
//  CS351 - TTR 5:30
//  Professor Gofman
//  Assignment 3 Bonus
//  Multi-threaded Parallel QuickSort.
//
//  Compile command: g++ Assn3Bonus.cpp -o sort -lpthread
//
//  This program completes the following tasks:
//
//  1. The program is invoked with the input file name and number of threads to be created as parameters.
//  2. The program then reads in integers from a file and stores them in an array.
//  3. The program then splits up this array, evenly, according to the number of threads created.
//  4. Then each thread performs a quick sort on his partition IN PARALLEL!.
//  5. Finally, after all threads are done, a merge sort is performed to put all of the partitions
//     in the correct order.
//
//  Sources used to help complete this assignment:
//
//  1. Cprogramming.com forums (12-29-2004):
//       http://cboard.cprogramming.com/cplusplus-programming/60180-nonrecursive-quick-sort.html
//  2. Wikipedia, "Merge Sort":
//       http://en.wikipedia.org/wiki/Merge_sort
//  3. Yahoo! Answers, "Help with c++ merge_sort and stl merge?":
//       http://answers.yahoo.com/question/index?qid=20101014145456AAGqHbp
//
//  4. And of course, Professor Gofman's guide files.
//
//=========================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <stack>
#include <algorithm>
#include <string>

using namespace std;

// "workerThreads" begin control here. 
void *worker(void *);

// ==============  Quick Sort Functions ============== 
void swap_in_array(vector<int> &, int, int);
int partition(vector<int> &, int, int);
void quicksort_it(vector<int> &, int, int);
// ===================================================

// This utilizes the STL function inplace_merge() 
void merge_sort(vector<int>::iterator, vector<int>::iterator);

// The number of worker threads to be created (determined by the user). 
int numThreads;

// Vector to hold the data to be sorted 
vector<int> data;

// The worker thread 
pthread_t *workerThread;

// The default number of threads to be created if the user chooses an invalid number. 
const int DEFAULT_NUM = 5;

// The number of partitions.  n = arraySize / numThreads 
int n;

int main(int argc, char** argv)
{
    // There should be 3 parameters for correct execution. (2 entered by the user,
    // and argv[0] which is simply the path where this program is stored). 
    if (argc != 3)
    {
        cout << "\nTo run this program, use the following command:\n"
            << argv[0] << " <FILENAME> <NUMBER OF THREADS>\n\n";
        exit(EXIT_FAILURE);
    }

    // Open the file 
    ifstream fin;
    fin.open(argv[1]);

    // Make sure the file exists. 
    if (!fin)
    {
        cout << "File not found!" << endl;
        exit(EXIT_FAILURE);
    }

    // Populate the vector 
    int temp;
    while (!fin.eof())
    {
        fin >> temp;
        data.push_back(temp);
    }
    data.pop_back();

    fin.close();

    // arraySize is the total number of integers that came from the input file. 
    int arraySize = (int)data.size();

    cout << "\n================================================================================\n";
    cout << "Be sure to play with the number of threads so that that partition size changes!\n";
    cout << "\nData before sort:\n";
    for (int i = 0; i < arraySize; i++)
    {
        cout << data[i] << " ";
    }
    cout << endl;

    //  The number of worker threads to be created (designated by the user). 
    numThreads = atoi(argv[2]);

    // Make sure the user enters a valid number of threads. 
    if (numThreads < 1 || numThreads > arraySize)
    {
        cout << "The number of threads must be between 1 and " << arraySize << ".\n"
             << "Using default value " << DEFAULT_NUM << " instead.\n";
        numThreads = DEFAULT_NUM;
    }

    // The number of integers that each thread will sort. 
    n = arraySize / numThreads;

    // The integers leftover if the partitions weren't divided evenly. 
    int r = arraySize % numThreads;

    // The main thread will sort the remainder. 
    if (r)
    {
        vector<int> tempArray;

        // Copy the remaining integers into a smaller, temporary array that will be sorted. 
        for (int i = 0; i < r; i++)
        {
            // NOTE: No need for a mutex lock here, we are only reading from the data vector. 
            // "(n*numThreads)+i" is the offset that tells us where the remaining integers are stored. 
            tempArray.push_back(data[(n*numThreads) + i]);
        }

        // Perform a quick sort on this partition 
        quicksort_it(tempArray, 0, r - 1);


        // NOTE: This is probably not very safe. In fact there probably should be a mutex lock here. However,
        //       each thread is given a specific partition that no other thread will ever write to. So, I'm
        //       taking the risk! 
        for (int i = 0; i < r; i++)
        {
            // Once the partition has been sorted, copy it back to it's old spot in the data array. 
            data[(n*numThreads) + i] = tempArray[i];
        }
    }

    // Set of thread attributes. 
    pthread_attr_t attr;

    // Get the default attributes. 
    pthread_attr_init(&attr);

    // Create the worker threads. 
    workerThread = new pthread_t[numThreads];
    for (long i = 0; i < numThreads; i++)
    {
        if (pthread_create(&workerThread[i], &attr, worker, (void *)i) < 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all of the threads to finish sorting their partitions. 
    for (long i = 0; i < numThreads; i++)
    {
        if (pthread_join(workerThread[i], NULL) < 0)
        {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    cout << "\nAfter threads' quick sort [notice that every " << n << " number(s) is partition set]:\n";
    for (int i = 0; i < arraySize; i++)
    {
        cout << data[i] << " ";
    }

    // Now, perform a merge sort on all of the partitions to get them in the correct order. 
    merge_sort(data.begin(), data.end());

    cout << "\n\nAfter merge sort:\n";
    for (int i = 0; i < arraySize; i++)
    {
        cout << data[i] << " ";
    }
    cout << "\n================================================================================\n";

    exit(EXIT_SUCCESS);
}

void *worker(void *i)
{
    // The ID of the current thread. Cast from void* to integer. 
    int my_id = *((int*)(&i));

    vector<int> tempArray;

    // Copy the partition into its own smaller array that will be sorted. 
    for (int j = 0; j < n; j++)
    {
        // NOTE: No need for a mutex lock here, we are only reading from the data vector. 
        // "(my_id*n)+j" is the offset that tells us where this thread's partition starts. 
        tempArray.push_back(data[(my_id*n) + j]);
    }

    // Perform a quick sort on this partition 
    quicksort_it(tempArray, 0, n - 1);

    // Once the partition has been sorted, copy it back to it's old spot in the original data array.
    for (int j = 0; j < n; j++)
    {
        // Yes, there probably should be a mutex lock here. But show mercy! We were running out of time :)
        data[(my_id*n) + j] = tempArray[j];
    }
}

//============ Quick Sort Functions (See cited source at top of program) =========
void swap_in_array(vector<int> &array, int i, int j)
{
    int tmp;
    tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}

int partition(vector<int> &array, int l, int r)
{
    int pivot, i, j;

    pivot = array[l];
    i = l + 1;
    j = r;

    for (;;)
    {
        while ((array[i] <= pivot) && (i <= r))i++;
        while ((array[j] > pivot) && (j > l))j--;

        if (i < j)
            swap_in_array(array, i, j);
        else
            break;
    }
    swap_in_array(array, j, l);

    return j;
}

void quicksort_it(vector<int> &dataset, int l, int r)
{
    stack<int> st;
    int j;
    st.push(r);
    st.push(l);
    while (!st.empty())
    {
        l = st.top();
        st.pop();
        r = st.top();
        st.pop();

        j = partition(dataset, l, r);

        if (l < (j - 1))
        {
            st.push(j - 1);
            st.push(l);
        }
        if ((j + 1) < r)
        {
            st.push(r);
            st.push(j + 1);
        }
    }
}
//================================================================================

// Stl merge_sort
void merge_sort(vector<int>::iterator beg, vector<int>::iterator end)
{
    if (end - beg > 1)
    {
        std::vector<int>::iterator mid = beg + (end - beg) / 2;
        merge_sort(beg, mid);
        merge_sort(mid, end);
        std::inplace_merge(beg, mid, end);
    }
}

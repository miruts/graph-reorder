#include "sort.h"

void merge (unsigned int* arr, unsigned int* key, unsigned int low, unsigned int high, unsigned int mid)
{
    unsigned int i = low;
    unsigned int j = mid+1;
    unsigned int k = 0;
    unsigned int* temp = new unsigned int [high-low+1];
    unsigned int* tempKey = new unsigned int [high-low+1];
    while(i<=mid && j<=high)
    {
        if (arr[i] >= arr[j])
        {
            tempKey[k] = key[i];
            temp[k++] = arr[i++];
        }
        else
        {
            tempKey[k] = key[j];
            temp[k++] = arr[j++];
        }
    }
    while(i<=mid)
    {
        tempKey[k] = key[i];
        temp[k++] = arr[i++];
    }
    while(j<=high)
    {
        tempKey[k] = key[j];
        temp[k++] = arr[j++];
    }
    for (i=low; i<=high; i++)
    {   
        key[i] = tempKey[i-low];
        arr[i] = temp[i-low];
    }
    delete[] temp;
    delete[] tempKey;
}

void mergeSort (unsigned int* arr, unsigned int* key, unsigned int low, unsigned int high)
{

    if (low >= high)
        return;
    if ((high - low) == 1)
    {
        if (arr[high] > arr[low])
        {
            unsigned int temp = arr[high];
            arr[high] = arr[low];
            arr[low] = temp;
            temp = key[high];
            key[high] = key[low];
            key[low] = temp;
        }
        return;
    }
    unsigned int mid = (low + high)/2;
    mergeSort(arr, key, low, mid);
    mergeSort(arr, key, mid + 1, high);
    merge(arr, key, low, high, mid);
    
} 

void mergeWOkey (unsigned int* arr, unsigned int low, unsigned int high, unsigned int mid)
{
    unsigned int i = low;
    unsigned int j = mid+1;
    unsigned int k = 0;
    unsigned int* temp = new unsigned int [high-low+1];
    while(i<=mid && j<=high)
    {
        if (arr[i] >= arr[j])
            temp[k++] = arr[i++];
        else
            temp[k++] = arr[j++];
    }
    while(i<=mid)
        temp[k++] = arr[i++];
    while(j<=high)
        temp[k++] = arr[j++];
    for (i=low; i<=high; i++)
        arr[i] = temp[i-low];
    delete[] temp;
}

void mergeSortWOkey (unsigned int* arr, unsigned int low, unsigned int high)
{

    if (low >= high)
        return;
    if ((high - low) == 1)
    {
        if (arr[high] > arr[low])
        {
            unsigned int temp = arr[high];
            arr[high] = arr[low];
            arr[low] = temp;
        }
        return;
    }
    unsigned int mid = (low + high)/2;
    mergeSortWOkey(arr, low, mid);
    mergeSortWOkey(arr, mid + 1, high);
    mergeWOkey(arr, low, high, mid);
    
} 

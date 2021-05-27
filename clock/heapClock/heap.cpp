#include <vector>
#include <iostream>

using namespace std;

void heap_adjust(vector<int> &arr, int len, int i)
{
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int minIndex = i;
    if(left < len && arr[left] < arr[minIndex])
    {
        minIndex = left;
    }    
    if(right < len && arr[right] < arr[minIndex])
    {
        minIndex = right;
    }      
    if (minIndex != i)
    {
        swap(arr[minIndex], arr[i]);
        heap_adjust(arr, len, minIndex);
    }
}

void heapSort(vector<int> &arr)
{
    //构建小根堆：从完全二叉树的第 n/2 个位置开始构建
    for (int i = arr.size() - 1; i >= 0; i--)
    {
        heap_adjust(arr, arr.size(), i);
    }

    //小根堆根为最小值，取出顶部，调整剩余的堆
    for (int i = arr.size()-1; i > 0; i--)
    {
        //交换根节点和最后一个节点
        swap(arr[0], arr[i]);
        //调整剩下的堆
        heap_adjust(arr, i, 0);
    }
}

int main(int argc, char const *argv[])
{
    vector<int> arr = {8, 1, 14, 3, 31, 5, 7, 10};

    heapSort(arr);

    for (int i = arr.size()-1; i >= 0; i--)
    {
        cout << arr[i] << " ";
    }

    cout << endl;
    return 0;
}

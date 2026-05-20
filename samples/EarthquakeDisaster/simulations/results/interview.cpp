#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <fstream>
#include <numeric>

using namespace std;

// Template function
template <typename T>
T add(T a, T b) {
    return a + b;
}

// Class example
class TestClass {
private:
    int value;

public:
    TestClass(int v) : value(v) {}

    void show() {
        cout << "Class value: " << value << endl;
    }
};

// Thread function
void threadTask() {
    cout << "Thread running successfully!" << endl;
}

int main() {

    cout << "===== C++ IDE TEST START =====" << endl;

    // Basic I/O
    cout << "Hello from C++!" << endl;

    // STL vector
    vector<int> nums = {5, 3, 8, 1, 9};

    cout << "Original vector: ";
    for (int n : nums)
        cout << n << " ";
    cout << endl;

    // Sorting
    sort(nums.begin(), nums.end());

    cout << "Sorted vector: ";
    for (int n : nums)
        cout << n << " ";
    cout << endl;

    // STL algorithm
    int sum = accumulate(nums.begin(), nums.end(), 0);
    cout << "Sum of vector: " << sum << endl;

    // Template function
    cout << "Template add<int>: " << add<int>(3, 7) << endl;
    cout << "Template add<double>: " << add<double>(2.5, 3.1) << endl;

    // Class
    TestClass obj(42);
    obj.show();

    // Pointer
    int x = 10;
    int* ptr = &x;
    cout << "Pointer value: " << *ptr << endl;

    // File writing
    ofstream file("test_output.txt");
    file << "C++ IDE file writing test successful." << endl;
    file.close();

    cout << "File test_output.txt created." << endl;

    // Multithreading
    thread t(threadTask);
    t.join();

    cout << "===== C++ IDE TEST COMPLETE =====" << endl;

    return 0;
}
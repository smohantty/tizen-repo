#include "Span.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

using namespace utils;

void testBasicSpan() {
    std::cout << "=== Testing Basic Span ===" << std::endl;

    // Test with array
    int arr[] = {1, 2, 3, 4, 5};
    Span<int> span(arr);

    std::cout << "Span size: " << span.size() << std::endl;
    std::cout << "Span size in bytes: " << span.size_bytes() << std::endl;
    std::cout << "Span empty: " << span.empty() << std::endl;

    // Test element access
    std::cout << "First element: " << span.front() << std::endl;
    std::cout << "Last element: " << span.back() << std::endl;
    std::cout << "Element at index 2: " << span[2] << std::endl;

    // Test data pointer
    std::cout << "Data pointer: " << span.data() << std::endl;
}

void testSpanIteration() {
    std::cout << "\n=== Testing Span Iteration ===" << std::endl;

    int arr[] = {10, 20, 30, 40, 50};
    Span<int> span(arr);

    // Forward iteration
    std::cout << "Forward iteration: ";
    for (auto it = span.begin(); it != span.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Range-based for loop
    std::cout << "Range-based for: ";
    for (const auto& element : span) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    // Reverse iteration
    std::cout << "Reverse iteration: ";
    for (auto it = span.rbegin(); it != span.rend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Const iterators
    std::cout << "Const forward iteration: ";
    for (auto it = span.cbegin(); it != span.cend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

void testSpanConstruction() {
    std::cout << "\n=== Testing Span Construction ===" << std::endl;

    // Test with std::array
    std::array<int, 4> stdArr = {100, 200, 300, 400};
    Span<int> span1(stdArr);
    std::cout << "From std::array - Size: " << span1.size() << ", First: " << span1.front() << std::endl;

    // Test with const std::array
    const std::array<int, 3> constArr = {500, 600, 700};
    Span<const int> span2(constArr);
    std::cout << "From const std::array - Size: " << span2.size() << ", First: " << span2.front() << std::endl;

    // Test with std::vector
    std::vector<int> vec = {1000, 2000, 3000, 4000, 5000};
    Span<int> span3(vec);
    std::cout << "From std::vector - Size: " << span3.size() << ", First: " << span3.front() << std::endl;

    // Test with const std::vector
    const std::vector<int> constVec = {6000, 7000, 8000};
    Span<const int> span4(constVec);
    std::cout << "From const std::vector - Size: " << span4.size() << ", First: " << span4.front() << std::endl;

    // Test with pointer and size
    int* ptr = vec.data();
    Span<int> span5(ptr, 3);
    std::cout << "From pointer and size - Size: " << span5.size() << ", First: " << span5.front() << std::endl;
}

void testSubspan() {
    std::cout << "\n=== Testing Subspan ===" << std::endl;

    int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    Span<int> span(arr);

    // Test subspan with offset and count
    auto sub1 = span.subspan(2, 4);
    std::cout << "Subspan(2, 4): ";
    for (const auto& element : sub1) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    // Test subspan with offset only (to end)
    auto sub2 = span.subspan(5);
    std::cout << "Subspan(5): ";
    for (const auto& element : sub2) {
        std::cout << element << " ";
    }
    std::cout << std::endl;

    // Test nested subspans
    auto nested = span.subspan(1, 6).subspan(2, 3);
    std::cout << "Nested subspan: ";
    for (const auto& element : nested) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}

void testEmptySpan() {
    std::cout << "\n=== Testing Empty Span ===" << std::endl;

    Span<int> emptySpan;
    std::cout << "Empty span size: " << emptySpan.size() << std::endl;
    std::cout << "Empty span empty: " << emptySpan.empty() << std::endl;
    std::cout << "Empty span data: " << emptySpan.data() << std::endl;

    // Test iteration on empty span
    std::cout << "Empty span iteration: ";
    for (const auto& element : emptySpan) {
        std::cout << element << " ";
    }
    std::cout << "(no elements)" << std::endl;
}

void testSpanAlgorithms() {
    std::cout << "\n=== Testing Span with Algorithms ===" << std::endl;

    int arr[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    Span<int> span(arr);

    // Find element
    auto it = std::find(span.begin(), span.end(), 8);
    if (it != span.end()) {
        std::cout << "Found 8 at position: " << (it - span.begin()) << std::endl;
    }

    // Count elements
    auto count = std::count(span.begin(), span.end(), 5);
    std::cout << "Count of 5: " << count << std::endl;

    // Find maximum
    auto maxIt = std::max_element(span.begin(), span.end());
    std::cout << "Maximum element: " << *maxIt << std::endl;

    // Find minimum
    auto minIt = std::min_element(span.begin(), span.end());
    std::cout << "Minimum element: " << *minIt << std::endl;

    // Sum all elements
    int sum = 0;
    for (const auto& element : span) {
        sum += element;
    }
    std::cout << "Sum of all elements: " << sum << std::endl;
}

void testErrorHandling() {
    std::cout << "\n=== Testing Error Handling ===" << std::endl;

    int arr[] = {1, 2, 3};
    Span<int> span(arr);

    try {
        // Test out of bounds access
        std::cout << "Trying to access index 5..." << std::endl;
        int value = span[5];
        std::cout << "Value: " << value << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    try {
        // Test front() on empty span
        Span<int> emptySpan;
        std::cout << "Trying to access front of empty span..." << std::endl;
        int value = emptySpan.front();
        std::cout << "Value: " << value << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    try {
        // Test invalid subspan
        std::cout << "Trying invalid subspan..." << std::endl;
        auto invalid = span.subspan(10, 5);
        std::cout << "Invalid subspan created" << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
}

int main() {
    try {
        testBasicSpan();
        testSpanIteration();
        testSpanConstruction();
        testSubspan();
        testEmptySpan();
        testSpanAlgorithms();
        testErrorHandling();

        std::cout << "\n=== All Span tests completed successfully! ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// Suppress warnings from third-party libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc11-extensions"
#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wpedantic"

#include "FaceLandmarkTracker.h"
#include <opencv2/opencv.hpp>

#pragma GCC diagnostic pop

#include <iostream>
#include <string>
#include <chrono>

int main() {
    std::cout << "Face Landmark Tracker - Starting..." << std::endl;

    // Initialize the tracker
    facelandmark::FaceLandmarkTracker tracker;

    // Try to load the shape predictor model
    std::string shapePredictorPath = "shape_predictor_68_face_landmarks.dat";
    if (!tracker.initialize(shapePredictorPath)) {
        std::cerr << "Failed to initialize tracker. Please ensure shape_predictor_68_face_landmarks.dat is in the current directory." << std::endl;
        std::cerr << "You can download it from: http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << std::endl;
        return -1;
    }

    // Load existing database
    std::string databasePath = "user_db.bin";
    tracker.loadDatabase(databasePath);

    // Initialize camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera" << std::endl;
        return -1;
    }

    // Set camera properties
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 30);

    std::cout << "Camera initialized. Controls:" << std::endl;
    std::cout << "  'a' - Add new user" << std::endl;
    std::cout << "  'q' - Quit and save database" << std::endl;
    std::cout << "  's' - Save database" << std::endl;

    cv::Mat frame;
    bool running = true;

    // FPS monitoring
    auto startTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    double fps = 0.0;

    while (running) {
        // Capture frame
        auto captureStart = std::chrono::high_resolution_clock::now();
        cap >> frame;
        auto captureEnd = std::chrono::high_resolution_clock::now();
        auto captureTime = std::chrono::duration_cast<std::chrono::microseconds>(captureEnd - captureStart).count();

        if (frame.empty()) {
            std::cerr << "Failed to capture frame" << std::endl;
            break;
        }

        // Process frame for face detection and identification
        auto processStart = std::chrono::high_resolution_clock::now();
        tracker.processFrame(frame);
        auto processEnd = std::chrono::high_resolution_clock::now();
        auto processTime = std::chrono::duration_cast<std::chrono::microseconds>(processEnd - processStart).count();

        // Calculate and display FPS
        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
        if (elapsed.count() >= 1000) { // Update FPS every second
            fps = frameCount * 1000.0 / elapsed.count();
            frameCount = 0;
            startTime = currentTime;
        }

        // Display FPS and timing information
        std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
        cv::putText(frame, fpsText, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                   cv::Scalar(0, 255, 0), 2);

        std::string timingText = "Capture: " + std::to_string(captureTime/1000.0) + "ms, Process: " + std::to_string(processTime/1000.0) + "ms";
        cv::putText(frame, timingText, cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                   cv::Scalar(255, 255, 0), 1);

        cv::putText(frame, "Press 'a' to add user, 'q' to quit",
                   cv::Point(10, 90), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                   cv::Scalar(255, 255, 255), 2);

        // Show frame
        cv::imshow("Face Landmark Tracker", frame);

        // Handle keyboard input
        char key = cv::waitKey(1) & 0xFF;
        switch (key) {
            case 'a':
            case 'A': {
                // Add new user
                auto faces = tracker.detectFacesOpenCV(frame);
                if (faces.empty()) {
                    std::cout << "No face detected. Please position your face in front of the camera." << std::endl;
                } else {
                    // Use the first detected face
                    auto landmarks = tracker.extractLandmarks(frame, faces[0]);
                    if (!landmarks.empty()) {
                        std::cout << "Enter user name: ";
                        std::string userName;
                        std::getline(std::cin, userName);

                        if (!userName.empty()) {
                            tracker.addUser(userName, landmarks);
                            tracker.saveDatabase(databasePath);
                        } else {
                            std::cout << "Invalid user name. User not added." << std::endl;
                        }
                    }
                }
                break;
            }
            case 's':
            case 'S': {
                // Save database
                tracker.saveDatabase(databasePath);
                break;
            }
            case 'q':
            case 'Q':
            case 27: // ESC key
                running = false;
                break;
        }
    }

    // Save database before exiting
    tracker.saveDatabase(databasePath);

    // Cleanup
    cap.release();
    cv::destroyAllWindows();

    std::cout << "Face Landmark Tracker - Exiting..." << std::endl;
    return 0;
}

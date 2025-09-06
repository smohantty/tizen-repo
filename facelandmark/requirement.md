Face Landmark Tracking and User Identification – Specification Document
1. Overview

The application is a Linux C++17 desktop program that performs:

Face detection from a camera feed.

Landmark extraction for each detected face.

Normalization and storage of face landmarks against user names.

Serialization/deserialization of landmark database using a custom serde library.

Real-time user identification by comparing camera landmarks with stored landmarks.

Multi-face tracking for continuous monitoring.

2. Goals

Maintain a database of known users with normalized face landmarks.

Identify and label faces in real-time from the camera feed.

Store/retrieve the database efficiently using a header-only, macro-free serialization library (serde).

Support multiple users and multiple faces simultaneously.

Provide a user-friendly interface for adding new users and saving the database.

3. Technology Stack
Component	Technology / Library
Programming Language	C++17
Camera capture	OpenCV
Face detection	dlib (frontal_face_detector)
Landmark detection	dlib (shape_predictor_68_face_landmarks.dat)
Serialization / storage	Custom serde.h library
Build system	CMake or Makefile
OS	Linux
4. Data Structures
4.1 UserLandmark

Stores a single user’s normalized landmarks:

struct UserLandmark {
    std::string name;                    // User name
    std::vector<cv::Point2f> landmarks;  // Normalized 68-point landmarks

    void serialize(std::vector<uint8_t>& buf) const;   // Write to buffer
    void deserialize(const std::vector<uint8_t>& buf, size_t& offset); // Read from buffer
};

4.2 UserDatabase

Stores multiple users:

struct UserDatabase {
    std::vector<UserLandmark> users;

    void serialize(std::vector<uint8_t>& buf) const;
    void deserialize(const std::vector<uint8_t>& buf, size_t& offset);
};

5. Landmark Processing
5.1 Normalization

Translate landmarks so centroid of points is at origin.

Scale landmarks by interocular distance (distance between outer eye corners).

Optional: rotate to align eyes horizontally.

5.2 Comparison

Compute mean Euclidean distance between two sets of landmarks.

Threshold distance determines match.

6. Application Workflow
6.1 Startup

Open camera using OpenCV (cv::VideoCapture).

Load dlib face detector and shape predictor.

Load serialized user database if exists (user_db.bin).

6.2 Real-Time Loop

For each camera frame:

Detect faces using dlib detector.

Extract 68 landmarks for each face.

Normalize landmarks.

Compare with stored users in database:

Identify best match using mean Euclidean distance.

Label face with user name or "Unknown".

Draw bounding box and label on camera frame.

Display frame in OpenCV window.

6.3 Add New User

Press 'a' to add a detected face as a new user:

Prompt for user name.

Normalize landmarks.

Append to UserDatabase.

Serialize and save database to user_db.bin.

6.4 Exit

Press 'q' to quit.

Serialize and save database automatically.

7. File Management
File	Purpose
shape_predictor_68_face_landmarks.dat	dlib pretrained landmark model
user_db.bin	Serialized user landmarks database
serde.h	Header-only serialization library
Source files	main.cpp, serde.h
8. Functions / Modules
8.1 Landmark Processing Module

normalizeLandmarks(std::vector<cv::Point2f>) → std::vector<cv::Point2f>

landmarkDistance(a, b) → float

identifyUser(landmarks, db) → std::string

addUser(db, name, landmarks)

8.2 Serialization Module

UserLandmark::serialize() / deserialize()

UserDatabase::serialize() / deserialize()

serde::save_file(filename, buffer)

serde::load_file(filename)

8.3 Camera & GUI Module

Capture frame.

Detect faces.

Extract landmarks.

Draw bounding boxes and user labels.

Key bindings:

'a' → add user

'q' → quit

9. Thresholds and Tuning

Landmark distance threshold: 0.05 (may need adjustment based on resolution and lighting)

Camera resolution: minimum 640x480 for stable landmark detection

Frame rate: 15-30 FPS recommended for real-time tracking

10. Optional Enhancements

Support multiple landmark models (e.g., MediaPipe FaceMesh 468 points for higher precision).

Add tracking across frames using centroid or KCF/CSRT trackers to reduce detection frequency.

GUI for user management (add/delete users, view database).

Support saving landmarks for multiple expressions/angles per user for better identification.

11. Deliverables

main.cpp – main application source code

serde.h – header-only serialization library

CMakeLists.txt or Makefile

shape_predictor_68_face_landmarks.dat

Instructions for building and running

Example user_db.bin with one or two users

12. Build Instructions (Linux)
g++ -std=c++17 main.cpp -o face_tracker \
    `pkg-config --cflags --libs opencv4` \
    -ldlib
./face_tracker

13. User Interaction

On startup, existing database is loaded.

Display camera feed with boxes and labels.

Press 'a' → prompt for new user name → save landmarks.

Press 'q' → exit and save database.
// Suppress warnings from third-party libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc11-extensions"
#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wpedantic"

#include "FaceLandmarkTracker.h"

#pragma GCC diagnostic pop

#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace facelandmark {

FaceLandmarkTracker::FaceLandmarkTracker()
    : mInitialized(false), mDistanceThreshold(0.05f),
      mFrameSkipCounter(0), mFrameSkipInterval(8), mTrackingFrames(0) {
}

FaceLandmarkTracker::~FaceLandmarkTracker() = default;

bool FaceLandmarkTracker::initialize(const std::string& shapePredictorPath) {
    try {
        dlib::deserialize(shapePredictorPath) >> mShapePredictor;

        // Try to load OpenCV face detector for faster detection
        if (!mOpenCVFaceDetector.load(cv::samples::findFile("haarcascades/haarcascade_frontalface_alt.xml"))) {
            std::cout << "Warning: Could not load OpenCV face detector, using dlib only" << std::endl;
        } else {
            std::cout << "OpenCV face detector loaded successfully" << std::endl;
        }

        mInitialized = true;
        std::cout << "Face landmark tracker initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize face landmark tracker: " << e.what() << std::endl;
        mInitialized = false;
        return false;
    }
}

bool FaceLandmarkTracker::loadDatabase(const std::string& databasePath) {
    try {
        auto buffer = serde::load_file(databasePath);
        size_t offset = 0;
        mDatabase.deserialize(buffer, offset);
        std::cout << "Loaded database with " << mDatabase.users.size() << " users" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "No existing database found or failed to load: " << e.what() << std::endl;
        return false;
    }
}

bool FaceLandmarkTracker::saveDatabase(const std::string& databasePath) {
    try {
        auto buffer = serde::serialize(mDatabase);
        serde::save_file(databasePath, buffer);
        std::cout << "Saved database with " << mDatabase.users.size() << " users" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save database: " << e.what() << std::endl;
        return false;
    }
}

cv::Point2f FaceLandmarkTracker::calculateCentroid(const std::vector<cv::Point2f>& points) {
    cv::Point2f centroid(0, 0);
    for (const auto& point : points) {
        centroid += point;
    }
    centroid.x /= points.size();
    centroid.y /= points.size();
    return centroid;
}

float FaceLandmarkTracker::calculateInterocularDistance(const std::vector<cv::Point2f>& landmarks) {
    // Use outer eye corners (points 36 and 45 in 68-point model)
    if (landmarks.size() >= 68) {
        const auto& leftEye = landmarks[36];   // Left eye outer corner
        const auto& rightEye = landmarks[45];  // Right eye outer corner
        return std::sqrt(std::pow(rightEye.x - leftEye.x, 2) + std::pow(rightEye.y - leftEye.y, 2));
    }
    return 1.0f; // Default scale if landmarks are invalid
}

std::vector<cv::Point2f> FaceLandmarkTracker::normalizeLandmarks(const std::vector<cv::Point2f>& landmarks) {
    if (landmarks.empty()) return landmarks;

    // Calculate centroid
    cv::Point2f centroid = calculateCentroid(landmarks);

    // Translate to origin
    std::vector<cv::Point2f> normalized;
    for (const auto& landmark : landmarks) {
        normalized.emplace_back(landmark.x - centroid.x, landmark.y - centroid.y);
    }

    // Scale by interocular distance
    float interocularDist = calculateInterocularDistance(landmarks);
    if (interocularDist > 0) {
        for (auto& point : normalized) {
            point.x /= interocularDist;
            point.y /= interocularDist;
        }
    }

    return normalized;
}

float FaceLandmarkTracker::landmarkDistance(const std::vector<cv::Point2f>& landmarks1,
                                          const std::vector<cv::Point2f>& landmarks2) {
    if (landmarks1.size() != landmarks2.size() || landmarks1.empty()) {
        return std::numeric_limits<float>::max();
    }

    float totalDistance = 0.0f;
    for (size_t i = 0; i < landmarks1.size(); ++i) {
        float dx = landmarks1[i].x - landmarks2[i].x;
        float dy = landmarks1[i].y - landmarks2[i].y;
        totalDistance += std::sqrt(dx * dx + dy * dy);
    }

    return totalDistance / landmarks1.size();
}

std::string FaceLandmarkTracker::identifyUser(const std::vector<cv::Point2f>& landmarks) {
    if (!mInitialized || mDatabase.users.empty()) {
        return "Unknown";
    }

    auto normalizedLandmarks = normalizeLandmarks(landmarks);
    float minDistance = std::numeric_limits<float>::max();
    std::string bestMatch = "Unknown";

    for (const auto& user : mDatabase.users) {
        auto userLandmarks = user.getLandmarks();
        float distance = landmarkDistance(normalizedLandmarks, userLandmarks);
        if (distance < minDistance && distance < mDistanceThreshold) {
            minDistance = distance;
            bestMatch = user.name;
        }
    }

    return bestMatch;
}

void FaceLandmarkTracker::addUser(const std::string& name, const std::vector<cv::Point2f>& landmarks) {
    UserLandmark newUser;
    newUser.name = name;
    auto normalizedLandmarks = normalizeLandmarks(landmarks);
    newUser.setLandmarks(normalizedLandmarks);
    mDatabase.users.push_back(newUser);
    std::cout << "Added user: " << name << std::endl;
}


std::vector<dlib::rectangle> FaceLandmarkTracker::detectFacesOpenCV(const cv::Mat& frame) {
    if (!mInitialized) return {};

    // Scale down frame for faster detection (1/4 resolution)
    cv::Mat smallFrame;
    cv::resize(frame, smallFrame, cv::Size(), 0.25, 0.25, cv::INTER_LINEAR);

    // Convert to grayscale for OpenCV detector
    cv::Mat grayFrame;
    cv::cvtColor(smallFrame, grayFrame, cv::COLOR_BGR2GRAY);

    // Detect faces using OpenCV (much faster than dlib)
    std::vector<cv::Rect> opencvFaces;
    mOpenCVFaceDetector.detectMultiScale(grayFrame, opencvFaces, 1.1, 3, 0, cv::Size(30, 30));

    // Convert to dlib rectangles and scale back to original size
    std::vector<dlib::rectangle> faces;
    for (const auto& rect : opencvFaces) {
        faces.emplace_back(
            dlib::rectangle(
                rect.x * 4, rect.y * 4,
                (rect.x + rect.width) * 4, (rect.y + rect.height) * 4
            )
        );
    }

    return faces;
}

std::vector<cv::Point2f> FaceLandmarkTracker::extractLandmarks(const cv::Mat& frame, const dlib::rectangle& face) {
    if (!mInitialized) return {};

    try {
        // Convert OpenCV Mat to dlib image
        dlib::cv_image<dlib::bgr_pixel> dlibImage(frame);

        // Get facial landmarks
        dlib::full_object_detection shape = mShapePredictor(dlibImage, face);

        // Convert to OpenCV points
        std::vector<cv::Point2f> landmarks;
        for (unsigned long i = 0; i < shape.num_parts(); ++i) {
            landmarks.emplace_back(shape.part(i).x(), shape.part(i).y());
        }

        return landmarks;
    } catch (const std::exception& e) {
        std::cerr << "Failed to extract landmarks: " << e.what() << std::endl;
        return {};
    }
}

void FaceLandmarkTracker::processFrame(cv::Mat& frame) {
    if (!mInitialized) return;

    auto totalStart = std::chrono::high_resolution_clock::now();
    std::vector<dlib::rectangle> faces;

    // Frame skipping optimization: only detect faces every few frames
    mFrameSkipCounter++;
    bool shouldDetectFaces = (mFrameSkipCounter % mFrameSkipInterval == 0) || mLastFaces.empty();

    auto detectionStart = std::chrono::high_resolution_clock::now();
    if (shouldDetectFaces) {
        // Use OpenCV for faster face detection
        faces = detectFacesOpenCV(frame);
        mLastFaces = faces;
        mTrackingFrames = 0;
    } else {
        // Use last detected faces for tracking
        faces = mLastFaces;
        mTrackingFrames++;

        // If we've been tracking too long without re-detection, force detection
        if (mTrackingFrames >= MAX_TRACKING_FRAMES) {
            faces = detectFacesOpenCV(frame);
            mLastFaces = faces;
            mTrackingFrames = 0;
        }
    }
    auto detectionEnd = std::chrono::high_resolution_clock::now();
    auto detectionTime = std::chrono::duration_cast<std::chrono::microseconds>(detectionEnd - detectionStart).count();

    // Process each face
    long totalLandmarkTime = 0;
    long totalIdentificationTime = 0;

    for (const auto& face : faces) {
        // Extract landmarks
        auto extractStart = std::chrono::high_resolution_clock::now();
        auto landmarks = extractLandmarks(frame, face);
        auto extractEnd = std::chrono::high_resolution_clock::now();
        totalLandmarkTime += std::chrono::duration_cast<std::chrono::microseconds>(extractEnd - extractStart).count();

        if (landmarks.empty()) continue;

        // Identify user
        auto identifyStart = std::chrono::high_resolution_clock::now();
        std::string userName = identifyUser(landmarks);
        auto identifyEnd = std::chrono::high_resolution_clock::now();
        totalIdentificationTime += std::chrono::duration_cast<std::chrono::microseconds>(identifyEnd - identifyStart).count();

        // Draw bounding box
        cv::Rect rect(face.left(), face.top(), face.width(), face.height());
        cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);

        // Draw label
        cv::putText(frame, userName, cv::Point(face.left(), face.top() - 10),
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);

        // Draw some key landmarks (only every few frames to reduce drawing overhead)
        if (mFrameSkipCounter % 2 == 0) {
            for (size_t i = 0; i < landmarks.size() && i < 68; ++i) {
                cv::circle(frame, landmarks[i], 1, cv::Scalar(255, 0, 0), -1);
            }
        }
    }
    auto totalEnd = std::chrono::high_resolution_clock::now();

    // Log detailed timing information every 30 frames
    if (mFrameSkipCounter % 30 == 0) {
        auto totalTime = std::chrono::duration_cast<std::chrono::microseconds>(totalEnd - totalStart).count();
        std::cout << "=== Frame " << mFrameSkipCounter << " Timing ===" << std::endl;
        std::cout << "Detection: " << detectionTime/1000.0 << "ms" << std::endl;
        std::cout << "Landmarks: " << totalLandmarkTime/1000.0 << "ms" << std::endl;
        std::cout << "Identification: " << totalIdentificationTime/1000.0 << "ms" << std::endl;
        std::cout << "Total Process: " << totalTime/1000.0 << "ms" << std::endl;
        std::cout << "Faces detected: " << faces.size() << std::endl;
        std::cout << "=========================" << std::endl;
    }
}

} // namespace facelandmark

#pragma once

// Suppress warnings from third-party libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc11-extensions"
#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wpedantic"

#include <opencv2/opencv.hpp>
#include <dlib/opencv.h>
#include <dlib/image_processing/shape_predictor.h>

#pragma GCC diagnostic pop
#include <vector>
#include <string>
#include <memory>
#include "serde.h"

namespace facelandmark {

struct UserLandmark {
    std::string name;                    // User name
    std::vector<float> landmarks;        // Normalized 68-point landmarks as floats (x1,y1,x2,y2,...)

    void serialize(std::vector<uint8_t>& buf) const {
        serde::write(buf, name);
        serde::write(buf, landmarks);
    }

    void deserialize(const std::vector<uint8_t>& buf, size_t& offset) {
        serde::read(buf, offset, name);
        serde::read(buf, offset, landmarks);
    }

    // Helper methods to convert between cv::Point2f and float vector
    void setLandmarks(const std::vector<cv::Point2f>& points) {
        landmarks.clear();
        landmarks.reserve(points.size() * 2);
        for (const auto& point : points) {
            landmarks.push_back(point.x);
            landmarks.push_back(point.y);
        }
    }

    std::vector<cv::Point2f> getLandmarks() const {
        std::vector<cv::Point2f> points;
        points.reserve(landmarks.size() / 2);
        for (size_t i = 0; i < landmarks.size(); i += 2) {
            points.emplace_back(landmarks[i], landmarks[i + 1]);
        }
        return points;
    }
};

struct UserDatabase {
    std::vector<UserLandmark> users;

    void serialize(std::vector<uint8_t>& buf) const {
        serde::write(buf, users);
    }

    void deserialize(const std::vector<uint8_t>& buf, size_t& offset) {
        serde::read(buf, offset, users);
    }
};

class FaceLandmarkTracker {
public:
    FaceLandmarkTracker();
    ~FaceLandmarkTracker();

    bool initialize(const std::string& shapePredictorPath);
    bool loadDatabase(const std::string& databasePath);
    bool saveDatabase(const std::string& databasePath);

    std::vector<cv::Point2f> normalizeLandmarks(const std::vector<cv::Point2f>& landmarks);
    float landmarkDistance(const std::vector<cv::Point2f>& landmarks1,
                          const std::vector<cv::Point2f>& landmarks2);
    std::string identifyUser(const std::vector<cv::Point2f>& landmarks);
    void addUser(const std::string& name, const std::vector<cv::Point2f>& landmarks);

    std::vector<dlib::rectangle> detectFacesOpenCV(const cv::Mat& frame);
    std::vector<cv::Point2f> extractLandmarks(const cv::Mat& frame, const dlib::rectangle& face);

    void processFrame(cv::Mat& frame);

    // Getters
    const UserDatabase& getDatabase() const { return mDatabase; }
    bool isInitialized() const { return mInitialized; }

private:
    dlib::shape_predictor mShapePredictor;
    UserDatabase mDatabase;
    bool mInitialized;
    float mDistanceThreshold;

    // OpenCV face detector for faster detection
    cv::CascadeClassifier mOpenCVFaceDetector;

    // Performance optimization variables
    int mFrameSkipCounter;
    int mFrameSkipInterval;
    std::vector<dlib::rectangle> mLastFaces;
    int mTrackingFrames;
    static const int MAX_TRACKING_FRAMES = 5;

    cv::Point2f calculateCentroid(const std::vector<cv::Point2f>& points);
    float calculateInterocularDistance(const std::vector<cv::Point2f>& landmarks);
};

} // namespace facelandmark

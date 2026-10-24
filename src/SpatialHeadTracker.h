#ifndef JAZZ_SPATIAL_HEAD_TRACKER_H
#define JAZZ_SPATIAL_HEAD_TRACKER_H

#include <vector>
#include <string>
#include <cmath>

namespace Jazz {

struct HeadOrientationQuaternion {
    float w;
    float x;
    float y;
    float z;

    HeadOrientationQuaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    HeadOrientationQuaternion(float _w, float _x, float _y, float _z)
        : w(_w), x(_x), y(_y), z(_z) {}
};

struct HeadOrientationEuler {
    float yawDeg;    // -180.0 to +180.0 (Azimuth turning left/right)
    float pitchDeg;  // -90.0 to +90.0 (Elevation tilting up/down)
    float rollDeg;   // -180.0 to +180.0 (Ear tilt shoulder-to-shoulder)
};

class JZSpatialHeadTracker {
public:
    JZSpatialHeadTracker();
    ~JZSpatialHeadTracker() = default;

    // Telemetry Ingestion
    void setOrientationEuler(float yawDeg, float pitchDeg, float rollDeg);
    void setOrientationQuaternion(float w, float x, float y, float z);

    // OSC (Open Sound Control) Message Parser
    // Supports address "/spatial/tracker/rot" [yaw, pitch, roll]
    // Supports address "/spatial/tracker/quat" [w, x, y, z]
    bool parseOscMessage(const std::string& address, const std::vector<float>& floatArgs);

    // Orientation Accessors
    HeadOrientationEuler getEulerOrientation() const { return m_euler; }
    HeadOrientationQuaternion getQuaternionOrientation() const { return m_quat; }

    // Calibration & Center Reset
    void resetReferenceCenter(); // Sets current head position as 0,0,0
    bool isTrackingActive() const { return m_isTrackingActive; }
    void setTrackingActive(bool active) { m_isTrackingActive = active; }

    // Smoothing & Drift Filter (Complementary / Kalman factor)
    void setSmoothingFactor(float smoothing); // 0.0 (raw) to 0.99 (ultra-smooth)
    float getSmoothingFactor() const { return m_smoothingFactor; }

    // 3D Spatial Rotation Transformation
    // Computes listener-relative sound source azimuth & elevation for HRTF binaural spatializers
    void transformWorldToListenerCoordinates(
        float worldAzimuthDeg, float worldElevationDeg,
        float& outListenerAzimuthDeg, float& outListenerElevationDeg) const;

private:
    HeadOrientationEuler m_euler;
    HeadOrientationQuaternion m_quat;
    HeadOrientationEuler m_centerOffset;

    float m_smoothingFactor;
    bool m_isTrackingActive;

    void updateEulerFromQuat();
    void updateQuatFromEuler();
    static float normalizeAngle180(float angleDeg);
};

} // namespace Jazz

#endif // JAZZ_SPATIAL_HEAD_TRACKER_H

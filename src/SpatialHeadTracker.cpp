#include "SpatialHeadTracker.h"
#include <algorithm>

namespace Jazz {

static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / 3.14159265358979323846f;

JZSpatialHeadTracker::JZSpatialHeadTracker()
    : m_smoothingFactor(0.0f)
    , m_isTrackingActive(true)
{
    m_euler = { 0.0f, 0.0f, 0.0f };
    m_quat = { 1.0f, 0.0f, 0.0f, 0.0f };
    m_centerOffset = { 0.0f, 0.0f, 0.0f };
}

void JZSpatialHeadTracker::setOrientationEuler(float yawDeg, float pitchDeg, float rollDeg)
{
    float targetYaw = normalizeAngle180(yawDeg);
    float targetPitch = std::clamp(pitchDeg, -90.0f, 90.0f);
    float targetRoll = normalizeAngle180(rollDeg);

    if (m_smoothingFactor > 0.0f) {
        m_euler.yawDeg = m_euler.yawDeg * m_smoothingFactor + targetYaw * (1.0f - m_smoothingFactor);
        m_euler.pitchDeg = m_euler.pitchDeg * m_smoothingFactor + targetPitch * (1.0f - m_smoothingFactor);
        m_euler.rollDeg = m_euler.rollDeg * m_smoothingFactor + targetRoll * (1.0f - m_smoothingFactor);
    } else {
        m_euler.yawDeg = targetYaw;
        m_euler.pitchDeg = targetPitch;
        m_euler.rollDeg = targetRoll;
    }

    updateQuatFromEuler();
}

void JZSpatialHeadTracker::setOrientationQuaternion(float w, float x, float y, float z)
{
    float len = std::sqrt(w * w + x * x + y * y + z * z);
    if (len > 0.00001f) {
        m_quat.w = w / len;
        m_quat.x = x / len;
        m_quat.y = y / len;
        m_quat.z = z / len;
    }

    updateEulerFromQuat();
}

bool JZSpatialHeadTracker::parseOscMessage(const std::string& address, const std::vector<float>& floatArgs)
{
    if (address == "/spatial/tracker/rot" && floatArgs.size() >= 3) {
        setOrientationEuler(floatArgs[0], floatArgs[1], floatArgs[2]);
        return true;
    }
    if (address == "/spatial/tracker/quat" && floatArgs.size() >= 4) {
        setOrientationQuaternion(floatArgs[0], floatArgs[1], floatArgs[2], floatArgs[3]);
        return true;
    }
    return false;
}

void JZSpatialHeadTracker::resetReferenceCenter()
{
    m_centerOffset = m_euler;
}

void JZSpatialHeadTracker::setSmoothingFactor(float smoothing)
{
    m_smoothingFactor = std::clamp(smoothing, 0.0f, 0.99f);
}

void JZSpatialHeadTracker::transformWorldToListenerCoordinates(
    float worldAzimuthDeg, float worldElevationDeg,
    float& outListenerAzimuthDeg, float& outListenerElevationDeg) const
{
    if (!m_isTrackingActive) {
        outListenerAzimuthDeg = normalizeAngle180(worldAzimuthDeg);
        outListenerElevationDeg = std::clamp(worldElevationDeg, -90.0f, 90.0f);
        return;
    }

    // Effective listener orientation relative to calibrated center
    float listenerYaw = normalizeAngle180(m_euler.yawDeg - m_centerOffset.yawDeg);
    float listenerPitch = m_euler.pitchDeg - m_centerOffset.pitchDeg;

    // Relative azimuth: when listener turns right (+yaw), sound on front moves left (-azimuth)
    float relAzimuth = worldAzimuthDeg - listenerYaw;
    outListenerAzimuthDeg = normalizeAngle180(relAzimuth);

    // Relative elevation
    float relElevation = worldElevationDeg - listenerPitch;
    outListenerElevationDeg = std::clamp(relElevation, -90.0f, 90.0f);
}

void JZSpatialHeadTracker::updateEulerFromQuat()
{
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (m_quat.w * m_quat.x + m_quat.y * m_quat.z);
    float cosr_cosp = 1.0f - 2.0f * (m_quat.x * m_quat.x + m_quat.y * m_quat.y);
    m_euler.rollDeg = std::atan2(sinr_cosp, cosr_cosp) * RAD_TO_DEG;

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (m_quat.w * m_quat.y - m_quat.z * m_quat.x);
    if (std::abs(sinp) >= 1.0f)
        m_euler.pitchDeg = std::copysign(90.0f, sinp); // Use 90 degrees if out of range
    else
        m_euler.pitchDeg = std::asin(sinp) * RAD_TO_DEG;

    // Yaw (z-axis rotation)
    float siny_cosp = 2.0f * (m_quat.w * m_quat.z + m_quat.x * m_quat.y);
    float cosy_cosp = 1.0f - 2.0f * (m_quat.y * m_quat.y + m_quat.z * m_quat.z);
    m_euler.yawDeg = std::atan2(siny_cosp, cosy_cosp) * RAD_TO_DEG;
}

void JZSpatialHeadTracker::updateQuatFromEuler()
{
    float cy = std::cos(m_euler.yawDeg * 0.5f * DEG_TO_RAD);
    float sy = std::sin(m_euler.yawDeg * 0.5f * DEG_TO_RAD);
    float cp = std::cos(m_euler.pitchDeg * 0.5f * DEG_TO_RAD);
    float sp = std::sin(m_euler.pitchDeg * 0.5f * DEG_TO_RAD);
    float cr = std::cos(m_euler.rollDeg * 0.5f * DEG_TO_RAD);
    float sr = std::sin(m_euler.rollDeg * 0.5f * DEG_TO_RAD);

    m_quat.w = cr * cp * cy + sr * sp * sy;
    m_quat.x = sr * cp * cy - cr * sp * sy;
    m_quat.y = cr * sp * cy + sr * cp * sy;
    m_quat.z = cr * cp * sy - sr * sp * cy;
}

float JZSpatialHeadTracker::normalizeAngle180(float angleDeg)
{
    while (angleDeg > 180.0f) angleDeg -= 360.0f;
    while (angleDeg < -180.0f) angleDeg += 360.0f;
    return angleDeg;
}

} // namespace Jazz

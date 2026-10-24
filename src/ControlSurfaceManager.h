#ifndef JAZZ_CONTROL_SURFACE_MANAGER_H
#define JAZZ_CONTROL_SURFACE_MANAGER_H

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Jazz {

enum class ControlSurfaceProtocol {
    MACKIE_CONTROL_UNIVERSAL, // MCU standard (pitch bend 10-bit faders, sysex scribble strips)
    MACKIE_HUI,               // Pro Tools HUI protocol
    CUSTOM_OSC                // Network Open Sound Control surface
};

enum class VPotLedRingMode {
    SINGLE_DOT,  // Panning center-relative
    BOOST_CUT,   // Volume / Gain boost-cut bar
    WRAP,        // Full ring wrap around
    SPREAD       // Stereo width spread
};

struct ChannelStripHardwareState {
    int channelIndex;
    std::string trackName;     // Up to 7 characters for MCU LCD scribble strip
    uint16_t motorizedFader10Bit; // 0 to 1023 (or 0 to 16383 in 14-bit)
    uint8_t vpotPosition;      // 0 to 127
    VPotLedRingMode ledMode;
    bool isMuted;
    bool isSoloed;
    bool isArmed;
    bool isSelected;
};

class JZControlSurfaceManager {
public:
    JZControlSurfaceManager(ControlSurfaceProtocol protocol = ControlSurfaceProtocol::MACKIE_CONTROL_UNIVERSAL);
    ~JZControlSurfaceManager() = default;

    // Protocol Configuration
    ControlSurfaceProtocol getProtocol() const { return m_protocol; }
    void setProtocol(ControlSurfaceProtocol protocol) { m_protocol = protocol; }

    // Bank Navigation (8 faders per bank)
    int getCurrentBankOffset() const { return m_bankOffset; }
    void bankLeft();
    void bankRight(int maxTracks);
    void channelLeft();
    void channelRight(int maxTracks);

    // Track Binding & Scribble Strip Management
    void setTrackInfo(int globalTrackIdx, const std::string& trackName, float volumeNorm, float panNorm);
    const ChannelStripHardwareState* getChannelState(int localFaderIdx) const; // 0 to 7

    // MCU Motorized Fader Formatting (10-bit pitch bend representation)
    std::vector<uint8_t> generateFaderMotorMessage(int localFaderIdx, uint16_t faderVal10Bit) const;
    bool parseFaderMoveMessage(uint8_t statusByte, uint8_t data1, uint8_t data2,
                               int& outLocalFaderIdx, uint16_t& outFaderVal10Bit) const;

    // MCU LCD Scribble Strip SysEx Formatting
    // SysEx format: F0 00 00 66 14 12 <offset> <7 chars per channel> F7
    std::vector<uint8_t> generateScribbleStripSysEx(int lineIndex = 0) const;

    // Transport & Jog Wheel
    bool isPlaying() const { return m_isPlaying; }
    void setPlaying(bool play) { m_isPlaying = play; }

    bool isRecording() const { return m_isRecording; }
    void setRecording(bool rec) { m_isRecording = rec; }

    int processJogWheelDelta(int8_t tickDelta);
    int64_t getCurrentTransportClockTicks() const { return m_transportTicks; }

private:
    ControlSurfaceProtocol m_protocol;
    int m_bankOffset;
    bool m_isPlaying;
    bool m_isRecording;
    int64_t m_transportTicks;

    std::vector<ChannelStripHardwareState> m_channelStrips; // 8 faders per bank
};

} // namespace Jazz

#endif // JAZZ_CONTROL_SURFACE_MANAGER_H

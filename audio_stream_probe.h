#pragma once
#include <type_traits>
#include <utility>
// Read-only probe for shared Audio fields. Explicit template
// instantiation permits private member pointers without changing its layout,
// replacing the library, or redefining private throughout system headers.
template<class Tag, typename Tag::type Member> struct AudioProbeAccess {
    friend typename Tag::type probeMember(Tag) { return Member; }
};
#define AUDIO_PROBE_FIELD(name, fieldType, field) \
struct name { using type = fieldType Audio::*; friend type probeMember(name); }; \
template struct AudioProbeAccess<name, &Audio::field>;
AUDIO_PROBE_FIELD(ProbeMetaCount, uint32_t, m_metacount)
AUDIO_PROBE_FIELD(ProbeMetaInterval, uint32_t, m_metaint)
AUDIO_PROBE_FIELD(ProbeChunked, bool, m_f_chunked)
AUDIO_PROBE_FIELD(ProbeType, uint8_t, m_streamType)
AUDIO_PROBE_FIELD(ProbeValid, int16_t, m_validSamples)
AUDIO_PROBE_FIELD(ProbeCurrent, int16_t, m_curSample)
#undef AUDIO_PROBE_FIELD

// 4.x ships these headers; the pinned 3.x library does not.
// Keep different private names in separate preprocessor branches.
#if __has_include("audiolib_structs.hpp") && __has_include("psram_unique_ptr.hpp")
#define AUDIO_PROBE_V4 1
#define AUDIO_PROBE_FIELD(name, fieldType, field) \
struct name { using type = fieldType Audio::*; friend type probeMember(name); }; \
template struct AudioProbeAccess<name, &Audio::field>;
AUDIO_PROBE_FIELD(ProbeSecureClient, NetworkClientSecure, clientsecure)
AUDIO_PROBE_FIELD(ProbeClient, NetworkClient*, m_client)
AUDIO_PROBE_FIELD(ProbeMode, uint16_t, m_dataMode)
AUDIO_PROBE_FIELD(ProbePosition, uint32_t, m_audioFilePosition)
#else
#define AUDIO_PROBE_V4 0
#define AUDIO_PROBE_FIELD(name, fieldType, field) \
struct name { using type = fieldType Audio::*; friend type probeMember(name); }; \
template struct AudioProbeAccess<name, &Audio::field>;
AUDIO_PROBE_FIELD(ProbeSecureClient, WiFiClientSecure, clientsecure)
AUDIO_PROBE_FIELD(ProbeClient, WiFiClient*, _client)
AUDIO_PROBE_FIELD(ProbeMode, uint16_t, m_datamode)
#endif
AUDIO_PROBE_FIELD(ProbeSSL, bool, m_f_ssl)
#undef AUDIO_PROBE_FIELD

// Access the library accessor without depending on its client field name.
struct ProbeAvailable { using type = uint32_t (Audio::*)(); friend type probeMember(ProbeAvailable); };
template struct AudioProbeAccess<ProbeAvailable, &Audio::streamavail>;

// Shared input buffer and state accessor, not version-specific client fields.
struct ProbeInput { using type = AudioBuffer Audio::*; friend type probeMember(ProbeInput); };
template struct AudioProbeAccess<ProbeInput, &Audio::InBuff>;

// Older AudioBuffer versions do not expose positions. Do not call getReadPtr:
// that accessor can copy frame data, unlike the read-only position accessors.
template<class Buffer>
static auto probeBufferPositions(Buffer& buffer, uint32_t& read, uint32_t& write, int)
    -> decltype(buffer.getReadPos(), buffer.getWritePos(), bool()) {
    read = buffer.getReadPos();
    write = buffer.getWritePos();
    return true;
}
template<class Buffer>
static bool probeBufferPositions(Buffer&, uint32_t&, uint32_t&, long) {
    return false;
}

// Diagnostic access only: preserve the core's BIO context and callbacks.
using TLSProbeClient = typename std::remove_reference<
    decltype(std::declval<Audio&>().*probeMember(ProbeSecureClient{}))>::type;
struct ProbeTLSContext {
    using type = std::shared_ptr<sslclient_context> TLSProbeClient::*;
    friend type probeTLSContext(ProbeTLSContext);
};
template<typename Tag, typename Tag::type Member> struct TLSProbeAccess {
    friend typename Tag::type probeTLSContext(Tag) { return Member; }
};
template struct TLSProbeAccess<ProbeTLSContext, &TLSProbeClient::sslclient>;

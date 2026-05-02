#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace portbridge::protocol {

inline constexpr std::size_t kFrameHeaderSize = 9;
inline constexpr std::uint32_t kMaxPayloadSize = 1024 * 1024;

enum class FrameType : std::uint8_t {
  data = 1,
  open = 2,
  close = 3,
  ping = 4,
  error = 5,
};

struct TunnelFrame {
  FrameType type{};
  std::uint32_t stream_id{};
  std::vector<std::byte> payload;
};

enum class DecodeStatus {
  complete,
  need_more,
  invalid_type,
  payload_too_large,
};

struct DecodeResult {
  DecodeStatus status{};
  std::optional<TunnelFrame> frame;
  std::size_t bytes_consumed{};
  std::string error;

  [[nodiscard]] bool ok() const;
};

[[nodiscard]] std::vector<std::byte> encode_frame(const TunnelFrame& frame);

[[nodiscard]] DecodeResult decode_frame(
    std::span<const std::byte> bytes,
    std::uint32_t max_payload_size = kMaxPayloadSize);

}  // namespace portbridge::protocol

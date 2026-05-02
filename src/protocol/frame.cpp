#include "portbridge/protocol/frame.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace portbridge::protocol {
namespace {

std::uint8_t byte_value(std::byte value) {
  return std::to_integer<std::uint8_t>(value);
}

void append_u32(std::vector<std::byte> &output, std::uint32_t value) {
  output.push_back(static_cast<std::byte>((value >> 24) & 0xFF));
  output.push_back(static_cast<std::byte>((value >> 16) & 0xFF));
  output.push_back(static_cast<std::byte>((value >> 8) & 0xFF));
  output.push_back(static_cast<std::byte>(value & 0xFF));
}

std::uint32_t read_u32(std::span<const std::byte> bytes, std::size_t offset) {
  return (static_cast<std::uint32_t>(byte_value(bytes[offset])) << 24) |
         (static_cast<std::uint32_t>(byte_value(bytes[offset + 1])) << 16) |
         (static_cast<std::uint32_t>(byte_value(bytes[offset + 2])) << 8) |
         static_cast<std::uint32_t>(byte_value(bytes[offset + 3]));
}

std::optional<FrameType> parse_frame_type(std::byte raw_type) {
  switch (byte_value(raw_type)) {
  case static_cast<std::uint8_t>(FrameType::data):
    return FrameType::data;
  case static_cast<std::uint8_t>(FrameType::open):
    return FrameType::open;
  case static_cast<std::uint8_t>(FrameType::close):
    return FrameType::close;
  case static_cast<std::uint8_t>(FrameType::ping):
    return FrameType::ping;
  case static_cast<std::uint8_t>(FrameType::error):
    return FrameType::error;
  default:
    return std::nullopt;
  }
}

DecodeResult need_more() {
  return DecodeResult{DecodeStatus::need_more, std::nullopt, 0, {}};
}

} // namespace

bool DecodeResult::ok() const {
  return status == DecodeStatus::complete && frame.has_value() && error.empty();
}

std::vector<std::byte> encode_frame(const TunnelFrame &frame) {
  if (frame.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("frame payload is too large to encode");
  }

  std::vector<std::byte> output;
  output.reserve(kFrameHeaderSize + frame.payload.size());

  output.push_back(
      static_cast<std::byte>(static_cast<std::uint8_t>(frame.type)));
  append_u32(output, frame.stream_id);
  append_u32(output, static_cast<std::uint32_t>(frame.payload.size()));
  output.insert(output.end(), frame.payload.begin(), frame.payload.end());

  return output;
}

DecodeResult decode_frame(std::span<const std::byte> bytes,
                          std::uint32_t max_payload_size) {
  if (bytes.size() < kFrameHeaderSize) {
    return need_more();
  }

  const auto type = parse_frame_type(bytes[0]);
  if (!type.has_value()) {
    return DecodeResult{DecodeStatus::invalid_type, std::nullopt, 0,
                        "unknown frame type"};
  }

  const auto stream_id = read_u32(bytes, 1);
  const auto payload_size = read_u32(bytes, 5);

  if (payload_size > max_payload_size) {
    return DecodeResult{DecodeStatus::payload_too_large, std::nullopt, 0,
                        "frame payload exceeds configured limit"};
  }

  const auto frame_size =
      kFrameHeaderSize + static_cast<std::size_t>(payload_size);
  if (bytes.size() < frame_size) {
    return need_more();
  }

  auto payload_begin =
      bytes.begin() + static_cast<std::ptrdiff_t>(kFrameHeaderSize);
  auto payload_end = payload_begin + static_cast<std::ptrdiff_t>(payload_size);

  TunnelFrame frame{
      *type,
      stream_id,
      std::vector<std::byte>(payload_begin, payload_end),
  };

  return DecodeResult{DecodeStatus::complete, std::move(frame), frame_size, {}};
}

} // namespace portbridge::protocol

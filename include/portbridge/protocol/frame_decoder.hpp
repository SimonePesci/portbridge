#pragma once

#include "portbridge/protocol/frame.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace portbridge::protocol {

class FrameDecoder {
 public:
  explicit FrameDecoder(std::uint32_t max_payload_size = kMaxPayloadSize);

  void append(std::span<const std::byte> bytes);
  [[nodiscard]] DecodeResult next_frame();

  [[nodiscard]] std::size_t buffered_bytes() const;
  void clear();

 private:
  std::uint32_t max_payload_size_;
  std::vector<std::byte> buffer_;
};

}  // namespace portbridge::protocol

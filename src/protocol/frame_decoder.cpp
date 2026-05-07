#include "portbridge/protocol/frame_decoder.hpp"

namespace portbridge::protocol {

FrameDecoder::FrameDecoder(std::uint32_t max_payload_size)
    : max_payload_size_(max_payload_size) {}

void FrameDecoder::append(std::span<const std::byte> bytes) {
  buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

DecodeResult FrameDecoder::next_frame() {
  auto result = decode_frame(buffer_, max_payload_size_);

  if (result.ok()) {
    buffer_.erase(buffer_.begin(),
                  buffer_.begin() + static_cast<std::ptrdiff_t>(result.bytes_consumed));
  }

  return result;
}

std::size_t FrameDecoder::buffered_bytes() const {
  return buffer_.size();
}

void FrameDecoder::clear() {
  buffer_.clear();
}

}  // namespace portbridge::protocol

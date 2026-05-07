#include "portbridge/protocol/frame_decoder.hpp"

#include "portbridge/protocol/frame.hpp"

#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAILED: " << message << '\n';
  }
}

std::vector<std::byte> bytes(std::initializer_list<unsigned int> values) {
  std::vector<std::byte> result;
  result.reserve(values.size());

  for (const auto value : values) {
    result.push_back(static_cast<std::byte>(value));
  }

  return result;
}

std::vector<std::byte> payload(std::string_view value) {
  std::vector<std::byte> result;
  result.reserve(value.size());

  for (const auto character : value) {
    result.push_back(static_cast<std::byte>(character));
  }

  return result;
}

void partial_header_waits_for_more_bytes() {
  portbridge::protocol::FrameDecoder decoder;

  decoder.append(bytes({0x01, 0x00}));
  const auto result = decoder.next_frame();

  expect(result.status == portbridge::protocol::DecodeStatus::need_more,
         "partial header should need more bytes");
  expect(decoder.buffered_bytes() == 2,
         "partial header should remain buffered");
}

void partial_payload_waits_for_more_bytes() {
  portbridge::protocol::FrameDecoder decoder;

  decoder.append(bytes({0x01, 0x00, 0x00, 0x00, 0x01,
                        0x00, 0x00, 0x00, 0x04,
                        0xAA, 0xBB}));
  const auto result = decoder.next_frame();

  expect(result.status == portbridge::protocol::DecodeStatus::need_more,
         "partial payload should need more bytes");
  expect(decoder.buffered_bytes() == 11,
         "partial payload should remain buffered");
}

void appended_remainder_completes_frame() {
  portbridge::protocol::FrameDecoder decoder;

  decoder.append(bytes({0x01, 0x00, 0x00, 0x00, 0x07,
                        0x00, 0x00, 0x00, 0x04,
                        0xAA}));
  expect(decoder.next_frame().status == portbridge::protocol::DecodeStatus::need_more,
         "first chunk should be incomplete");

  decoder.append(bytes({0xBB, 0xCC, 0xDD}));
  const auto result = decoder.next_frame();

  expect(result.ok(), "second chunk should complete frame");
  expect(result.frame.has_value() &&
             result.frame->type == portbridge::protocol::FrameType::data,
         "completed frame should keep type");
  expect(result.frame.has_value() && result.frame->stream_id == 7,
         "completed frame should keep stream id");
  expect(result.frame.has_value() &&
             result.frame->payload == bytes({0xAA, 0xBB, 0xCC, 0xDD}),
         "completed frame should keep payload");
  expect(decoder.buffered_bytes() == 0,
         "completed frame should be removed from the buffer");
}

void returns_two_frames_one_at_a_time() {
  portbridge::protocol::FrameDecoder decoder;

  const portbridge::protocol::TunnelFrame first{
      portbridge::protocol::FrameType::open,
      10,
      payload("one"),
  };
  const portbridge::protocol::TunnelFrame second{
      portbridge::protocol::FrameType::close,
      11,
      payload("two"),
  };

  auto input = portbridge::protocol::encode_frame(first);
  const auto encoded_second = portbridge::protocol::encode_frame(second);
  input.insert(input.end(), encoded_second.begin(), encoded_second.end());

  decoder.append(input);

  const auto first_result = decoder.next_frame();
  expect(first_result.ok(), "first frame should decode");
  expect(first_result.frame.has_value() && first_result.frame->stream_id == 10,
         "first frame should preserve stream id");
  expect(decoder.buffered_bytes() == encoded_second.size(),
         "second frame should stay buffered after first decode");

  const auto second_result = decoder.next_frame();
  expect(second_result.ok(), "second frame should decode");
  expect(second_result.frame.has_value() && second_result.frame->stream_id == 11,
         "second frame should preserve stream id");
  expect(decoder.buffered_bytes() == 0,
         "buffer should be empty after both frames decode");
}

void keeps_partial_second_frame_buffered() {
  portbridge::protocol::FrameDecoder decoder;

  const auto first = portbridge::protocol::encode_frame({
      portbridge::protocol::FrameType::ping,
      1,
      {},
  });
  const auto second_prefix = bytes({0x01, 0x00, 0x00});

  auto input = first;
  input.insert(input.end(), second_prefix.begin(), second_prefix.end());

  decoder.append(input);
  const auto result = decoder.next_frame();

  expect(result.ok(), "complete first frame should decode");
  expect(decoder.buffered_bytes() == second_prefix.size(),
         "partial second frame should stay buffered");
  expect(decoder.next_frame().status == portbridge::protocol::DecodeStatus::need_more,
         "partial second frame should still need more bytes");
}

void invalid_type_does_not_discard_buffer() {
  portbridge::protocol::FrameDecoder decoder;

  decoder.append(bytes({0xFF, 0x00, 0x00, 0x00, 0x01,
                        0x00, 0x00, 0x00, 0x00}));
  const auto result = decoder.next_frame();

  expect(result.status == portbridge::protocol::DecodeStatus::invalid_type,
         "invalid type should be reported");
  expect(decoder.buffered_bytes() == portbridge::protocol::kFrameHeaderSize,
         "invalid frame should remain buffered for caller handling");
}

void oversized_payload_does_not_discard_buffer() {
  portbridge::protocol::FrameDecoder decoder(4);

  decoder.append(bytes({0x01, 0x00, 0x00, 0x00, 0x01,
                        0x00, 0x00, 0x00, 0x05}));
  const auto result = decoder.next_frame();

  expect(result.status == portbridge::protocol::DecodeStatus::payload_too_large,
         "oversized payload should be reported");
  expect(decoder.buffered_bytes() == portbridge::protocol::kFrameHeaderSize,
         "oversized frame should remain buffered for caller handling");
}

void clear_discards_buffered_bytes() {
  portbridge::protocol::FrameDecoder decoder;

  decoder.append(bytes({0x01, 0x00, 0x00}));
  decoder.clear();

  expect(decoder.buffered_bytes() == 0, "clear should discard buffered bytes");
  expect(decoder.next_frame().status == portbridge::protocol::DecodeStatus::need_more,
         "empty decoder should need more bytes");
}

}  // namespace

int main() {
  partial_header_waits_for_more_bytes();
  partial_payload_waits_for_more_bytes();
  appended_remainder_completes_frame();
  returns_two_frames_one_at_a_time();
  keeps_partial_second_frame_buffered();
  invalid_type_does_not_discard_buffer();
  oversized_payload_does_not_discard_buffer();
  clear_discards_buffered_bytes();

  if (failures != 0) {
    std::cerr << failures << " test expectation(s) failed\n";
    return 1;
  }

  return 0;
}

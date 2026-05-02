#include "portbridge/protocol/frame.hpp"

#include <cstddef>
#include <cstdint>
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

void encodes_frame_header_in_network_order() {
  const portbridge::protocol::TunnelFrame frame{
      portbridge::protocol::FrameType::data,
      0x01020304,
      payload("abc"),
  };

  const auto encoded = portbridge::protocol::encode_frame(frame);

  expect(encoded == bytes({0x01, 0x01, 0x02, 0x03, 0x04,
                           0x00, 0x00, 0x00, 0x03,
                           0x61, 0x62, 0x63}),
         "encoded frame should use type, stream id, length, payload");
}

void decodes_complete_frame() {
  const auto encoded = bytes({0x02, 0x00, 0x00, 0x00, 0x2A,
                              0x00, 0x00, 0x00, 0x05,
                              0x68, 0x65, 0x6C, 0x6C, 0x6F});

  const auto result = portbridge::protocol::decode_frame(encoded);

  expect(result.ok(), "complete frame should decode");
  expect(result.bytes_consumed == encoded.size(),
         "complete frame should report consumed bytes");
  expect(result.frame.has_value(), "complete frame should contain a frame");
  expect(result.frame.has_value() &&
             result.frame->type == portbridge::protocol::FrameType::open,
         "decoded frame should keep type");
  expect(result.frame.has_value() && result.frame->stream_id == 42,
         "decoded frame should keep stream id");
  expect(result.frame.has_value() && result.frame->payload == payload("hello"),
         "decoded frame should keep payload");
}

void reports_need_more_for_partial_header() {
  const auto result = portbridge::protocol::decode_frame(bytes({0x01, 0x00}));

  expect(result.status == portbridge::protocol::DecodeStatus::need_more,
         "partial header should need more bytes");
  expect(!result.frame.has_value(), "partial header should not return frame");
  expect(result.bytes_consumed == 0, "partial header should consume nothing");
}

void reports_need_more_for_partial_payload() {
  const auto result = portbridge::protocol::decode_frame(
      bytes({0x01, 0x00, 0x00, 0x00, 0x01,
             0x00, 0x00, 0x00, 0x04,
             0xAA, 0xBB}));

  expect(result.status == portbridge::protocol::DecodeStatus::need_more,
         "partial payload should need more bytes");
  expect(!result.frame.has_value(), "partial payload should not return frame");
  expect(result.bytes_consumed == 0, "partial payload should consume nothing");
}

void rejects_unknown_frame_type() {
  const auto result = portbridge::protocol::decode_frame(
      bytes({0xFF, 0x00, 0x00, 0x00, 0x01,
             0x00, 0x00, 0x00, 0x00}));

  expect(result.status == portbridge::protocol::DecodeStatus::invalid_type,
         "unknown type should be rejected");
  expect(!result.frame.has_value(), "unknown type should not return frame");
}

void rejects_payload_larger_than_limit() {
  const auto result = portbridge::protocol::decode_frame(
      bytes({0x01, 0x00, 0x00, 0x00, 0x01,
             0x00, 0x00, 0x00, 0x05}),
      4);

  expect(result.status == portbridge::protocol::DecodeStatus::payload_too_large,
         "payload larger than limit should be rejected");
  expect(!result.frame.has_value(), "oversized payload should not return frame");
}

void decodes_one_frame_and_reports_consumed_bytes() {
  auto input = bytes({0x04, 0x00, 0x00, 0x00, 0x09,
                      0x00, 0x00, 0x00, 0x00});
  input.push_back(static_cast<std::byte>(0x01));
  input.push_back(static_cast<std::byte>(0x02));

  const auto result = portbridge::protocol::decode_frame(input);

  expect(result.ok(), "first frame should decode even with trailing bytes");
  expect(result.bytes_consumed == portbridge::protocol::kFrameHeaderSize,
         "decoder should report only the first frame as consumed");
  expect(result.frame.has_value() &&
             result.frame->type == portbridge::protocol::FrameType::ping,
         "decoder should keep zero-length frame type");
}

}  // namespace

int main() {
  encodes_frame_header_in_network_order();
  decodes_complete_frame();
  reports_need_more_for_partial_header();
  reports_need_more_for_partial_payload();
  rejects_unknown_frame_type();
  rejects_payload_larger_than_limit();
  decodes_one_frame_and_reports_consumed_bytes();

  if (failures != 0) {
    std::cerr << failures << " test expectation(s) failed\n";
    return 1;
  }

  return 0;
}

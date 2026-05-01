#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace portbridge {

inline constexpr std::string_view kVersion = "0.1.0";

struct HelpCommand {};

struct VersionCommand {};

struct ServerCommand {
  std::uint16_t listen_port{};
};

struct ExposeCommand {
  std::uint16_t local_port{};
  std::string remote_host;
};

using Command = std::variant<HelpCommand, VersionCommand, ServerCommand, ExposeCommand>;

struct ParseResult {
  std::optional<Command> command;
  std::string error;

  [[nodiscard]] bool ok() const;
};

[[nodiscard]] ParseResult parse_cli(const std::vector<std::string_view>& args);
[[nodiscard]] std::string help_text(std::string_view program_name);
[[nodiscard]] std::string describe_command(const Command& command);

}  // namespace portbridge

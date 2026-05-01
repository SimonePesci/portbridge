#include "portbridge/cli.hpp"

#include <charconv>
#include <limits>
#include <sstream>
#include <type_traits>
#include <utility>

namespace portbridge {
namespace {

ParseResult make_error(std::string message) {
  return ParseResult{std::nullopt, std::move(message)};
}

std::optional<std::uint16_t> parse_port(std::string_view value) {
  if (value.empty()) {
    return std::nullopt;
  }

  unsigned int parsed{};
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);

  if (result.ec != std::errc{} || result.ptr != end || parsed == 0 ||
      parsed > std::numeric_limits<std::uint16_t>::max()) {
    return std::nullopt;
  }

  return static_cast<std::uint16_t>(parsed);
}

ParseResult parse_server(const std::vector<std::string_view>& args) {
  std::optional<std::uint16_t> listen_port;

  for (std::size_t index = 1; index < args.size(); ++index) {
    const auto arg = args[index];

    if (arg != "--listen") {
      return make_error("unknown server option: " + std::string(arg));
    }

    if (index + 1 >= args.size()) {
      return make_error("--listen requires a port");
    }

    const auto port = parse_port(args[++index]);
    if (!port.has_value()) {
      return make_error("--listen must be a port between 1 and 65535");
    }

    listen_port = *port;
  }

  if (!listen_port.has_value()) {
    return make_error("server requires --listen <port>");
  }

  return ParseResult{ServerCommand{*listen_port}, {}};
}

ParseResult parse_expose(const std::vector<std::string_view>& args) {
  std::optional<std::uint16_t> local_port;
  std::optional<std::string> remote_host;

  for (std::size_t index = 1; index < args.size(); ++index) {
    const auto arg = args[index];

    if (arg == "--local") {
      if (index + 1 >= args.size()) {
        return make_error("--local requires a port");
      }

      const auto port = parse_port(args[++index]);
      if (!port.has_value()) {
        return make_error("--local must be a port between 1 and 65535");
      }

      local_port = *port;
      continue;
    }

    if (arg == "--remote") {
      if (index + 1 >= args.size()) {
        return make_error("--remote requires a host name");
      }

      auto host = std::string(args[++index]);
      if (host.empty()) {
        return make_error("--remote requires a non-empty host name");
      }

      remote_host = std::move(host);
      continue;
    }

    return make_error("unknown expose option: " + std::string(arg));
  }

  if (!local_port.has_value()) {
    return make_error("expose requires --local <port>");
  }

  if (!remote_host.has_value()) {
    return make_error("expose requires --remote <host>");
  }

  return ParseResult{ExposeCommand{*local_port, *remote_host}, {}};
}

}  // namespace

bool ParseResult::ok() const {
  return command.has_value() && error.empty();
}

ParseResult parse_cli(const std::vector<std::string_view>& args) {
  if (args.empty()) {
    return ParseResult{HelpCommand{}, {}};
  }

  const auto command = args.front();

  if (command == "-h" || command == "--help" || command == "help") {
    return ParseResult{HelpCommand{}, {}};
  }

  if (command == "-v" || command == "--version" || command == "version") {
    return ParseResult{VersionCommand{}, {}};
  }

  if (command == "server") {
    return parse_server(args);
  }

  if (command == "expose") {
    return parse_expose(args);
  }

  return make_error("unknown command: " + std::string(command));
}

std::string help_text(std::string_view program_name) {
  std::ostringstream output;
  output << "PortBridge " << kVersion << '\n'
         << '\n'
         << "Usage:\n"
         << "  " << program_name << " server --listen <port>\n"
         << "  " << program_name << " expose --local <port> --remote <host>\n"
         << "  " << program_name << " --version\n"
         << '\n'
         << "Commands:\n"
         << "  server   Accept tunnel clients on the given public port\n"
         << "  expose   Connect to a server and expose a local port\n";
  return output.str();
}

std::string describe_command(const Command& command) {
  return std::visit(
      [](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;

        if constexpr (std::is_same_v<T, HelpCommand>) {
          return "help";
        } else if constexpr (std::is_same_v<T, VersionCommand>) {
          return "version";
        } else if constexpr (std::is_same_v<T, ServerCommand>) {
          return "server listen=" + std::to_string(value.listen_port);
        } else {
          return "expose local=" + std::to_string(value.local_port) +
                 " remote=" + value.remote_host;
        }
      },
      command);
}

}  // namespace portbridge

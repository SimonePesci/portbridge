#include "portbridge/cli.hpp"

#include <iostream>
#include <string_view>
#include <variant>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAILED: " << message << '\n';
  }
}

void defaults_to_help() {
  const auto result = portbridge::parse_cli({});

  expect(result.ok(), "empty args should parse");
  expect(std::holds_alternative<portbridge::HelpCommand>(*result.command),
         "empty args should select help");
}

void parses_version_command() {
  const auto result = portbridge::parse_cli({"--version"});

  expect(result.ok(), "--version should parse");
  expect(std::holds_alternative<portbridge::VersionCommand>(*result.command),
         "--version should select version");
}

void parses_server_listen_port() {
  const auto result = portbridge::parse_cli({"server", "--listen", "443"});

  expect(result.ok(), "server --listen should parse");
  const auto* command = std::get_if<portbridge::ServerCommand>(&*result.command);
  expect(command != nullptr, "server should produce ServerCommand");
  expect(command != nullptr && command->listen_port == 443,
         "server should keep listen port");
}

void rejects_server_without_port() {
  const auto result = portbridge::parse_cli({"server", "--listen"});

  expect(!result.ok(), "server --listen without value should fail");
  expect(result.error == "--listen requires a port",
         "server missing port should explain the error");
}

void parses_expose_command() {
  const auto result = portbridge::parse_cli(
      {"expose", "--remote", "myapp.example.com", "--local", "3000"});

  expect(result.ok(), "expose should parse flags in any order");
  const auto* command = std::get_if<portbridge::ExposeCommand>(&*result.command);
  expect(command != nullptr, "expose should produce ExposeCommand");
  expect(command != nullptr && command->local_port == 3000,
         "expose should keep local port");
  expect(command != nullptr && command->remote_host == "myapp.example.com",
         "expose should keep remote host");
}

void rejects_invalid_expose_port() {
  const auto result = portbridge::parse_cli(
      {"expose", "--local", "70000", "--remote", "myapp.example.com"});

  expect(!result.ok(), "out-of-range local port should fail");
  expect(result.error == "--local must be a port between 1 and 65535",
         "invalid local port should explain the valid range");
}

}  // namespace

int main() {
  defaults_to_help();
  parses_version_command();
  parses_server_listen_port();
  rejects_server_without_port();
  parses_expose_command();
  rejects_invalid_expose_port();

  if (failures != 0) {
    std::cerr << failures << " test expectation(s) failed\n";
    return 1;
  }

  return 0;
}

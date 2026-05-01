#include "portbridge/cli.hpp"

#include <iostream>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
  std::vector<std::string_view> args;
  args.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));

  for (int index = 1; index < argc; ++index) {
    args.emplace_back(argv[index]);
  }

  const auto parsed = portbridge::parse_cli(args);
  const auto program_name = argc > 0 ? std::string_view{argv[0]} : std::string_view{"portbridge"};

  if (!parsed.ok()) {
    std::cerr << "error: " << parsed.error << "\n\n";
    std::cerr << portbridge::help_text(program_name);
    return 2;
  }

  const auto& command = *parsed.command;

  if (std::holds_alternative<portbridge::HelpCommand>(command)) {
    std::cout << portbridge::help_text(program_name);
    return 0;
  }

  if (std::holds_alternative<portbridge::VersionCommand>(command)) {
    std::cout << "portbridge " << portbridge::kVersion << '\n';
    return 0;
  }

  std::cout << portbridge::describe_command(command) << '\n';
  std::cout << "networking is not implemented in this step\n";
  return 0;
}

#pragma once

#include <string_view>

namespace workbench {

class Session;
struct Layout;

struct Status {
  std::string_view message;
  double expiresAt = 0;

  void show(std::string_view value);
  std::string_view current() const;
};

void handleInput(Session &session, const Layout &layout, Status &status);

} // namespace workbench

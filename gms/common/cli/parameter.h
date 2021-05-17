#pragma once

#include <memory>
#include <optional>

namespace GMS::CLI {
class ParamSpec {
public:
    std::string name;
    std::optional<std::string> alias;
    std::optional<std::string> default_value;
    std::string help;
    std::shared_ptr<std::string> value_ptr;

    ParamSpec() = default;

    ParamSpec(const std::string &name,
              const std::optional<std::string> &alias,
              const std::optional<std::string> &defaultValue,
              const std::string &help) :
            name(name), alias(alias), default_value(defaultValue), help(help) {
        if (alias == "") { this->alias.reset(); }
        if (defaultValue.has_value()) {
            value_ptr = std::make_shared<std::string>(defaultValue.value());
        } else {
            value_ptr = std::make_shared<std::string>();
        }
    }

    std::string &value() {
        return *value_ptr;
    }
};

class Param {
private:
    std::shared_ptr<std::string> value_ptr;

public:
    Param(const std::shared_ptr<std::string> &valuePtr) : value_ptr(valuePtr) {}

    const std::string &value() const {
        return *value_ptr;
    }

    int to_int() const {
        return std::stoi(value());
    }

    double to_double() const {
        return std::stod(value());
    }
};

} // namespace gms::cli
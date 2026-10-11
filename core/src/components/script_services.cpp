/**
 * @file script_services.cpp
 * @brief Definição do armazenamento estático de ScriptServices.
 */
#include "components/script_services.hpp"

namespace Yume {

IInput* ScriptServices::input = nullptr;
std::unordered_map<std::string, ScriptServices::InputAlias> ScriptServices::inputAliases;

void ScriptServices::configure(IInput* in, std::unordered_map<std::string, InputAlias> aliases) {
    input = in;
    inputAliases = std::move(aliases);
}

} // namespace Yume

#ifndef MODULE_H
#define MODULE_H

#include <string>

int load_module(const std::string& filename, const std::string& module_arguments) noexcept;
int unload_module(const std::string& name) noexcept;

#endif // MODULE_H

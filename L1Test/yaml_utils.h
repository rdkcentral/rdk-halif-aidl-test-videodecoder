#ifndef YAML_UTILS_H
#define YAML_UTILS_H

#include <yaml-cpp/yaml.h>
#include <vector>
#include <string>
#include <iostream>

// Function to get the count of elements in a list at the given path
int get_list_Count_from_profile(const YAML::Node& config, const std::string& path);

// Function to get an integer value from a YAML node based on a path
int get_integer_value_from_profile(const YAML::Node& config, const std::string& path);

// Function to extract a string value based on the path
std::string get_string_value_from_profile(const YAML::Node& config, const std::string& path);

// Function to extract a boolean value based on the path
bool get_boolean_value_from_profile(const YAML::Node& config, const std::string& path);

#endif

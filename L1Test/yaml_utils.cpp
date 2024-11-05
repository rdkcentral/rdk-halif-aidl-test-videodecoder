#include "yaml_utils.h"

// Helper function to split the path into segments
std::vector<std::string> splitPath(const std::string& path, char delimiter) {
    std::vector<std::string> segments;
    size_t start = 0;
    size_t end = path.find(delimiter);

    while (end != std::string::npos) {
        segments.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find(delimiter, start);
    }

    segments.push_back(path.substr(start));
    return segments;
}

// Function to get the count of elements in a list based on the path
int get_list_Count_from_profile(const YAML::Node& config, const std::string& path) {
    std::vector<std::string> segments = splitPath(path, '.');

    // Initial check for "components"
    if (segments.empty() || segments[0] != "components") {
        std::cerr << "Invalid path or incorrect format." << std::endl;
        return -1;
    }

    YAML::Node componentsNode = YAML::Clone(config["components"]);  // Clone the components node
    if (!componentsNode || !componentsNode.IsSequence()) {
        std::cerr << "components node not found or not a list." << std::endl;
        return -1;
    }

    // Search for the correct component based on the name (e.g., "IVideoDecoderManager")
    YAML::Node targetNode;
    bool found = false;
    for (std::size_t i = 0; i < componentsNode.size(); ++i) {
        YAML::Node component = componentsNode[i];
        if (component["name"] && component["name"].as<std::string>() == segments[1]) {
            targetNode = YAML::Clone(component);  // Clone target component
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Component with name " << segments[1] << " not found." << std::endl;
        return -1;
    }

    YAML::Node currentNode = targetNode;
    for (size_t i = 2; i < segments.size(); ++i) {
        const std::string& segment = segments[i];

        // Check if the segment is an index in the list
        try {
            int index = std::stoi(segment);
            if (currentNode.IsSequence() && index < currentNode.size()) {
                currentNode = YAML::Clone(currentNode[index]);  // Clone node before accessing index
            } else {
                std::cerr << "Invalid index or list out of bounds: " << index << std::endl;
                return -1;
            }
        } catch (std::invalid_argument&) {
            // If not an index, treat it as a key for a map
            if (currentNode.IsMap() && currentNode[segment]) {
                currentNode = YAML::Clone(currentNode[segment]);  // Clone node before accessing key
            } else {
                std::cerr << "Invalid path segment: " << segment << std::endl;
                return -1;
            }
        }
    }

    if (currentNode.IsSequence()) {
        return currentNode.size();  // Return the size if it's a sequence
    } else if (currentNode.IsScalar()) {
        //std::cout << "Reached scalar value: " << currentNode.as<std::string>() << std::endl;
        return 1;  // Return 1 for a scalar value
    } else {
        std::cerr << "List or scalar not found at the given path." << std::endl;
        return -1;
    }
}

// Function to extract an integer value based on the path
int get_integer_value_from_profile(const YAML::Node& config, const std::string& path) {
    std::vector<std::string> segments = splitPath(path, '.');

    // Initial check for "components"
    if (segments.empty() || segments[0] != "components") {
        std::cerr << "Invalid path or incorrect format." << std::endl;
        return -1;
    }

    YAML::Node componentsNode = YAML::Clone(config["components"]);  // Clone the components node
    if (!componentsNode || !componentsNode.IsSequence()) {
        std::cerr << "components node not found or not a list." << std::endl;
        return -1;
    }

    // Search for the correct component based on the name (e.g., "IVideoDecoderManager")
    YAML::Node targetNode;
    bool found = false;
    for (std::size_t i = 0; i < componentsNode.size(); ++i) {
        YAML::Node component = componentsNode[i];
        if (component["name"] && component["name"].as<std::string>() == segments[1]) {
            targetNode = YAML::Clone(component);  // Clone target component
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Component with name " << segments[1] << " not found." << std::endl;
        return -1;
    }

    YAML::Node currentNode = targetNode;
    for (size_t i = 2; i < segments.size(); ++i) {
        const std::string& segment = segments[i];

        // Check if the segment is an index in the list
        try {
            int index = std::stoi(segment);
            if (currentNode.IsSequence() && index < currentNode.size()) {
                currentNode = YAML::Clone(currentNode[index]);  // Clone node before accessing index
            } else {
                std::cerr << "Invalid index or list out of bounds: " << index << std::endl;
                return -1;
            }
        } catch (std::invalid_argument&) {
            // If not an index, treat it as a key for a map
            if (currentNode.IsMap() && currentNode[segment]) {
                currentNode = YAML::Clone(currentNode[segment]);  // Clone node before accessing key
            } else {
                std::cerr << "Invalid path segment: " << segment << std::endl;
                return -1;
            }
        }
    }

    if (currentNode.IsScalar() && currentNode.IsDefined()) {
        try {
            return currentNode.as<int>();  // Return the integer value
        } catch (const YAML::BadConversion& e) {
            std::cerr << "The value at the path is not an integer." << std::endl;
            return -1;
        }
    } else {
        std::cerr << "No scalar value found at the given path." << std::endl;
        return -1;
    }
}

// Function to extract a string value based on the path
std::string get_string_value_from_profile(const YAML::Node& config, const std::string& path) {
    std::vector<std::string> segments = splitPath(path, '.');

    // Initial check for "components"
    if (segments.empty() || segments[0] != "components") {
        std::cerr << "Invalid path or incorrect format." << std::endl;
        return "";
    }

    YAML::Node componentsNode = YAML::Clone(config["components"]);  // Clone the components node
    if (!componentsNode || !componentsNode.IsSequence()) {
        std::cerr << "components node not found or not a list." << std::endl;
        return "";
    }

    // Search for the correct component based on the name (e.g., "IVideoDecoderManager")
    YAML::Node targetNode;
    bool found = false;
    for (std::size_t i = 0; i < componentsNode.size(); ++i) {
        YAML::Node component = componentsNode[i];
        if (component["name"] && component["name"].as<std::string>() == segments[1]) {
            targetNode = YAML::Clone(component);  // Clone target component
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Component with name " << segments[1] << " not found." << std::endl;
        return "";
    }

    YAML::Node currentNode = targetNode;
    for (size_t i = 2; i < segments.size(); ++i) {
        const std::string& segment = segments[i];

        // Check if the segment is an index in the list
        try {
            int index = std::stoi(segment);
            if (currentNode.IsSequence() && index < currentNode.size()) {
                currentNode = YAML::Clone(currentNode[index]);  // Clone node before accessing index
            } else {
                std::cerr << "Invalid index or list out of bounds: " << index << std::endl;
                return "";
            }
        } catch (std::invalid_argument&) {
            // If not an index, treat it as a key for a map
            if (currentNode.IsMap() && currentNode[segment]) {
                currentNode = YAML::Clone(currentNode[segment]);  // Clone node before accessing key
            } else {
                std::cerr << "Invalid path segment: " << segment << std::endl;
                return "";
            }
        }
    }

    if (currentNode.IsScalar() && currentNode.IsDefined()) {
        try {
            return currentNode.as<std::string>();  // Return the string value
        } catch (const YAML::BadConversion& e) {
            std::cerr << "The value at the path is not a string." << std::endl;
            return "";
        }
    } else {
        std::cerr << "No scalar value found at the given path." << std::endl;
        return "";
    }
}

// Function to extract a boolean value based on the path
bool get_boolean_value_from_profile(const YAML::Node& config, const std::string& path) {
    std::vector<std::string> segments = splitPath(path, '.');

    // Initial check for "components"
    if (segments.empty() || segments[0] != "components") {
        std::cerr << "Invalid path or incorrect format." << std::endl;
        return false;
    }

    YAML::Node componentsNode = YAML::Clone(config["components"]);  // Clone the components node
    if (!componentsNode || !componentsNode.IsSequence()) {
        std::cerr << "components node not found or not a list." << std::endl;
        return false;
    }

    // Search for the correct component based on the name (e.g., "IVideoDecoderManager")
    YAML::Node targetNode;
    bool found = false;
    for (std::size_t i = 0; i < componentsNode.size(); ++i) {
        YAML::Node component = componentsNode[i];
        if (component["name"] && component["name"].as<std::string>() == segments[1]) {
            targetNode = YAML::Clone(component);  // Clone target component
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Component with name " << segments[1] << " not found." << std::endl;
        return false;
    }

    YAML::Node currentNode = targetNode;
    for (size_t i = 2; i < segments.size(); ++i) {
        const std::string& segment = segments[i];

        // Check if the segment is an index in the list
        try {
            int index = std::stoi(segment);
            if (currentNode.IsSequence() && index < currentNode.size()) {
                currentNode = YAML::Clone(currentNode[index]);  // Clone node before accessing index
            } else {
                std::cerr << "Invalid index or list out of bounds: " << index << std::endl;
                return false;
            }
        } catch (std::invalid_argument&) {
            // If not an index, treat it as a key for a map
            if (currentNode.IsMap() && currentNode[segment]) {
                currentNode = YAML::Clone(currentNode[segment]);  // Clone node before accessing key
            } else {
                std::cerr << "Invalid path segment: " << segment << std::endl;
                return false;
            }
        }
    }

    if (currentNode.IsScalar() && currentNode.IsDefined()) {
        try {
            return currentNode.as<bool>();  // Return the boolean value
        } catch (const YAML::BadConversion& e) {
            std::cerr << "The value at the path is not a boolean." << std::endl;
            return false;
        }
    } else {
        std::cerr << "No scalar value found at the given path." << std::endl;
        return false;
    }
}

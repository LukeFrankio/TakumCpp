/**
 * @file runtime_config.h
 * @brief Runtime configuration system for TakumCpp.
 *
 * This header provides a modern configuration system that replaces compile-time
 * macros with runtime-configurable options. This enables:
 * - Dynamic strategy selection based on workload characteristics
 * - A/B testing of different optimization approaches
 * - Fine-tuning for specific hardware architectures
 * - Better integration with profiling and benchmarking tools
 */
#pragma once

#include <string>
#include <unordered_map>
#include <type_traits>
#include <memory>
#include <mutex>
#include <functional>

namespace takum {
namespace config {

/**
 * @brief Configuration option types supported by the system.
 */
enum class option_type {
    boolean,
    integer,
    floating_point,
    string,
    strategy_selector
};

/**
 * @brief Base class for configuration values.
 */
class config_value {
public:
    virtual ~config_value() = default;
    virtual option_type type() const noexcept = 0;
    virtual std::string to_string() const = 0;
    virtual void from_string(const std::string& value) = 0;
    virtual std::unique_ptr<config_value> clone() const = 0;
};

/**
 * @brief Template for typed configuration values.
 */
template<typename T>
class typed_config_value : public config_value {
public:
    explicit typed_config_value(T default_value) : value_(default_value) {}
    
    T get() const { return value_; }
    void set(T new_value) { value_ = new_value; }
    
    option_type type() const noexcept override {
        if constexpr (std::is_same_v<T, bool>) {
            return option_type::boolean;
        } else if constexpr (std::is_integral_v<T>) {
            return option_type::integer;
        } else if constexpr (std::is_floating_point_v<T>) {
            return option_type::floating_point;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return option_type::string;
        } else {
            return option_type::strategy_selector;
        }
    }
    
    std::string to_string() const override;
    void from_string(const std::string& str) override;
    std::unique_ptr<config_value> clone() const override {
        return std::make_unique<typed_config_value<T>>(value_);
    }
    
private:
    T value_;
};

/**
 * @brief Validation function type for configuration values.
 */
using validator_func = std::function<bool(const config_value&)>;

/**
 * @brief Configuration option descriptor.
 */
struct config_option {
    std::string name;
    std::string description;
    std::unique_ptr<config_value> default_value;
    validator_func validator;
    bool is_runtime_configurable;
    
    config_option(std::string name, std::string description,
                 std::unique_ptr<config_value> default_val,
                 validator_func validator = nullptr,
                 bool runtime_configurable = true)
        : name(std::move(name))
        , description(std::move(description))
        , default_value(std::move(default_val))
        , validator(std::move(validator))
        , is_runtime_configurable(runtime_configurable) {}
};

/**
 * @brief Main configuration manager.
 *
 * This class provides a centralized configuration system that replaces
 * the previous macro-based approach. It supports:
 * - Type-safe configuration values
 * - Runtime validation
 * - Thread-safe access
 * - Configuration persistence
 * - Environment variable integration
 */
class configuration_manager {
public:
    /**
     * @brief Get the global configuration manager instance.
     */
    static configuration_manager& instance();
    
    /**
     * @brief Register a configuration option.
     */
    void register_option(config_option option);
    
    /**
     * @brief Get a configuration value.
     */
    template<typename T>
    T get(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = values_.find(name);
        if (it == values_.end()) {
            auto opt_it = options_.find(name);
            if (opt_it != options_.end()) {
                auto* typed_val = dynamic_cast<typed_config_value<T>*>(opt_it->second.default_value.get());
                if (typed_val) {
                    return typed_val->get();
                }
            }
            throw std::runtime_error("Unknown configuration option: " + name);
        }
        
        auto* typed_val = dynamic_cast<typed_config_value<T>*>(it->second.get());
        if (!typed_val) {
            throw std::runtime_error("Type mismatch for configuration option: " + name);
        }
        
        return typed_val->get();
    }
    
    /**
     * @brief Set a configuration value.
     */
    template<typename T>
    void set(const std::string& name, T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto opt_it = options_.find(name);
        if (opt_it == options_.end()) {
            throw std::runtime_error("Unknown configuration option: " + name);
        }
        
        if (!opt_it->second.is_runtime_configurable) {
            throw std::runtime_error("Configuration option is not runtime configurable: " + name);
        }
        
        auto new_val = std::make_unique<typed_config_value<T>>(value);
        
        if (opt_it->second.validator && !opt_it->second.validator(*new_val)) {
            throw std::runtime_error("Validation failed for configuration option: " + name);
        }
        
        values_[name] = std::move(new_val);
    }
    
    /**
     * @brief Load configuration from environment variables.
     */
    void load_from_environment();
    
    /**
     * @brief Load configuration from file.
     */
    void load_from_file(const std::string& filename);
    
    /**
     * @brief Save configuration to file.
     */
    void save_to_file(const std::string& filename) const;
    
    /**
     * @brief Get all configuration options.
     */
    std::vector<std::string> list_options() const;
    
    /**
     * @brief Get option description.
     */
    std::string get_description(const std::string& name) const;
    
    /**
     * @brief Reset option to default value.
     */
    void reset_to_default(const std::string& name);
    
    /**
     * @brief Reset all options to default values.
     */
    void reset_all_to_defaults();
    
private:
    configuration_manager() { register_default_options(); }
    
    void register_default_options();
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, config_option> options_;
    std::unordered_map<std::string, std::unique_ptr<config_value>> values_;
};

/**
 * @brief Convenience functions for common configuration options.
 */
namespace options {

// Φ evaluation strategy selection
inline std::string phi_strategy() {
    return configuration_manager::instance().get<std::string>("phi_strategy");
}

inline void set_phi_strategy(const std::string& strategy) {
    configuration_manager::instance().set("phi_strategy", strategy);
}

// LUT size configuration
inline size_t coarse_lut_size() {
    return configuration_manager::instance().get<size_t>("coarse_lut_size");
}

inline void set_coarse_lut_size(size_t size) {
    configuration_manager::instance().set("coarse_lut_size", size);
}

// Cubic interpolation toggle
inline bool enable_cubic_interpolation() {
    return configuration_manager::instance().get<bool>("enable_cubic_interpolation");
}

inline void set_enable_cubic_interpolation(bool enable) {
    configuration_manager::instance().set("enable_cubic_interpolation", enable);
}

// Performance diagnostics
inline bool enable_phi_diagnostics() {
    return configuration_manager::instance().get<bool>("enable_phi_diagnostics");
}

inline void set_enable_phi_diagnostics(bool enable) {
    configuration_manager::instance().set("enable_phi_diagnostics", enable);
}

// Fast addition heuristics
inline bool enable_fast_add() {
    return configuration_manager::instance().get<bool>("enable_fast_add");
}

inline void set_enable_fast_add(bool enable) {
    configuration_manager::instance().set("enable_fast_add", enable);
}

} // namespace options

/**
 * @brief RAII configuration scope for temporary settings.
 */
class config_scope {
public:
    template<typename T>
    config_scope(const std::string& name, T temporary_value) : name_(name) {
        auto& manager = configuration_manager::instance();
        
        // Save current value
        try {
            saved_value_ = std::make_unique<typed_config_value<T>>(manager.get<T>(name));
        } catch (...) {
            // Option might not be set, use default
            saved_value_ = nullptr;
        }
        
        // Set temporary value
        manager.set(name, temporary_value);
    }
    
    ~config_scope() {
        if (saved_value_) {
            auto& manager = configuration_manager::instance();
            // Restore saved value
            manager.reset_to_default(name_);
            // TODO: Properly restore the saved value
        }
    }
    
    config_scope(const config_scope&) = delete;
    config_scope& operator=(const config_scope&) = delete;
    
private:
    std::string name_;
    std::unique_ptr<config_value> saved_value_;
};

} // namespace config
} // namespace takum
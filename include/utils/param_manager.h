// param_manager.h
#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace utils
{

class ParamManager
{
private:
  class ParamHandlerBase
  {
  public:
    virtual ~ParamHandlerBase() = default;
    virtual void declare(rclcpp::Node * node) = 0;
    virtual void update(
      const rcl_interfaces::msg::Parameter & param) = 0;
  };

  template <typename T>
  class ParamHandler : public ParamHandlerBase
  {
  public:
    ParamHandler(
      const std::string & name, T & storage_ref,
      const T & default_value)
    : name_(name),
      storage_ref_(storage_ref),
      default_value_(default_value)
    {
    }

    void declare(rclcpp::Node * node) override
    {
      if (!node) return;
      node_ = node;

      try {
        storage_ref_ =
          node->declare_parameter<T>(name_, default_value_);
        RCLCPP_DEBUG(
          node->get_logger(), "Declared parameter: %s = %s",
          name_.c_str(), toString(storage_ref_).c_str());
      } catch (const std::exception & e) {
        RCLCPP_ERROR(
          node->get_logger(), "Failed to declare %s: %s",
          name_.c_str(), e.what());
        throw;
      }
    }

    void update(const rcl_interfaces::msg::Parameter & param) override
    {
      try {
        rclcpp::Parameter rclcpp_param =
          rclcpp::Parameter::from_parameter_msg(param);
        T new_value = rclcpp_param.get_value<T>();
        T old_value = storage_ref_;
        storage_ref_ = new_value;

        if (node_) {
          RCLCPP_INFO(
            node_->get_logger(), "Updated %s = %s", name_.c_str(),
            toString(new_value).c_str());
        }

        if (callback_) {
          callback_(old_value, new_value);
        }
      } catch (const std::exception & e) {
        if (node_) {
          RCLCPP_ERROR(
            node_->get_logger(), "Failed to update %s: %s",
            name_.c_str(), e.what());
        }
      }
    }

    void setCallback(
      std::function<void(const T &, const T &)> callback)
    {
      callback_ = std::move(callback);
    }

  private:
    template <typename U>
    std::string toString(const U & value)
    {
      if constexpr (std::is_same_v<U, bool>) {
        return value ? "true" : "false";
      } else if constexpr (std::is_arithmetic_v<U>) {
        return std::to_string(value);
      } else if constexpr (std::is_same_v<U, std::string>) {
        return value;
      } else {
        return "[complex type]";
      }
    }

    std::string name_;
    T & storage_ref_;
    T default_value_;
    rclcpp::Node * node_ = nullptr;
    std::function<void(const T &, const T &)> callback_;
  };

public:
  static ParamManager & getInstance()
  {
    static ParamManager instance;
    return instance;
  }

  // Prohibit copying and moving the singleton instance
  ParamManager(const ParamManager &) = delete;
  ParamManager & operator=(const ParamManager &) = delete;
  ParamManager(ParamManager &&) = delete;
  ParamManager & operator=(ParamManager &&) = delete;

  ~ParamManager() { reset(); }

  void init(rclcpp::Node * node)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (node_) {
      RCLCPP_WARN(
        node->get_logger(), "ParamManager already initialized");
      return;
    }

    node_ = node;

    for (auto & [name, handler] : handlers_) {
      handler->declare(node_);
    }

    setupParameterCallback();

    RCLCPP_INFO(
      node_->get_logger(),
      "ParamManager initialized with %lu parameters",
      handlers_.size());
  }

  void reset()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    node_ = nullptr;
    param_event_handler_.reset();
    callback_handle_.reset();
    handlers_.clear();
  }

  template <typename T>
  T & registerParam(
    const std::string & name, const T & default_value,
    T & storage_ref)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    if (handlers_.find(name) != handlers_.end()) {
      throw std::runtime_error(
        "Parameter already registered: " + name);
    }

    auto handler = std::make_unique<ParamHandler<T>>(
      name, storage_ref, default_value);

    if (node_) {
      handler->declare(node_);
    }

    handlers_[name] = std::move(handler);

    return storage_ref;
  }

  template <typename T>
  void setParamCallback(
    const std::string & name,
    std::function<void(const T &, const T &)> callback)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = handlers_.find(name);
    if (it == handlers_.end()) {
      throw std::runtime_error("Parameter not found: " + name);
    }

    auto * handler =
      dynamic_cast<ParamHandler<T> *>(it->second.get());
    if (!handler) {
      throw std::runtime_error(
        "Type mismatch for parameter: " + name);
    }

    handler->setCallback(std::move(callback));
  }

private:
  ParamManager() : node_(nullptr) {}

  void setupParameterCallback()
  {
    if (!node_) return;

    param_event_handler_ =
      std::make_shared<rclcpp::ParameterEventHandler>(node_);

    auto callback =
      [this](const rcl_interfaces::msg::ParameterEvent & event) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto params = event.changed_parameters;
        if (node_) {
          RCLCPP_DEBUG(
            node_->get_logger(),
            "Parameter event: %zu changed, %zu new, %zu deleted",
            params.size(), event.new_parameters.size(),
            event.deleted_parameters.size());
          for (const auto & param : params) {
            RCLCPP_DEBUG(
              node_->get_logger(), "  - %s (type: %d)",
              param.name.c_str(), param.value.type);
          }
        }
        for (const auto & param : params) {
          auto it = handlers_.find(param.name);
          if (it != handlers_.end()) {
            it->second->update(param);
          } else {
            if (node_) {
              RCLCPP_DEBUG(
                node_->get_logger(), "No handler for parameter: %s",
                param.name.c_str());
            }
          }
        }
      };

    callback_handle_ =
      param_event_handler_->add_parameter_event_callback(callback);
  }

  rclcpp::Node * node_;
  std::unordered_map<std::string, std::unique_ptr<ParamHandlerBase>>
    handlers_;
  std::shared_ptr<rclcpp::ParameterEventHandler> param_event_handler_;
  rclcpp::ParameterEventCallbackHandle::SharedPtr callback_handle_;
  mutable std::mutex mutex_;
};

// ============================================================================
// macro definition
// ============================================================================

// define a parameter and register it
#define DEFINE_PARAM(type, name, default_val)               \
  type param_##name##_storage_ = default_val;               \
  type & name =                                             \
    utils::ParamManager::getInstance().registerParam<type>( \
      #name, default_val, param_##name##_storage_)

// define a parameter with namespace and register it
#define DEFINE_PARAM_NS(ns, type, name, default_val)        \
  type param_##ns##_##name##_storage_ = default_val;        \
  type & name =                                             \
    utils::ParamManager::getInstance().registerParam<type>( \
      #ns "." #name, default_val, param_##ns##_##name##_storage_)

// define a parameter and register it
#define DEFINE_PARAM_INLINE(type, name, default_val)        \
  inline type param_##name##_storage_ = default_val;        \
  inline type & name =                                      \
    utils::ParamManager::getInstance().registerParam<type>( \
      #name, default_val, param_##name##_storage_)

// define a parameter with namespace and register it
#define DEFINE_PARAM_NS_INLINE(ns, type, name, default_val) \
  inline type param_##ns##_##name##_storage_ = default_val; \
  inline type & name =                                      \
    utils::ParamManager::getInstance().registerParam<type>( \
      #ns "." #name, default_val, param_##ns##_##name##_storage_)

// set a callback for a parameter
#define SET_PARAM_CALLBACK(type, name, callback)             \
  utils::ParamManager::getInstance().setParamCallback<type>( \
    #name, callback)

// set a callback for a parameter with namespace
#define SET_PARAM_CALLBACK_NS(ns, type, name, callback)      \
  utils::ParamManager::getInstance().setParamCallback<type>( \
    #ns "." #name, callback)

}  // namespace utils
// param_manager.h
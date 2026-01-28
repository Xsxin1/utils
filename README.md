# ParamManager - ROS 2 参数管理库

ParamManager 是一个用于 ROS 2 的 C++ 参数管理库，提供类型安全、线程安全的参数注册、声明和动态更新功能。它简化了 ROS 2 参数处理，支持自动参数声明、运行时更新回调以及便捷的宏定义。

## 特性

- **单例模式**：全局唯一的参数管理器实例
- **类型安全**：支持多种数据类型（int、double、bool、string、vector 等）
- **线程安全**：使用互斥锁保护内部数据结构
- **延迟初始化**：可在节点初始化前注册参数
- **动态更新**：自动监听参数变化并更新存储变量
- **便捷宏**：提供 `DEFINE_PARAM` 和 `DEFINE_PARAM_NS` 宏简化参数定义
- **完整测试**：包含全面的单元测试

## 依赖

- ROS 2 (Humble 或更高版本)
- GTest (用于测试)

## 使用方法

### 1. 基本使用

```cpp
#include "utils/param_manager.h"

// 在类或全局作用域中定义参数
DEFINE_PARAM(double, max_velocity, 2.0);
DEFINE_PARAM(bool, enable_safety, true);
DEFINE_PARAM_NS(controller, double, kp, 1.0);


int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("my_node");
    
    // 使用裸指针初始化 ParamManager
    auto& manager = param::ParamManager::getInstance();
    manager.init(node.get());
    
    // 使用参数
    RCLCPP_INFO(node->get_logger(), "Max velocity: %f", max_velocity);
    
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
```

## API 参考

### 单例访问

```cpp
param::ParamManager& manager = param::ParamManager::getInstance();
```

### 主要方法

#### `void init(rclcpp::Node* node)`
初始化参数管理器并绑定到 ROS 2 节点。

#### `~ParamManager()`
析构函数，自动调用 `reset()` 清理资源。确保在程序退出时正确释放所有内部资源。

#### `void reset()`
重置参数管理器，清除所有注册的参数和节点绑定。主要用于测试。

#### `template <typename T> T& registerParam(const std::string& name, const T& default_value, T& storage_ref)`
注册一个新参数。

- `name`: 参数名称（ROS 2 参数服务器中的名称）
- `default_value`: 默认值
- `storage_ref`: 存储变量的引用，参数值将存储于此
- 返回：存储变量的引用（与 `storage_ref` 相同）

#### 宏定义

##### `DEFINE_PARAM(type, name, default_val)`
定义并注册一个参数。

- `type`: 参数类型（如 `int`、`double`、`bool`、`std::string` 等）
- `name`: 参数变量名
- `default_val`: 默认值

示例：
```cpp
DEFINE_PARAM(double, max_speed, 1.5);
// 创建变量 max_speed，可通过 max_speed 访问
```

##### `DEFINE_PARAM_NS(ns, type, name, default_val)`
定义并注册一个带命名空间的参数。

- `ns`: 命名空间前缀
- `type`: 参数类型
- `name`: 参数变量名
- `default_val`: 默认值

示例：
```cpp
DEFINE_PARAM_NS(robot, double, velocity, 1.0);
// 创建变量 velocity，对应 ROS 参数名为 "robot.velocity"
```

## 支持的数据类型

ParamManager 支持所有 ROS 2 参数类型：

- 基本类型：`bool`、`int`、`double`、`std::string`
- 数组类型：`std::vector<bool>`、`std::vector<int>`、`std::vector<double>`、`std::vector<std::string>`

## 线程安全

所有公共方法都是线程安全的，使用内部互斥锁保护。可以从多个线程安全地注册参数和调用 `init()`。

## 错误处理

- 重复注册同名参数会抛出 `std::runtime_error`
- 参数声明失败会记录错误并抛出异常
- 参数更新失败会记录错误但不会抛出异常（避免影响其他参数更新）

## 示例

参考 `test/test_param_manager.cpp` 中的完整测试用例，展示了各种使用场景：

1. 基本参数注册和更新
2. 宏定义使用
3. 线程安全测试
4. 完整工作流示例

## 构建和测试

### 构建

```bash
cd /path/to/workspace
colcon build --packages-select utils
```

### 运行测试

```bash
cd /path/to/workspace
colcon test --packages-select utils
```

## 设计说明

### 内部架构

ParamManager 使用模板方法模式，通过 `ParamHandlerBase` 和 `ParamHandler<T>` 处理不同类型参数。每个参数处理器负责：

1. 在 ROS 2 节点上声明参数
2. 监听参数更新事件
3. 更新存储变量

### 参数更新机制

1. 通过 `rclcpp::ParameterEventHandler` 监听参数事件
2. 当参数变化时，查找对应的参数处理器
3. 调用处理器的 `update()` 方法更新存储变量
4. 记录更新日志

## 限制

- 参数必须在节点运行前注册（或至少在参数服务器查询前）
- 不支持嵌套命名空间（仅支持单层命名空间）
- 不支持动态类型变更（参数类型在注册时确定）

## 贡献

欢迎提交 Issue 和 Pull Request。

## 许可证

BSD-3-Clause


## 作者

- xsxin (xsxin123@163.com)
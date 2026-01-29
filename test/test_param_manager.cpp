// test_param_manager.cpp
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <thread>

// Include the header file to test
#include "utils/param_manager.h"

using namespace std::chrono_literals;

/**
 * Test fixture for ParamManager tests
 * Provides a clean ROS2 node for each test
 */
class ParamManagerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rclcpp::init(0, nullptr);
    node_ = std::make_shared<rclcpp::Node>("test_param_manager_node");
  }

  void TearDown() override
  {
    // Reset ParamManager to avoid stale node pointer across tests
    utils::ParamManager::getInstance().reset();
    node_.reset();
    rclcpp::shutdown();
  }

  std::shared_ptr<rclcpp::Node> node_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(ParamManagerTest, SingletonInstance)
{
  // Test that getInstance returns the same instance
  auto & instance1 = utils::ParamManager::getInstance();
  auto & instance2 = utils::ParamManager::getInstance();

  EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ParamManagerTest, InitializeWithNode)
{
  auto & manager = utils::ParamManager::getInstance();

  // Should initialize without throwing
  EXPECT_NO_THROW(manager.init(node_.get()));
}

TEST_F(ParamManagerTest, InitializeMultipleTimes)
{
  auto & manager = utils::ParamManager::getInstance();

  manager.init(node_.get());

  // Second initialization should not throw, just warn
  EXPECT_NO_THROW(manager.init(node_.get()));
}

// ============================================================================
// Parameter Registration Tests
// ============================================================================

TEST_F(ParamManagerTest, RegisterIntParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  int storage = 0;
  int default_value = 42;

  auto & param_ref =
    manager.registerParam<int>("test_int", default_value, storage);

  manager.init(node_.get());

  // Check that storage is updated to default value
  EXPECT_EQ(storage, default_value);
  EXPECT_EQ(param_ref, default_value);
  EXPECT_EQ(&param_ref, &storage);
}

TEST_F(ParamManagerTest, RegisterDoubleParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  double storage = 0.0;
  double default_value = 3.14159;

  auto & param_ref = manager.registerParam<double>(
    "test_double", default_value, storage);

  manager.init(node_.get());

  EXPECT_NEAR(storage, default_value, 0.00001);
  EXPECT_NEAR(param_ref, default_value, 0.00001);
}

TEST_F(ParamManagerTest, RegisterBoolParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  bool storage = false;
  bool default_value = true;

  auto & param_ref =
    manager.registerParam<bool>("test_bool", default_value, storage);

  manager.init(node_.get());

  EXPECT_EQ(storage, default_value);
  EXPECT_EQ(param_ref, default_value);
}

TEST_F(ParamManagerTest, RegisterStringParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  std::string storage = "";
  std::string default_value = "test_string_value";

  auto & param_ref = manager.registerParam<std::string>(
    "test_string", default_value, storage);

  manager.init(node_.get());

  EXPECT_EQ(storage, default_value);
  EXPECT_EQ(param_ref, default_value);
}

TEST_F(ParamManagerTest, RegisterVectorParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  std::vector<double> storage;
  std::vector<double> default_value = {1.0, 2.0, 3.0};

  auto & param_ref = manager.registerParam<std::vector<double>>(
    "test_vector", default_value, storage);

  manager.init(node_.get());

  ASSERT_EQ(storage.size(), default_value.size());
  for (size_t i = 0; i < storage.size(); ++i) {
    EXPECT_DOUBLE_EQ(storage[i], default_value[i]);
  }
}

TEST_F(ParamManagerTest, RegisterMultipleParameters)
{
  auto & manager = utils::ParamManager::getInstance();

  int int_storage = 0;
  double double_storage = 0.0;
  bool bool_storage = false;

  manager.registerParam<int>("param1", 10, int_storage);
  manager.registerParam<double>("param2", 2.5, double_storage);
  manager.registerParam<bool>("param3", true, bool_storage);

  manager.init(node_.get());
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(int_storage, 10);
  EXPECT_DOUBLE_EQ(double_storage, 2.5);
  EXPECT_EQ(bool_storage, true);
}

TEST_F(ParamManagerTest, RegisterDuplicateParameterThrows)
{
  auto & manager = utils::ParamManager::getInstance();

  int storage1 = 0;
  int storage2 = 0;

  manager.registerParam<int>("duplicate_param", 10, storage1);

  // Registering the same parameter name again should throw
  EXPECT_THROW(
    manager.registerParam<int>("duplicate_param", 20, storage2),
    std::runtime_error);
}

// ============================================================================
// Parameter Update Tests
// ============================================================================

TEST_F(ParamManagerTest, UpdateIntParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  int storage = 0;
  manager.registerParam<int>("update_test_int", 10, storage);
  manager.init(node_.get());

  EXPECT_EQ(storage, 10);

  // Update parameter
  node_->set_parameter(rclcpp::Parameter("update_test_int", 42));

  // Give time for callback to process
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(storage, 42);
}

TEST_F(ParamManagerTest, UpdateDoubleParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  double storage = 0.0;
  manager.registerParam<double>("update_test_double", 1.0, storage);
  manager.init(node_.get());

  EXPECT_DOUBLE_EQ(storage, 1.0);

  // Update parameter
  node_->set_parameter(
    rclcpp::Parameter("update_test_double", 2.718));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_DOUBLE_EQ(storage, 2.718);
}

TEST_F(ParamManagerTest, UpdateBoolParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  bool storage = false;
  manager.registerParam<bool>("update_test_bool", false, storage);
  manager.init(node_.get());

  EXPECT_FALSE(storage);

  // Update parameter
  node_->set_parameter(rclcpp::Parameter("update_test_bool", true));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(storage);
}

TEST_F(ParamManagerTest, UpdateStringParameter)
{
  auto & manager = utils::ParamManager::getInstance();

  std::string storage = "";
  manager.registerParam<std::string>(
    "update_test_string", "initial", storage);
  manager.init(node_.get());

  EXPECT_EQ(storage, "initial");

  // Update parameter
  node_->set_parameter(
    rclcpp::Parameter("update_test_string", "updated"));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(storage, "updated");
}

TEST_F(ParamManagerTest, UpdateMultipleParameters)
{
  auto & manager = utils::ParamManager::getInstance();

  int int_storage = 0;
  double double_storage = 0.0;
  bool bool_storage = false;

  manager.registerParam<int>("multi_update_int", 1, int_storage);
  manager.registerParam<double>(
    "multi_update_double", 1.0, double_storage);
  manager.registerParam<bool>(
    "multi_update_bool", true, bool_storage);
  manager.init(node_.get());

  std::this_thread::sleep_for(100ms);

  auto results = node_->set_parameters(
    {rclcpp::Parameter("multi_update_int", 100),
     rclcpp::Parameter("multi_update_double", 3.14),
     rclcpp::Parameter("multi_update_bool", false)});

  ASSERT_TRUE(results[0].successful);
  ASSERT_TRUE(results[1].successful);
  ASSERT_TRUE(results[2].successful);

  auto start = std::chrono::steady_clock::now();
  bool all_updated = false;

  while (!all_updated && std::chrono::steady_clock::now() - start <
                           std::chrono::seconds(2)) {
    rclcpp::spin_some(node_);
    std::this_thread::sleep_for(10ms);

    all_updated = (int_storage == 100) &&
                  (std::abs(double_storage - 3.14) < 1e-9) &&
                  (bool_storage == false);
  }

  EXPECT_EQ(int_storage, 100);
  EXPECT_NEAR(double_storage, 3.14, 1e-9);
  EXPECT_FALSE(bool_storage);
}

// ============================================================================
// Macro Tests
// ============================================================================

TEST_F(ParamManagerTest, DefineParamMacro)
{
  DEFINE_PARAM(int, test_macro_param, 99);

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  EXPECT_EQ(test_macro_param, 99);

  // Update and verify
  node_->set_parameter(rclcpp::Parameter("test_macro_param", 200));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(test_macro_param, 200);
}

TEST_F(ParamManagerTest, DefineParamNsMacro)
{
  DEFINE_PARAM_NS(robot, double, velocity, 1.5);

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  EXPECT_DOUBLE_EQ(velocity, 1.5);

  // Update with namespaced parameter
  node_->set_parameter(rclcpp::Parameter("robot.velocity", 2.5));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_DOUBLE_EQ(velocity, 2.5);
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

TEST_F(ParamManagerTest, RegisterBeforeInit)
{
  auto & manager = utils::ParamManager::getInstance();

  int storage = 0;

  // Register parameter before initialization
  EXPECT_NO_THROW(
    manager.registerParam<int>("before_init", 5, storage));

  // Storage should still have default value
  EXPECT_EQ(storage, 0);

  // Now initialize
  manager.init(node_.get());

  // Storage should be updated after init
  EXPECT_EQ(storage, 5);
}

TEST_F(ParamManagerTest, RegisterAfterInit)
{
  auto & manager = utils::ParamManager::getInstance();

  manager.init(node_.get());

  int storage = 0;

  // Register parameter after initialization
  EXPECT_NO_THROW(
    manager.registerParam<int>("after_init", 7, storage));

  // Storage should be immediately updated
  EXPECT_EQ(storage, 7);
}

TEST_F(ParamManagerTest, ThreadSafety)
{
  auto & manager = utils::ParamManager::getInstance();

  std::vector<int> storages(10, 0);

  // Register multiple parameters from different threads
  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&manager, &storages, i]() {
      manager.registerParam<int>(
        "thread_param_" + std::to_string(i), i, storages[i]);
    });
  }

  for (auto & thread : threads) {
    thread.join();
  }

  manager.init(node_.get());

  // Verify all parameters were registered correctly
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(storages[i], i);
  }
}

TEST_F(ParamManagerTest, ReferenceStability)
{
  auto & manager = utils::ParamManager::getInstance();

  int storage = 0;
  auto & ref = manager.registerParam<int>("ref_test", 5, storage);

  manager.init(node_.get());

  // Verify that the reference points to storage
  EXPECT_EQ(&ref, &storage);

  // Update via node
  node_->set_parameter(rclcpp::Parameter("ref_test", 10));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // Both should be updated
  EXPECT_EQ(storage, 10);
  EXPECT_EQ(ref, 10);
  EXPECT_EQ(&ref, &storage);
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_F(ParamManagerTest, LambdaCallback)
{
  int callback_invoked = 0;
  double old_val = 0.0;
  double new_val = 0.0;

  DEFINE_PARAM(double, max_speed, 10.0);

  // 设置回调
  SET_PARAM_CALLBACK(
    double, max_speed,
    [&](const double & old_v, const double & new_v) {
      callback_invoked++;
      old_val = old_v;
      new_val = new_v;
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 修改参数
  node_->set_parameter(rclcpp::Parameter("max_speed", 20.0));

  // 等待回调执行
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 验证回调被调用
  EXPECT_EQ(callback_invoked, 1);
  EXPECT_DOUBLE_EQ(old_val, 10.0);
  EXPECT_DOUBLE_EQ(new_val, 20.0);
  EXPECT_DOUBLE_EQ(max_speed, 20.0);
}

TEST_F(ParamManagerTest, MultipleUpdates)
{
  std::vector<std::pair<int, int>> changes;

  DEFINE_PARAM(int, retry_count, 3);

  SET_PARAM_CALLBACK(
    int, retry_count, [&](const int & old_v, const int & new_v) {
      changes.push_back({old_v, new_v});
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 多次修改参数
  node_->set_parameter(rclcpp::Parameter("retry_count", 5));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(50ms);

  node_->set_parameter(rclcpp::Parameter("retry_count", 7));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(50ms);

  node_->set_parameter(rclcpp::Parameter("retry_count", 10));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(50ms);

  // 验证所有变更都被记录
  EXPECT_EQ(changes.size(), 3);
  EXPECT_EQ(changes[0].first, 3);  // 3 -> 5
  EXPECT_EQ(changes[0].second, 5);
  EXPECT_EQ(changes[1].first, 5);  // 5 -> 7
  EXPECT_EQ(changes[1].second, 7);
  EXPECT_EQ(changes[2].first, 7);  // 7 -> 10
  EXPECT_EQ(changes[2].second, 10);
}

TEST_F(ParamManagerTest, BooleanCallback)
{
  int callback_invoked = 0;
  bool old_val = false;
  bool new_val = false;

  DEFINE_PARAM(bool, enable_safety, false);

  SET_PARAM_CALLBACK(
    bool, enable_safety, [&](const bool & old_v, const bool & new_v) {
      callback_invoked++;
      old_val = old_v;
      new_val = new_v;
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 修改参数
  node_->set_parameter(rclcpp::Parameter("enable_safety", true));

  // 等待回调执行
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 验证回调被调用
  EXPECT_EQ(callback_invoked, 1);
  EXPECT_FALSE(old_val);
  EXPECT_TRUE(new_val);
  EXPECT_TRUE(enable_safety);
}

TEST_F(ParamManagerTest, StringCallback)
{
  std::string last_old_value;
  std::string last_new_value;
  int callback_count = 0;

  DEFINE_PARAM(std::string, robot_name, "default_robot");

  SET_PARAM_CALLBACK(
    std::string, robot_name,
    [&](const std::string & old_v, const std::string & new_v) {
      callback_count++;
      last_old_value = old_v;
      last_new_value = new_v;
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 初始值
  EXPECT_EQ(robot_name, "default_robot");

  // 更新参数
  node_->set_parameter(rclcpp::Parameter("robot_name", "new_robot"));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 验证回调被调用
  EXPECT_EQ(callback_count, 1);
  EXPECT_EQ(last_old_value, "default_robot");
  EXPECT_EQ(last_new_value, "new_robot");
  EXPECT_EQ(robot_name, "new_robot");
}

TEST_F(ParamManagerTest, NamespacedParameterCallback)
{
  int callback_count = 0;
  double old_val = 0.0;
  double new_val = 0.0;

  DEFINE_PARAM_NS(robot, double, max_torque, 10.0);

  SET_PARAM_CALLBACK_NS(
    robot, double, max_torque,
    [&](const double & old_v, const double & new_v) {
      callback_count++;
      old_val = old_v;
      new_val = new_v;
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 初始值
  EXPECT_DOUBLE_EQ(max_torque, 10.0);

  // 更新命名空间参数
  node_->set_parameter(rclcpp::Parameter("robot.max_torque", 25.0));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 验证回调被调用
  EXPECT_EQ(callback_count, 1);
  EXPECT_DOUBLE_EQ(old_val, 10.0);
  EXPECT_DOUBLE_EQ(new_val, 25.0);
  EXPECT_DOUBLE_EQ(max_torque, 25.0);
}

TEST_F(ParamManagerTest, MemberFunctionCallback)
{
  // 创建一个测试类来模拟成员函数回调
  class TestCallbackClass
  {
  public:
    int callback_count = 0;
    double old_value = 0.0;
    double new_value = 0.0;

    void onParameterChanged(
      const double & old_v, const double & new_v)
    {
      callback_count++;
      old_value = old_v;
      new_value = new_v;
    }
  };

  TestCallbackClass callback_obj;

  DEFINE_PARAM(double, sensor_threshold, 0.5);

  // 使用lambda包装成员函数
  SET_PARAM_CALLBACK(
    double, sensor_threshold,
    [&callback_obj](const double & old_v, const double & new_v) {
      callback_obj.onParameterChanged(old_v, new_v);
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 初始值
  EXPECT_DOUBLE_EQ(sensor_threshold, 0.5);

  // 更新参数
  node_->set_parameter(rclcpp::Parameter("sensor_threshold", 0.8));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 验证成员函数回调被调用
  EXPECT_EQ(callback_obj.callback_count, 1);
  EXPECT_DOUBLE_EQ(callback_obj.old_value, 0.5);
  EXPECT_DOUBLE_EQ(callback_obj.new_value, 0.8);
  EXPECT_DOUBLE_EQ(sensor_threshold, 0.8);
}

TEST_F(ParamManagerTest, CallbackBusinessLogic)
{
  // 模拟业务逻辑：当速度超过阈值时记录警告
  int warning_count = 0;
  double last_excessive_speed = 0.0;

  DEFINE_PARAM(double, max_allowed_speed, 30.0);

  SET_PARAM_CALLBACK(
    double, max_allowed_speed,
    [&](const double & old_v, const double & new_v) {
      // 业务逻辑：如果新值超过50，记录警告
      if (new_v > 50.0) {
        warning_count++;
        last_excessive_speed = new_v;
        // 在实际应用中，这里可能会记录日志或采取其他措施
      }
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 初始值不超过阈值，不应触发警告
  EXPECT_EQ(warning_count, 0);

  // 更新到合理值（不超过阈值）
  node_->set_parameter(rclcpp::Parameter("max_allowed_speed", 40.0));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(warning_count, 0);  // 40 <= 50，不应触发警告
  EXPECT_DOUBLE_EQ(max_allowed_speed, 40.0);

  // 更新到超过阈值的值
  node_->set_parameter(rclcpp::Parameter("max_allowed_speed", 60.0));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 应触发警告
  EXPECT_EQ(warning_count, 1);
  EXPECT_DOUBLE_EQ(last_excessive_speed, 60.0);
  EXPECT_DOUBLE_EQ(max_allowed_speed, 60.0);

  // 再次更新到另一个超过阈值的值
  node_->set_parameter(rclcpp::Parameter("max_allowed_speed", 70.0));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(warning_count, 2);
  EXPECT_DOUBLE_EQ(last_excessive_speed, 70.0);
  EXPECT_DOUBLE_EQ(max_allowed_speed, 70.0);
}

TEST_F(ParamManagerTest, CallbackExceptionHandling)
{
  // 定义参数
  DEFINE_PARAM(double, max_speed, 10.0);

  // 设置会抛出异常的回调
  SET_PARAM_CALLBACK(
    double, max_speed, [](const double &, const double &) {
      throw std::runtime_error("Callback error");
    });

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 参数更新不应该因为回调异常而失败
  EXPECT_NO_THROW({
    node_->set_parameter(rclcpp::Parameter("max_speed", 35.0));
    rclcpp::spin_some(node_);
    std::this_thread::sleep_for(100ms);
  });

  // 参数值应该仍然被更新
  EXPECT_DOUBLE_EQ(max_speed, 35.0);
}

TEST_F(ParamManagerTest, UpdateWithoutCallback)
{
  // 定义参数但不设置回调
  DEFINE_PARAM(int, retry_count, 3);

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 直接更新参数，没有设置回调
  node_->set_parameter(rclcpp::Parameter("retry_count", 15));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  // 参数值应该正常更新
  EXPECT_EQ(retry_count, 15);
}

TEST_F(ParamManagerTest, CallbackAccessingNodeResources)
{
  // 定义参数
  DEFINE_PARAM(double, max_speed, 10.0);

  std::string logged_message;

  // 设置回调，访问节点资源
  SET_PARAM_CALLBACK(
    double, max_speed,
    ([this, &logged_message](
       const double & old_v, const double & new_v) {
      // 在回调中可以访问节点的日志功能
      logged_message = "Speed changed from " + std::to_string(old_v) +
                       " to " + std::to_string(new_v);
      RCLCPP_INFO(node_->get_logger(), "%s", logged_message.c_str());
    }));

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // 修改参数
  node_->set_parameter(rclcpp::Parameter("max_speed", 40.0));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_FALSE(logged_message.empty());
  EXPECT_NE(logged_message.find("10.000000"), std::string::npos);
  EXPECT_NE(logged_message.find("40.000000"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ParamManagerTest, CompleteWorkflow)
{
  DEFINE_PARAM(double, max_velocity, 2.0);
  DEFINE_PARAM(double, max_acceleration, 1.0);
  DEFINE_PARAM(bool, enable_safety, true);
  DEFINE_PARAM_NS(controller, double, kp, 1.0);
  DEFINE_PARAM_NS(controller, double, ki, 0.1);
  DEFINE_PARAM_NS(controller, double, kd, 0.01);

  auto & manager = utils::ParamManager::getInstance();
  manager.init(node_.get());

  // Verify initial values
  EXPECT_DOUBLE_EQ(param_max_velocity_storage_, 2.0);
  EXPECT_DOUBLE_EQ(param_max_acceleration_storage_, 1.0);
  EXPECT_TRUE(param_enable_safety_storage_);
  EXPECT_DOUBLE_EQ(param_controller_kp_storage_, 1.0);
  EXPECT_DOUBLE_EQ(param_controller_ki_storage_, 0.1);
  EXPECT_DOUBLE_EQ(param_controller_kd_storage_, 0.01);

  // Update some parameters
  node_->set_parameters(
    {rclcpp::Parameter("max_velocity", 3.0),
     rclcpp::Parameter("controller.kp", 2.0),
     rclcpp::Parameter("enable_safety", false)});

  for (int i = 0; i < 10; ++i) {
    rclcpp::spin_some(node_);
    std::this_thread::sleep_for(1ms);
  }

  // Verify updates
  EXPECT_DOUBLE_EQ(max_velocity, 3.0);
  EXPECT_DOUBLE_EQ(kp, 2.0);
  EXPECT_FALSE(enable_safety);

  // Unchanged parameters should remain the same
  EXPECT_DOUBLE_EQ(max_acceleration, 1.0);
  EXPECT_DOUBLE_EQ(ki, 0.1);
  EXPECT_DOUBLE_EQ(kd, 0.01);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
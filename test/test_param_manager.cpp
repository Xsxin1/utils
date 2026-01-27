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
    param::ParamManager::getInstance().reset();
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
  auto & instance1 = param::ParamManager::getInstance();
  auto & instance2 = param::ParamManager::getInstance();

  EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ParamManagerTest, InitializeWithNode)
{
  auto & manager = param::ParamManager::getInstance();

  // Should initialize without throwing
  EXPECT_NO_THROW(manager.init(node_.get()));
}

TEST_F(ParamManagerTest, InitializeMultipleTimes)
{
  auto & manager = param::ParamManager::getInstance();

  manager.init(node_.get());

  // Second initialization should not throw, just warn
  EXPECT_NO_THROW(manager.init(node_.get()));
}

// ============================================================================
// Parameter Registration Tests
// ============================================================================

TEST_F(ParamManagerTest, RegisterIntParameter)
{
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  struct TestStruct
  {
    DEFINE_PARAM(int, test_macro_param, 99);
  };

  TestStruct test_obj;

  auto & manager = param::ParamManager::getInstance();
  manager.init(node_.get());

  EXPECT_EQ(test_obj.test_macro_param, 99);

  // Update and verify
  node_->set_parameter(rclcpp::Parameter("test_macro_param", 200));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(test_obj.test_macro_param, 200);
}

TEST_F(ParamManagerTest, DefineParamNsMacro)
{
  struct TestStruct
  {
    DEFINE_PARAM_NS(robot, double, velocity, 1.5);
  };

  TestStruct test_obj;

  auto & manager = param::ParamManager::getInstance();
  manager.init(node_.get());

  EXPECT_DOUBLE_EQ(test_obj.velocity, 1.5);

  // Update with namespaced parameter
  node_->set_parameter(rclcpp::Parameter("robot.velocity", 2.5));
  rclcpp::spin_some(node_);
  std::this_thread::sleep_for(100ms);

  EXPECT_DOUBLE_EQ(test_obj.velocity, 2.5);
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

TEST_F(ParamManagerTest, RegisterBeforeInit)
{
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
  auto & manager = param::ParamManager::getInstance();

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
// Integration Tests
// ============================================================================

TEST_F(ParamManagerTest, CompleteWorkflow)
{
  struct RobotConfig
  {
    DEFINE_PARAM(double, max_velocity, 2.0);
    DEFINE_PARAM(double, max_acceleration, 1.0);
    DEFINE_PARAM(bool, enable_safety, true);
    DEFINE_PARAM_NS(controller, double, kp, 1.0);
    DEFINE_PARAM_NS(controller, double, ki, 0.1);
    DEFINE_PARAM_NS(controller, double, kd, 0.01);
  };

  RobotConfig config;
  auto & manager = param::ParamManager::getInstance();
  manager.init(node_.get());

  // Verify initial values
  EXPECT_DOUBLE_EQ(config.max_velocity, 2.0);
  EXPECT_DOUBLE_EQ(config.max_acceleration, 1.0);
  EXPECT_TRUE(config.enable_safety);
  EXPECT_DOUBLE_EQ(config.kp, 1.0);
  EXPECT_DOUBLE_EQ(config.ki, 0.1);
  EXPECT_DOUBLE_EQ(config.kd, 0.01);

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
  EXPECT_DOUBLE_EQ(config.max_velocity, 3.0);
  EXPECT_DOUBLE_EQ(config.kp, 2.0);
  EXPECT_FALSE(config.enable_safety);

  // Unchanged parameters should remain the same
  EXPECT_DOUBLE_EQ(config.max_acceleration, 1.0);
  EXPECT_DOUBLE_EQ(config.ki, 0.1);
  EXPECT_DOUBLE_EQ(config.kd, 0.01);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
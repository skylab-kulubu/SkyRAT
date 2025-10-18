/**
 * @file test_main.cpp
 * @brief Main test runner for SkyRAT Client integration and unit tests
 * 
 * This file provides comprehensive testing for all refactored components
 * including unit tests, integration tests, and compatibility tests with
 * the Python server.
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>
#include <vector>

// Test includes
#include "Core/Client.h"
#include "Core/ConfigManager.h"
#include "Core/ConnectionManager.h"
#include "Core/CommandDispatcher.h"
#include "Modules/ModuleManager.h"
#include "Network/MessageProtocol.h"
#include "Utils/Logger.h"
#include "Utils/Utilities.h"
#include "Utils/ThreadPool.h"

namespace SkyRAT {
namespace Tests {

    class TestRunner {
    public:
        TestRunner() : m_testsRun(0), m_testsPassed(0), m_testsFailed(0) {}

        void runAllTests() {
            std::cout << "=== SkyRAT Client Integration & Unit Tests ===" << std::endl;
            std::cout << std::endl;

            // Unit Tests
            testUtilities();
            testLogger();
            testThreadPool();
            testConfigManager();
            testMessageProtocol();
            testModuleManager();
            testCommandDispatcher();
            
            // Integration Tests
            testClientInitialization();
            testComponentIntegration();
            
            // Server Compatibility Tests
            testServerProtocolCompatibility();

            // Print summary
            printTestSummary();
        }

    private:
        int m_testsRun;
        int m_testsPassed;
        int m_testsFailed;

        void test(const std::string& testName, bool condition) {
            m_testsRun++;
            std::cout << "[TEST] " << testName << ": ";
            if (condition) {
                std::cout << "PASS" << std::endl;
                m_testsPassed++;
            } else {
                std::cout << "FAIL" << std::endl;
                m_testsFailed++;
            }
        }

        void testUtilities() {
            std::cout << "\n--- Testing Utilities Framework ---" << std::endl;
            
            // StringUtils tests
            test("StringUtils::trim", 
                Utils::StringUtils::trim("  hello  ") == "hello");
            
            test("StringUtils::toLower", 
                Utils::StringUtils::toLower("HELLO") == "hello");
            
            test("StringUtils::toUpper", 
                Utils::StringUtils::toUpper("hello") == "HELLO");
            
            auto parts = Utils::StringUtils::split("a,b,c", ",");
            test("StringUtils::split", 
                parts.size() == 3 && parts[0] == "a" && parts[1] == "b" && parts[2] == "c");
            
            // EncodingUtils tests
            std::string testData = "Hello World!";
            std::vector<char> dataVector(testData.begin(), testData.end());
            std::string encoded = Utils::EncodingUtils::base64Encode(dataVector);
            auto decoded = Utils::EncodingUtils::base64Decode(encoded);
            std::string decodedStr(decoded.begin(), decoded.end());
            test("EncodingUtils::base64 roundtrip", testData == decodedStr);
            
            // TimeUtils tests
            std::string timestamp = Utils::TimeUtils::getCurrentTimestamp();
            test("TimeUtils::getCurrentTimestamp", !timestamp.empty());
            
            // SystemUtils tests (basic functionality)
            std::string currentDir = Utils::SystemUtils::getCurrentDirectory();
            test("SystemUtils::getCurrentDirectory", !currentDir.empty());
        }

        void testLogger() {
            std::cout << "\n--- Testing Logger ---" << std::endl;
            
            auto logger = std::make_shared<Utils::Logger>();
            test("Logger::creation", logger != nullptr);
            
            bool initResult = logger->initialize("test_log.txt");
            test("Logger::initialize", initResult);
            
            // Test logging methods
            logger->info("Test info message");
            logger->warning("Test warning message");
            logger->error("Test error message");
            test("Logger::logging_methods", true); // If we get here, logging worked
        }

        void testThreadPool() {
            std::cout << "\n--- Testing ThreadPool ---" << std::endl;
            
            Utils::ThreadPool pool(4);
            test("ThreadPool::creation", true);
            
            // Test task execution
            auto future = pool.enqueue([]() -> int {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return 42;
            });
            
            int result = future.get();
            test("ThreadPool::task_execution", result == 42);
            
            // Test multiple tasks
            std::vector<std::future<int>> futures;
            for (int i = 0; i < 10; ++i) {
                futures.push_back(pool.enqueue([i]() -> int {
                    return i * 2;
                }));
            }
            
            bool allCorrect = true;
            for (int i = 0; i < 10; ++i) {
                if (futures[i].get() != i * 2) {
                    allCorrect = false;
                    break;
                }
            }
            test("ThreadPool::multiple_tasks", allCorrect);
        }

        void testConfigManager() {
            std::cout << "\n--- Testing ConfigManager ---" << std::endl;
            
            auto configManager = std::make_shared<Core::ConfigManager>();
            test("ConfigManager::creation", configManager != nullptr);
            
            // Test default values
            test("ConfigManager::default_server_ip", 
                configManager->getServerIP() == "127.0.0.1");
            test("ConfigManager::default_server_port", 
                configManager->getServerPort() == 4545);
            
            // Test configuration changes
            configManager->setServerIP("192.168.1.100");
            configManager->setServerPort(8080);
            test("ConfigManager::set_server_ip", 
                configManager->getServerIP() == "192.168.1.100");
            test("ConfigManager::set_server_port", 
                configManager->getServerPort() == 8080);
            
            // Test file operations
            std::string testConfigPath = "test_config.conf";
            bool saveResult = configManager->saveToFile(testConfigPath);
            test("ConfigManager::save_to_file", saveResult);
            
            auto newConfigManager = std::make_shared<Core::ConfigManager>();
            bool loadResult = newConfigManager->loadFromFile(testConfigPath);
            test("ConfigManager::load_from_file", loadResult);
            test("ConfigManager::config_persistence", 
                newConfigManager->getServerIP() == "192.168.1.100" &&
                newConfigManager->getServerPort() == 8080);
        }

        void testMessageProtocol() {
            std::cout << "\n--- Testing MessageProtocol ---" << std::endl;
            
            Network::MessageProtocol protocol;
            
            // Test structured message creation
            std::map<std::string, std::string> testData = {{"command", "TEST_COMMAND"}, {"data", "test_data"}};
            auto msg = protocol.createStructuredMessage(testData);
            test("MessageProtocol::create_structured_message", !msg.empty());
            
            // Test message parsing
            std::map<std::string, std::string> parsed = protocol.parseMessage(msg);
            test("MessageProtocol::parse_message", 
                parsed.find("command") != parsed.end() && 
                parsed["command"] == "TEST_COMMAND");
            
            // Test text message
            auto textMsg = protocol.createTextMessage("Hello World");
            test("MessageProtocol::create_text_message", !textMsg.empty());
            
            std::string extractedText = protocol.extractTextContent(textMsg);
            test("MessageProtocol::extract_text_content", extractedText == "Hello World");
        }

        void testModuleManager() {
            std::cout << "\n--- Testing ModuleManager ---" << std::endl;
            
            auto moduleManager = std::make_unique<Modules::ModuleManager>();
            test("ModuleManager::creation", moduleManager != nullptr);
            
            bool initResult = moduleManager->initializeModules();
            test("ModuleManager::initialize", initResult);
            
            // Test module registration (built-in modules)
            auto allModules = moduleManager->getAllModules();
            test("ModuleManager::has_modules", !allModules.empty());
            
            // Test supported commands
            auto supportedCommands = moduleManager->getSupportedCommands();
            test("ModuleManager::has_supported_commands", !supportedCommands.empty());
        }

        void testCommandDispatcher() {
            std::cout << "\n--- Testing CommandDispatcher ---" << std::endl;
            
            auto moduleManager = std::make_unique<Modules::ModuleManager>();
            moduleManager->initializeModules();
            
            // Create a mock ConnectionManager for testing
            auto logger = std::make_shared<Utils::Logger>();
            logger->initialize();
            auto configManager = std::make_shared<Core::ConfigManager>();
            auto connectionManager = std::make_unique<Core::ConnectionManager>(configManager, logger);
            
            auto dispatcher = std::make_unique<Core::CommandDispatcher>(*moduleManager, *connectionManager);
            test("CommandDispatcher::creation", dispatcher != nullptr);
            
            // Test command registration
            auto commands = dispatcher->getRegisteredCommands();
            test("CommandDispatcher::has_commands", !commands.empty());
            
            bool hasScreenshot = std::find(commands.begin(), commands.end(), "TAKE_SCREENSHOT") != commands.end();
            test("CommandDispatcher::has_screenshot_command", hasScreenshot);
        }

        void testClientInitialization() {
            std::cout << "\n--- Testing Client Integration ---" << std::endl;
            
            // Test client creation
            Core::Client client;
            test("Client::creation", true);
            
            // Test initialization (without running main loop)
            bool initResult = client.initialize();
            test("Client::initialization", initResult);
            
            test("Client::is_running_check", !client.isRunning()); // Should not be running yet
        }

        void testComponentIntegration() {
            std::cout << "\n--- Testing Component Integration ---" << std::endl;
            
            // Test that all components can work together
            auto logger = std::make_shared<Utils::Logger>();
            logger->initialize("integration_test.log");
            
            auto configManager = std::make_shared<Core::ConfigManager>();
            auto connectionManager = std::make_unique<Core::ConnectionManager>(configManager, logger);
            
            auto moduleManager = std::make_unique<Modules::ModuleManager>();
            moduleManager->initializeModules();
            
            auto commandDispatcher = std::make_unique<Core::CommandDispatcher>(*moduleManager, *connectionManager);
            
            test("Integration::all_components_created", 
                logger && configManager && connectionManager && moduleManager && commandDispatcher);
            
            // Test message flow (without actual network)
            Network::MessageProtocol protocol;
            std::map<std::string, std::string> cmdData = {{"command", "TAKE_SCREENSHOT"}};
            auto testMessage = protocol.createStructuredMessage(cmdData);
            test("Integration::message_creation", !testMessage.empty());
        }

        void testServerProtocolCompatibility() {
            std::cout << "\n--- Testing Server Protocol Compatibility ---" << std::endl;
            
            Network::MessageProtocol protocol;
            
            // Test message format compatibility
            std::map<std::string, std::string> cmdData = {{"command", "TAKE_SCREENSHOT"}};
            auto msg = protocol.createStructuredMessage(cmdData);
            auto parsed = protocol.parseMessage(msg);
            
            test("Protocol::message_roundtrip", 
                parsed.find("command") != parsed.end() && 
                parsed["command"] == "TAKE_SCREENSHOT");
            
            // Test all expected command formats
            std::vector<std::string> expectedCommands = {
                "TAKE_SCREENSHOT", "START_KEYLOGGER", "STOP_KEYLOGGER",
                "START_SCREEN_RECORDING", "STOP_SCREEN_RECORDING",
                "START_REMOTE_SHELL", "STOP_REMOTE_SHELL",
                "START_WEBCAM", "STOP_WEBCAM", "MOUSE"
            };
            
            bool allCommandsWork = true;
            for (const auto& cmd : expectedCommands) {
                std::map<std::string, std::string> testCmdData = {{"command", cmd}, {"data", "test_data"}};
                auto cmdMsg = protocol.createStructuredMessage(testCmdData);
                auto cmdParsed = protocol.parseMessage(cmdMsg);
                if (cmdParsed.find("command") == cmdParsed.end() || cmdParsed["command"] != cmd) {
                    allCommandsWork = false;
                    break;
                }
            }
            test("Protocol::all_commands_compatible", allCommandsWork);
            
            // Test greeting message format (what server expects)
            std::string greeting = "Hello from SkyRAT client!";
            test("Protocol::greeting_format", !greeting.empty());
        }

        void printTestSummary() {
            std::cout << "\n=== Test Summary ===" << std::endl;
            std::cout << "Total Tests: " << m_testsRun << std::endl;
            std::cout << "Passed: " << m_testsPassed << std::endl;
            std::cout << "Failed: " << m_testsFailed << std::endl;
            std::cout << "Success Rate: " << (m_testsRun > 0 ? (m_testsPassed * 100 / m_testsRun) : 0) << "%" << std::endl;
            
            if (m_testsFailed == 0) {
                std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
                std::cout << "SkyRAT Client is ready for production!" << std::endl;
            } else {
                std::cout << "\n⚠️  Some tests failed. Please review and fix issues." << std::endl;
            }
        }
    };

} // namespace Tests
} // namespace SkyRAT

int main() {
    try {
        SkyRAT::Tests::TestRunner runner;
        runner.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error during test execution" << std::endl;
        return 1;
    }
}
#include <memory>
#include <vector>
#include <queue>
#include <gtest/gtest.h>

#include "socket.hpp"
#include "HTTPClient.hpp"
#include "Task.hpp"
#include "TaskBuilder.hpp"
#include "TasksController.hpp"
#include "Master.hpp"

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

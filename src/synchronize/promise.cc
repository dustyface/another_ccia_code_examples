#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <future>

std::string process_db_query(const std::string& query) {
    // simulate database delay access process
    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (query == "error") {
        throw std::runtime_error("DB error occured");
    }
    return "Database: " + query;
}

void test_promise() {
    std::vector<std::future<std::string>> futures;

    for (const auto& query: {"select * from user", "select * from products", "error"}) {
        std::promise<std::string> promise;
        std::future<std::string> future = promise.get_future();
        futures.emplace_back(std::move(future));

        std::thread([promise = std::move(promise), query]() mutable {
            try {
                std::string data = process_db_query(query);
                promise.set_value(data);
            } catch(const std::runtime_error& e) {
                promise.set_exception(std::current_exception());
            }
        }).detach();
    }

    for (auto& future: futures) {
        try {
            std::cout << future.get() << std::endl;
        } catch(const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
}
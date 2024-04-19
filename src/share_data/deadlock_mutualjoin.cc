#include <iostream>
#include <thread>
#include <unistd.h>
#include "share_data.h"

std::thread global_tx;
std::thread global_ty;

void run() {
    std::thread tx([]{
        std::cout << "thread tx=" << std::this_thread::get_id() << std::endl;
        std::cout << "tx is about to sleep 2s" << std::endl;
        sleep(2);
        if (global_ty.joinable()) {
            std::cout << "global_ty is about to join" << std::endl;
            global_ty.join();
        }
    });
    global_tx = std::move(tx);
    std::thread ty([]{
        std::cout << "thread ty=" << std::this_thread::get_id() << std::endl;
        if (global_tx.joinable()) {
            std::cout << "global_tx is about to join" << std::endl;
            global_tx.join();
        }
        std::cout << "ty is about to sleep 2s" << std::endl;
        sleep(2);
    });
    global_ty = std::move(ty);
    std::cout << "main is about to sleep 5s" << std::endl;
    sleep(5);
    std::cout << "main end" << std::endl;
}
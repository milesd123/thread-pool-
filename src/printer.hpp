#include <iostream>
#include <mutex>
#include <condition_variable>

// Avoid print messages getting mangled in the terminal
class Printer{
    public:
        Printer() = default;
        
        void print(std::string&& string){
            std::unique_lock lock(printex);

            print_condition_var.wait(lock, [this](){
                return open;
            });
            open = false;
            std::cout << string;
            open = true;
            lock.unlock();
            print_condition_var.notify_one();
        }

    private:
        bool open = true;
        std::mutex printex;
        std::condition_variable print_condition_var;
};
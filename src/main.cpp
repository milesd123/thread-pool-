#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <future>
#include <queue>

#include "printer.hpp"
Printer printer;

class ThreadPool{
    public:
    ThreadPool(size_t n) :   number_of_threads(n){
        threads_.reserve(n);
    }
    
    //TODO: Implement concurrent enqueueing.
    void enqueue(std::function<void()> func){
        std::unique_lock<std::mutex> lock(queue_mutex);

        queue_condition_var.wait(lock, [this](){
            return queue_open || !running;
        });

        queue_open = false;

        if(num_jobs < 0){
            printer.print("Negative job count, potential data race...\n");
            // std::cout << "Negative job count, potential data race...\n";
        }
        
        num_jobs++;
        tasks.emplace(func);
        
        if(num_jobs == 1 && running){
            //only 1 thread will be awaiting this.
            jobs_available_condition_var.notify_one();
        }
        printer.print("Function queued. job count: " + std::to_string(num_jobs) + "\n");
        // std::cout << "Function queued. job count: " << num_jobs << std::endl;

        queue_open = true;
        queue_condition_var.notify_one();
        lock.unlock();
    }

    void start(){ 
        running = true;
        printer.print("Started\n");
        // std::cout << "Started\n";
        run();
    }

    void stop(){
        printer.print("Stopping...\n");
        // std::cout << "Stopping...\n";
        running = false;
        queue_condition_var.notify_all();
        jobs_available_condition_var.notify_all();

        // join all threads
        for(auto& th : threads_){
            th.join();
        }
    }

    private:
//------- Thread/vector members
        size_t number_of_threads;
        std::vector<std::thread> threads_;
        std::queue<std::function<void()>> tasks;
        size_t num_jobs = 0;
//------- Atomic/concurrency members
        bool running = false;
        bool queue_open = true;
        std::mutex queue_mutex;
        std::mutex job_mutex;
        std::condition_variable queue_condition_var;
        std::condition_variable jobs_available_condition_var;

//------- Retreive job from queue
        bool dequeue(std::function<void()>& work_function) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            std::unique_lock<std::mutex> job_count_lock(job_mutex); 
            // std::cout << "Thread "<< std::this_thread::get_id() << " Has locked dequeue()\n";

            queue_condition_var.wait(lock, [this](){ // essentially a better lock.lock();
                 return !running || queue_open; // re-checks this upon notify
            });   

            jobs_available_condition_var.wait(job_count_lock, [this](){
                return !running || (num_jobs > 0);
            });

            if(!running){
                return false;
            }

            
            //do work
            queue_open = false;
            work_function = std::move(tasks.front());
            tasks.pop();
            num_jobs--;

            // let another thread access now
            queue_open = true;
            lock.unlock();
            job_count_lock.unlock();
            queue_condition_var.notify_one();

            return true;
        }
//------ Run the thread pool
        void run(){
            for(int i = 1; i <= number_of_threads; i++){
                threads_.emplace_back(std::thread([this, i](){
                    while(running){
                        // std::cout << "Thread " << i << " Awaiting work\n"; 
                        //get a function, do its work.
                        std::function<void()> work;
                        if(dequeue(work)){
                            printer.print("Thread " + std::to_string(i) + " Work: ");
                            work();
                        }
                    }
                    printer.print("Thread " + std::to_string(i) + " Stopped\n");
                }));
            }
        }
};

// Work to be done
void func1(){
    printer.print("Function 1 Printing!\n");
    // std::cout << "Function 1 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func2(){
    printer.print("Function 2 Printing!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
}
void func3(){
    printer.print("Function 3 Printing!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func4(){
    printer.print("Function 4 Printing!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func5(){
    printer.print("Function 5 Printing!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func6(){
    printer.print("Function 6 Printing!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void funcNew(){
    printer.print("Running Queued Function Printing!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(){
    printer.print("main start\n");
    // tests
    ThreadPool pool(5);
    pool.enqueue(func1);
    pool.enqueue(func2);
    pool.enqueue(func3);
    pool.enqueue(func4);
    pool.enqueue(func5);
    pool.enqueue(func6);
    pool.enqueue([](){
        printer.print("Lambda Fn Printing!\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));

    });
    pool.start();
    pool.enqueue(funcNew);
    pool.enqueue(funcNew);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    pool.stop();
    return 0;
}

// TODO :
/*




*/
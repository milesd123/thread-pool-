#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <future>
#include <queue>

class ThreadPool{
    public:
    ThreadPool(size_t n) :   number_of_threads(n){
        threads_.reserve(n);
    }
    
    //TODO: Implement concurrent enqueueing.
    void enqueue(std::function<void()> func){
        if(num_jobs < 0){
            std::cout << "Negative job count, potential data race\n";
        }
        
        num_jobs++;
        tasks.emplace(func);
        
        if(num_jobs == 1 && running){
            //only 1 thread will be awaiting this.
            jobs_available_condition_var.notify_one();
        }

        std::cout << "Function queued. job count: " << num_jobs << std::endl;
    }

    void start(){ 
        running = true;
        std::cout << "Started\n";
        run();
    }

    void stop(){
        std::cout << "Stopping...\n";
        running = false;
        dequeue_condition_var.notify_all();
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
        bool dequeue_open = true;
        std::mutex dequeue_mutex;
        std::mutex job_mutex;
        std::condition_variable dequeue_condition_var;
        std::condition_variable jobs_available_condition_var;

//------- Retreive job from queue
        bool dequeue(std::function<void()>& work_function) {
            std::unique_lock<std::mutex> lock(dequeue_mutex);
            std::unique_lock<std::mutex> job_count_lock(job_mutex); 
            // std::cout << "Thread "<< std::this_thread::get_id() << " Has locked dequeue()\n";

            dequeue_condition_var.wait(lock, [this](){ // essentially a better lock.lock();
                 return !running || dequeue_open; // re-checks this upon 
            });   

            jobs_available_condition_var.wait(job_count_lock, [this](){
                return !running || (num_jobs > 0);
            });

            if(!running){
                return false;
            }

            
            //do work
            dequeue_open = false;
            work_function = std::move(tasks.front());
            tasks.pop();
            num_jobs--;

            // let another thread access now
            dequeue_open = true;
            lock.unlock();
            job_count_lock.unlock();
            dequeue_condition_var.notify_one();

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
                            std::cout << "Thread " << i << " Work:   ";
                            work();
                        }
                    }
                    std::cout << "Thread "<<i<<" Stopped\n";
                }));
            }
        }
};

// Work to be done
void func1(){
    std::cout << "Function 1 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func2(){
    std::cout << "Function 2 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
}
void func3(){
    std::cout << "Function 3 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func4(){
    std::cout << "Function 4 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func5(){
    std::cout << "Function 5 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
void func6(){
    std::cout << "Function 6 Printing!\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

int main(){
    std::cout << "Thread Pool\n";
    // tests
    ThreadPool pool(5);
    pool.enqueue(func1);
    pool.enqueue(func2);
    pool.enqueue(func3);
    pool.enqueue(func4);
    pool.enqueue(func5);
    pool.enqueue(func6);
    pool.enqueue([](){
        std::cout << "Lambda Fn Printing!\n";
    });
    pool.start();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    pool.stop();
    return 0;
}

// TODO :
/*




*/
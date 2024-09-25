#include<iostream>
#include<thread>
#include<queue>
#include<string>
#include<mutex>
#include<condition_variable>
#include<vector>
#include<functional>
class ThreadPool {
public:
	ThreadPool(int numThreads) :stop(false)
	{
		for (int i = 0; i < numThreads; i++) {
			threads.emplace_back([this] {
				while (1)
				{
					std::unique_lock<std::mutex> lock(mtx);
					condition.wait(lock, [this]
						{
							return !tasks.empty() || stop;
						});

					if (stop && tasks.empty()) {
						return;
					}
					std::function<void()> task(std::move(tasks.front()));
					tasks.pop();
					lock.unlock();
					task();
				}
				});
		}
	}
	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			stop = true;
		}
		condition.notify_all();
		for (auto& t : threads) {
			t.join();
		}
	}
	template<class F,class... Args>
	void enqueue( F &&f,Args&&...args) {
		std::function<void()>task =
			std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		{
			std::unique_lock<std::mutex> lock(mtx);
			  tasks.emplace(std::move(task));
		}
		condition.notify_one();
	}
private:
	std::vector <std::thread> threads;
	std::queue<std::function<void()>> tasks;
	std::mutex mtx;
	std::condition_variable condition;
	bool stop;
};

int main()
{
	ThreadPool pool(4);
	for (int i = 1; i < 10; i++)
	{
		pool.enqueue([i] {
			std::cout << "task : " << i << " is running" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "task : " << i << " is done" << std::endl;
			});
	}
	return 0;
}
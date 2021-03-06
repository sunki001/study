#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <iostream>

/*---------------------------------------*/
/* 시간관련 클래스, 함수들 */
class sbTimer
{
private:
	std::chrono::time_point<std::chrono::system_clock> start_time, end_time;
	std::chrono::time_point<std::chrono::system_clock> start_tact, end_tact;
	double set_time; // 타이머 시작할때 설정해놓는 시간.

	double t_duration_milli() { std::chrono::duration<double, std::milli> duration = (end_time - start_time); return duration.count(); }
	double t_duration_micro() { std::chrono::duration<double, std::micro> duration = (end_time - start_time); return duration.count(); }
	double t_duration_nano() { std::chrono::duration<double, std::nano> duration = (end_time - start_time); return duration.count(); }

public:
	//----------------------------------------------
	// 시간 경과 함수 start(), end(), duration()
	//----------------------------------------------
	//void t_start(double set) { start_time = std::chrono::system_clock::now(); set_time = set; }
	void t_start() { start_time = std::chrono::system_clock::now(); set_time = 0; }
	void t_end() { end_time = std::chrono::system_clock::now(); }
	double t_end_milli() { t_end(); return t_duration_milli(); }
	double t_end_micro() { t_end(); return t_duration_micro(); }
	double t_end_nano() { t_end(); return t_duration_nano(); }
	bool t_over(double mili_wait) {  // 정해진 시간이 경과했는가?
		t_end();
		if (t_duration_milli() > mili_wait) return true;
		else return false;
	}
	//bool t_over() {
	//	if (t_duration_milli() > set_time) return true;
	//	else return false;
	//}

	void tact_start() { start_tact = std::chrono::system_clock::now(); }
	double get_tact() { 
		end_tact = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> duration = (end_tact - start_tact); return duration.count();
	}

};

//-------------------------------------------------
/* thread pool */
//-------------------------------------------------
//namespace sblib 
//{
#define MAX_POOL_SIZE 30

class ThreadPool 
{
public:
	ThreadPool(size_t num_threads);
	~ThreadPool();


private:
	const size_t num_max_pool_;  // 최대 thread pool size
	size_t num_closed_;
	// 총 Worker 쓰레드의 개수.
	size_t num_threads_;
	// Worker 쓰레드를 보관하는 벡터.
	std::vector<std::thread> worker_threads_;

    using ret_jobs_q = std::function<void()>; // return type은 void로 하여 범용성을 유지한다.
	// 할일들을 보관하는 job 큐.
	std::queue<ret_jobs_q/*std::function<void()>*/> jobs_;

	// 위의 thread-safe하지 않는 job 큐를 위한 cv 와 m.
	std::condition_variable cv_job_q_;
	std::mutex m_job_q_;

	// 모든 쓰레드 종료
	bool stop_all;

	// Worker 쓰레드
	void WorkerThread();

public:
	size_t get_num_thread_pool() { return num_threads_; }

	// job 을 추가한다.
	/* return 값과 argument를 가지는 함수들 등록하기 위한 템플릿 */
	template <class F, class... Args>
	//std::future<typename std::result_of<F(Args...)>::type> EnqueueJob(F&& f, Args&&... args) 
	std::future<std::invoke_result_t<F, Args...>> EnqueueJob(F&& f, Args&&... args)
	{
		if (stop_all) {
			throw std::runtime_error("ThreadPool 사용 중지됨");
		}

		//using return_type = typename std::result_of<F(Args...)>::type;  // c++17 이후부터는 없어짐.. deprecated
		using return_type = std::invoke_result_t<F, Args...>;

		auto job = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::future<return_type> job_result_future = job->get_future();
		{
			std::lock_guard<std::mutex> lock(m_job_q_);
			jobs_.push([job]() { (*job)(); });
		}
		cv_job_q_.notify_one();

		return job_result_future;
	}

	//void EnqueueJob(std::function<void()> job);
};

//}  // namespace sblib

//-------------------------------------------------
/* thread pool */
//-------------------------------------------------
//namespace sblib
//{
	ThreadPool::ThreadPool(size_t num_threads)
		: num_max_pool_(MAX_POOL_SIZE), num_threads_(num_threads), stop_all(false)
	{
		if (num_threads_ < 1 || num_threads_>num_max_pool_)
		{
			int concurrency_hint = std::thread::hardware_concurrency();
			if (concurrency_hint > 1)
				num_threads_ = concurrency_hint;
			else
				num_threads_ = 3;
		}

        num_closed_ = 0;
		worker_threads_.reserve(num_threads_);
		for (size_t i = 0; i < num_threads_; ++i) 
		{
			worker_threads_.emplace_back([this]() { this->WorkerThread(); });
		}
	}

	void ThreadPool::WorkerThread() 
	{
		while (true) 
		{
			std::unique_lock<std::mutex> lock(m_job_q_);
			cv_job_q_.wait(lock, [this]() { return !this->jobs_.empty() || stop_all; });

			if (stop_all && this->jobs_.empty()) {
				num_closed_++;  // 시작 pool number와 close number 비교..
				return;
			}

			// 맨 앞의 job 을 뺀다.
			ret_jobs_q/*std::function<void()>*/ job = std::move(jobs_.front());
			jobs_.pop();
			lock.unlock();

			// 해당 job 을 수행한다 :)
			job();
		}
	}

	ThreadPool::~ThreadPool() 
	{
		stop_all = true;
		cv_job_q_.notify_all();

		for (auto& t : worker_threads_) 
		{
			t.join();
		}
	}

	//void ThreadPool::EnqueueJob(std::function<void()> job) {
	//	if (stop_all) {
	//		throw std::runtime_error("ThreadPool 사용 중지됨");
	//	}
	//	{
	//		std::lock_guard<std::mutex> lock(m_job_q_);
	//		jobs_.push(std::move(job));
	//	}
	//	cv_job_q_.notify_one();
	//}

//}  // namespace sblib

/* 참고 class */
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <vector>
#include <queue>
#include <condition_variable>
#include <chrono>

typedef enum __ENUM_COND_VAR_RSLT__
{
	COND_VAR_RSLT_TIMEOUT = 0,
	COND_VAR_RSLT_SIGNALED
} ENUM_COND_VAR_RSLT ;

///////////////////////////////////////////////////////////////////////////////
class CondVar
{
	public:
		CondVar()  = default;
		~CondVar() = default;

		void NotifyOne()
		{
			std::unique_lock<std::mutex> lock (cond_var_lock_);
			is_notified_ = true;
			cond_var_.notify_one();
		}

		void NotifyAll()
		{
			std::unique_lock<std::mutex> lock (cond_var_lock_);
			is_notified_ = true;
			cond_var_.notify_all();
		}

		void WaitForSignal()
		{
			std::unique_lock<std::mutex> lock (cond_var_lock_);
			while (!is_notified_) 
			{
				cond_var_.wait(lock );
			}    
			if(!is_all_waiting_end_)
			{
				is_notified_=false;
			}
		}

		ENUM_COND_VAR_RSLT WaitForSignalTimeout(int timeout_secs)
		{
			std::unique_lock<std::mutex> lock (cond_var_lock_);
			std::cv_status ret = std::cv_status::no_timeout;

			auto duration_sec = std::chrono::seconds(timeout_secs);

			while(!is_notified_ && std::cv_status::timeout !=ret)
			{ 
				ret=cond_var_.wait_for(lock, duration_sec);
			}

			if(!is_all_waiting_end_)
			{
				is_notified_=false;
			}

			if(std::cv_status::timeout ==ret)
			{
				return COND_VAR_RSLT_TIMEOUT;
			}
			return COND_VAR_RSLT_SIGNALED;
		}

		void SetAllWaitingEnd()
		{
			is_all_waiting_end_ = true;
		}

	private:    
		std::mutex              cond_var_lock_ ;
		std::condition_variable cond_var_ ;
		bool is_notified_ {false};
		bool is_all_waiting_end_ {false};

};

///////////////////////////////////////////////////////////////////////////////
class KoThreadPool 
{
	public:
		KoThreadPool()  = default;

		~KoThreadPool()
		{
			if(!stop_flag_)
			{
				Terminate();
			}
		}

		bool InitThreadPool(int num_of_threads= 0) 
		{
			if(num_of_threads == 0 )
			{
				int concurrency_hint = std::thread::hardware_concurrency();
				std::cout <<"std::thread::hardware_concurrency() = " << concurrency_hint << "\n";

				if(concurrency_hint > 1 )
				{
					num_of_threads_  = concurrency_hint ;
				}
				else
				{
					std::cerr << "std::thread::hardware_concurrency() failed! specify number of thread \n";
					return false;
				}
			}
			else
			{
				num_of_threads_  = num_of_threads ;
			}


			for(int i=0; i < num_of_threads_ ; i++)
			{
				//vec_thread_.push_back( std::thread (&KoThreadPool::WorkerThreadRoutine, this, i) ) ;
				vec_thread_.emplace_back(std::thread(&KoThreadPool::WorkerThreadRoutine, this, i));
			}

			return true;
		}

		void AssignTask( std::function<void()> & func ) 
		{
			PushQueue(func);
			cond_var_.NotifyOne();
		}

		void Terminate(bool terminate_immediately=false)
		{
			stop_flag_ = true;
			is_terminate_immediately_ = terminate_immediately;

			if(is_terminate_immediately_)
			{
				cond_var_.SetAllWaitingEnd(); //terminate immediately
			}

			cond_var_.NotifyAll();

			for(size_t i = 0; i < vec_thread_.size(); i++)
			{
				if (vec_thread_[i].joinable())
				{
					vec_thread_[i].join();
				}
			}
		}

		bool IsQueueEmpty()
		{
			std::unique_lock<std::mutex> lock(mutex_);
			return task_queue_.empty();
		}

	private:

		std::queue<std::function<void()>  > task_queue_ ;
		std::mutex    mutex_      ;
		std::atomic<bool>   stop_flag_ {false};
		std::atomic<bool>   is_terminate_immediately_ {false};
		std::vector<std::thread> vec_thread_ ;
		CondVar     cond_var_ ;
		int         num_of_threads_ {-1};

	private:

		void PushQueue( std::function<void()> const & value) 
		{
			std::unique_lock<std::mutex> lock(mutex_);
			task_queue_.push(value);
		}

		bool  PopQueue() 
		{
			std::function<void()> func ;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				if (task_queue_.empty())
				{
					return false;
				}

				func = task_queue_.front();
				task_queue_.pop();
			}

			func();

			return true;
		}


		void WorkerThreadRoutine(int index)
		{
			while (true) 
			{
				if(IsQueueEmpty())
				{
					if(stop_flag_)
					{
						//graceful terminate
						cond_var_.NotifyAll();
						return; 
					}
					cond_var_.WaitForSignal();
				}

				if(is_terminate_immediately_ )
				{
					//force terminate
					std::cout <<"WorkerThreadRoutine terminated: index " << index << "\n";
					return;
				}

				PopQueue() ;
			} 
		}
		
		KoThreadPool(const KoThreadPool &) = delete;
		KoThreadPool(KoThreadPool &&) = delete;
		KoThreadPool & operator=(const KoThreadPool &) = delete;
		KoThreadPool & operator=(KoThreadPool &&) = delete;
};


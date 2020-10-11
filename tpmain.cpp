#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <iostream>
#include <fstream>

#include "sblib.h"
#include "kothread.hpp"

std::mutex mtx_jobs;

// 사용 예시
int tpool_work(int t, int id)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(t));

	std::lock_guard<std::mutex> lock(mtx_jobs);
	std::cout << "[" << id << " end after " << t << " msec]" << std::endl;

	return t + id;
}

int tpool_work_m(int t, int id, std::mutex& m)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(t));

	std::lock_guard<std::mutex> lock(m);
	std::cout << "[" << id << " end after " << t << " msec]" << std::endl;

	return t + id;
}

class sbTest
{
public:

	std::vector<std::future<int>> _v_futures;

	std::mutex _m_job_q;

	int tpool_work2(int t, int id);
	void get_tpool_result();
};

int sbTest::tpool_work2(int t, int id)
{
//	std::lock_guard<std::mutex> lock(_m_job_q);

	std::this_thread::sleep_for(std::chrono::milliseconds(t));

	std::lock_guard<std::mutex> lock(mtx_jobs);
	std::cout << "[" << id << " end after " << t << " msec]" << std::endl;

	return t + id;
}

void sbTest::get_tpool_result()
{
	if (_v_futures.empty()) return;

	try {
		int id = 0;
		for (auto& f : _v_futures) {
			std::cout << "<" << id++ << ", result : " << f.get() << ">" << std::endl;
		}
		_v_futures.clear();
	}
	catch (...) {
		std::cout << "tpool option >> exception occurred!" << std::endl;
		_v_futures.clear();
	}
}
 
std::atomic<int> gSum1;
std::atomic<int> gSum2;

///////////////////////////////////////////////////////////////////////////////
bool MyThreadWork()
{
	gSum1++;
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return true;
}

///////////////////////////////////////////////////////////////////////////////
class MyClass
{
public:
	MyClass() {};
	~MyClass() {};

	bool MyThreadWork()
	{
		gSum2++;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));

		return true;
	}

};

int main()
{
    ThreadPool tpool(29);
	sbTimer timer;
	
	std::cout << "thread pool number: " << tpool.get_num_thread_pool() << std::endl;

    std::vector<std::future<int>> futures;
    int qmax = 100;
    
    int t_type = 0;
	timer.t_start();
   
    switch(t_type)
    {
        case 0:
            {
				std::mutex mtx_pool;
				
                for (int i = 0; i < qmax; i++) {
                    futures.emplace_back(tpool.EnqueueJob(
//                        std::bind(&tpool_work, (i % 3 + 1) * 10, i))
                        std::bind(&tpool_work_m, (i % 3 + 1) * 10, i, std::ref(mtx_pool)))
                                        );
                }

                for (auto& f : futures) {
                    std::cout << "result : " << f.get() << std::endl;
                }
				std::cout << "task: " << qmax << ", tact time: " << timer.t_end_milli() << " msec" << std::endl;
            }
            break;
            
        case 1: // 현재 함수내에서만 thread start하고, return 값 확인 할 경우
            {
                std::vector<std::future<int>> futures2;
                sbTest sbt;

                for (int i = 0; i < qmax; i++) {
                    futures2.emplace_back(tpool.EnqueueJob(
                        std::bind(&sbTest::tpool_work2, &sbt, (i % 3 + 1) * 10, i))
                                         );
                }

                for (auto& f : futures2) {
                    std::cout << "result : " << f.get() << std::endl;
                }
				
				std::cout << "task: " << qmax << ", tact time: " << timer.t_end_milli() << " msec" << std::endl;
           }
            break;
			
		case 2: // thread 시작은 여기서 하고, return 값은 다른 곳에서 확인하는 경우
			{
				sbTest sbt;
				
				sbt._v_futures.clear();

				sbt._v_futures.reserve(qmax);

				for (int i = 0; i < qmax; i++) {
					sbt._v_futures.emplace_back(tpool.EnqueueJob(
						std::bind(&sbTest::tpool_work2, &sbt, (i % 3 + 1) * 100, i))
					);
				}
				
				sbt.get_tpool_result();
				
				std::cout << "task: " << qmax << ", tact time: " << timer.t_end_milli() << " msec" << std::endl;
			}
			break;

        case 3:
            {
				/*
				std::string filename = "d:\\wandbox_para.txt";
				std::ifstream fin(filename);
				
				if(fin.fail())
				{
					std::cout << "file open error" << std::endl;
					break;
				}
				*/
				int num_tpool = 0;
		
				std::cout << "num_tpool...?  ";
				std::cin >> num_tpool;
				int second_param = 0;
				std::cin >> second_param;
				
				std::cout << num_tpool << " : " << second_param << std::endl;
				
				if(num_tpool <= 1 || num_tpool >= 30)
					num_tpool = 5;
				
                KoThreadPool tpool;
                if (!tpool.InitThreadPool(num_tpool))
                {
                    std::cerr << "Error : Init" << "\n";
                    exit(1);
                }
   
                gSum1 = 0;
                gSum2 = 0;

                MyClass myclass;

                int i = 0;
                int num_threads = 100;
                int timeout = 0;
                while (true)
                {
                    if (i >= num_threads)
                    {
                        break;
                    }

                    //class member
                    std::function<void()> temp_func1 = std::bind(&MyClass::MyThreadWork, &myclass);
                    tpool.AssignTask(temp_func1);

                    //free function
                    std::function<void()> temp_func2 = std::bind(&MyThreadWork);
                    tpool.AssignTask(temp_func2);
                    i++;
                }

                if(timeout < 1)
                {
                    tpool.Terminate(); //graceful terminate : wait until all work done
                    //tpool.Terminate(true); //true -->> terminate immediately
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));  // thread가 수행할 시간을 준다.
                }
                
                int a = gSum1; int b = gSum2;

                std::cout << "gSum1 = " << a << ", gSum2 = " << b << std::endl;     
				std::cout << "thread pool(" << num_tpool << "), tact time: " << timer.t_end_milli() << " msec" << std::endl;
            }
            break;
    }

    return 1;
}

#include <iostream>
#include <memory>
#include <vector>

class A {
  int *data;

 public:
  A() {
    data = new int[100];
    std::cout << "자원을 획득함!" << std::endl;
  }

  ~A() {
    std::cout << "소멸자 호출!" << std::endl;
    delete[] data;
  }
};

/* shared_ptr 사용 */ 
int main() {

    int case_num = 1;
    switch(case_num) 
    {
        case 0: /* shared_ptr 은 인자로 주소값이 전달된다면, 마치 자기가 해당 객체를 첫번째로 소유하는 shared_ptr 인 것 마냥 행동합니다. */
            {
				  A* a = new A();

				  std::shared_ptr<A> pa1(a);
				  std::shared_ptr<A> pa2(a);

				  std::cout << pa1.use_count() << std::endl;
				  std::cout << pa2.use_count() << std::endl;
				/* 소멸자가 두 번 호출되면서 오류가 나게 됩니다. */
            }
            break;
		case 1: /* shared_ptr 사용방법 1 */
			{
				std::shared_ptr<A> p1 = std::make_shared<A>();
				  std::cout << p1.use_count() << std::endl;
				
				std::shared_ptr<A> p2(p1);
				  std::cout << p2.use_count() << std::endl;
				std::shared_ptr<A> p3(p1);
				  std::cout << p3.use_count() << std::endl;
			}
			break;
        case 2: /* shared_ptr 사용방법 2 */
            {
			  std::vector<std::shared_ptr<A>> vec;

			  vec.push_back(std::shared_ptr<A>(new A()));
			  vec.push_back(std::shared_ptr<A>(vec[0]));
			  vec.push_back(std::shared_ptr<A>(vec[1]));
				
			  // 벡터의 첫번째 원소를 소멸 시킨다.
			  std::cout << vec.begin()->use_count() << std::endl;
			  std::cout << "첫 번째 소멸!" << std::endl;
			  vec.erase(vec.begin());

			  // 그 다음 원소를 소멸 시킨다.
			  std::cout << "다음 원소 소멸!" << std::endl;
			  std::cout << vec.begin()->use_count() << std::endl;
			  vec.erase(vec.begin());

			  // 마지막 원소 소멸
			  std::cout << "마지막 원소 소멸!" << std::endl;
			  std::cout << vec.begin()->use_count() << std::endl;
			  vec.erase(vec.begin());
			}
            break;
    }
  std::cout << "프로그램 종료!" << std::endl;
}

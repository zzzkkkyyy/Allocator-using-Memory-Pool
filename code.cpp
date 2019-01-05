#include <iostream>
#include <new>
#include <type_traits>
#include <vector>
#include <time.h>
#include <cstdlib>
#include <random>

using namespace std;

#define INT 1
#define FLOAT 2
#define DOUBLE 3
#define CLASS  4

template <class T> class MyAllocator;

typedef struct MemoryNode
{
	MemoryNode *next;
}MemoryNode;

class SizeList
{
public:
	MemoryNode *head;
	SizeList(MemoryNode *pt = 0) : head(pt) {}
};

template <class T>
class MyAllocator 
{
public:
	typedef void _Not_user_specialized;
	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef true_type propagate_on_container_move_assignment;
	typedef true_type is_always_equal;

	template <typename _other> struct rebind
	{
		typedef MyAllocator<_other> other;
	};

	MyAllocator() noexcept
	{
		
	}
	MyAllocator(const MyAllocator& other) noexcept
	{
		//MyAllocator();
	}
	template<class U>
	MyAllocator(const MyAllocator<U>& other) noexcept
	{
		//MyAllocator();
	}
	~MyAllocator() noexcept
	{

	}
	pointer address(reference x) const  noexcept
	{
		return &x;
	}
	const_pointer address(const_reference x) const noexcept
	{
		return &x;
	}
	char *MemoryAlloc(size_t size, size_t num)   //分配内存
	{
		size_t FreeMemory = PoolEnd - PoolStart;   //可供分配的内存
		if (FreeMemory >= size * num)   //如果可供分配的内存足够，就调整指针，分配内存
		{
			char *start = PoolStart;
			PoolStart += size * num;
			PoolSize -= size * num;
			return start;
		}
		else if (FreeMemory >= size)   //如果内存不够，但是足够开辟一块或者几块，则尽可能的使用
		{
			int current_num = static_cast<int>(FreeMemory) / size;   //计算可以开辟几块
			char *start = PoolStart;
			PoolStart += size * current_num;
			PoolSize -= size * current_num;
			return start;
		}
		else   //如果内存不够，则可能需要申请新的内存
		{
			size_t newbyte = 400 * size * num + adjustSize(PoolSize >> 4);   //所需开辟的新内存
			PoolStart = (char *)malloc(newbyte);   //使用内存池中的剩余内存开辟
			if (PoolStart == NULL)   //开辟失败，则需要申请新的内存
			{
				SizeList currentsizelist;
				for (int i = static_cast<int>(size * num + alignBits); i <= maxBytes; i += alignBits)
				{
					currentsizelist = sizelist[findIndex(i)];
					if (currentsizelist.head)
					{
						MemoryNode *temp = currentsizelist.head;
						PoolStart = reinterpret_cast<char *>(temp);
						PoolEnd = reinterpret_cast<char *>(temp + adjustSize(i));
						currentsizelist.head = (currentsizelist.head)->next;
						return MemoryAlloc(size, num);   //申请内存后重新分配
					}
				}
			}
			else   //内存池中剩余内存足够，则使用内存池中内存
			{
				PoolSize += newbyte;
				PoolEnd = PoolStart + newbyte;
				char *start = PoolStart;
				PoolStart += size * num;
				PoolSize = PoolEnd - PoolStart;
				return start;
			}
		}
	}
	size_t findIndex(size_t size)  //计算输入size之后，它应该存的index的位置
	{
		return (((size)-1) / (alignBits));
		return ((static_cast<int>(size) - 1) / static_cast<int>(alignBits));
	}
	size_t adjustSize(size_t size)	//size向上取整返回一个能被8整除的数字
	{
		return (findIndex(size) + 1) * alignBits;
	}
	void *addSizeNode(size_t size)    //重新分配一块地址给数组
	{
		char *addr = NULL;
		addr = MemoryAlloc(size, nodeAddNum);    //分配内存
		int temp = (PoolEnd - PoolStart) / size;
		size_t num = (nodeAddNum > temp) ? temp : nodeAddNum;    //计算需要分配的大小
		if (num > 1)    //将内存分配出去，并将剩余的地址串起来，将首地址存入数组
		{
			MemoryNode *head = reinterpret_cast<MemoryNode *>(addr + size);
			MemoryNode **temp = &(sizelist[findIndex(size)].head);
			for (int i = 0; i < num - 1; i++)
			{
				*temp = head;
				temp = &((*temp)->next);
				head = reinterpret_cast<MemoryNode *>(addr + size * (i + 2));
			}
			*temp = NULL;
		}
		return addr;
	}
	pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
	{    //分配内存的接口
		void *result;
		if (n == 0)
		{
			return static_cast<pointer>(NULL);
		}
		else if (n * sizeof(value_type) > maxBytes)    //如果需要分配的内存大于上限，则用malloc
		{
			return static_cast<pointer>(malloc(n * sizeof(value_type)));
		}
		else    //不大于上限，则用内存池分配内存
		{
			SizeList sizelisthead = sizelist[findIndex(n * sizeof(value_type))];   
			if (sizelisthead.head == NULL)
			{
				result = static_cast<void *>(addSizeNode(adjustSize(n * sizeof(value_type))));
			}
			else
			{
				result = sizelisthead.head;
				sizelisthead.head = (sizelisthead.head)->next;
			}
			return static_cast<T *>(result);
		}
	}
	void deallocate(pointer p, size_t n)    //将释放掉的内存重新存入内存池
	{
		if (p == NULL || n == 0)
		{
			return;
		}
		else
		{
			if (sizeof(value_type) * n > maxBytes)
			{
				free(reinterpret_cast<void *>(p));
			}
			else
			{
				size_t ind = findIndex(sizeof(value_type) * n);
				static_cast<MemoryNode *>(static_cast<void *>(p))->next = sizelist[ind].head;
				sizelist[ind].head = static_cast<MemoryNode *>(static_cast<void *>(p));
			}
			return;
		}
	}
	size_type max_size() const noexcept
	{
		return size_t(-1) / sizeof(value_type);
	}

	template<typename U>
	void construct(pointer p, const_reference val) {
		new((void*)p) U(val);
	}
	template <class U>
	void destroy(U* p)
	{
		p->~U();
	}
private:
	static const int maxBytes = 80000;  //地址申请上限，最多申请80000个byte的地址
	static const int alignBits = 8;		//以一个byte为单位，在计算偏移量时有用
	static const int nodeAddNum = 2;	//一次性最多再申请2个地址

	static SizeList sizelist[maxBytes / alignBits]; //计算存放内存池的数组的长度

	static char *PoolStart, *PoolEnd;
	static size_t PoolSize;
};

template <class T>
char* MyAllocator<T>::PoolStart = NULL;
template <class T>
char* MyAllocator<T>::PoolEnd = NULL;
template <class T>
size_t MyAllocator<T>::PoolSize = 0;
template <class T>
SizeList MyAllocator<T>::sizelist[maxBytes / alignBits] = { 0 };

template<typename T>
bool operator==(const MyAllocator<T>&, const MyAllocator<T>&)
{
	return true;
}

template<typename T>
bool operator!=(const MyAllocator<T>&, const MyAllocator<T>&)
{
	return false;
}


class myObject
{
public:
	myObject() : m_X(0), m_Y(0) {}
	myObject(int t1, int t2) :m_X(t1), m_Y(t2) {}
	myObject(const myObject &rhs) { m_X = rhs.m_X; m_Y = rhs.m_Y; }
	bool operator == (const myObject &rhs)
	{
		if ((rhs.m_X == m_X) && (rhs.m_Y == m_Y))
			return true;
		else
			return false;
	}
protected:
	int m_X;
	int m_Y;
};

#define TESTSIZE 10000

const int TestSize = 10000;
const int PickSize = 1000;

template<class T>
using Allocator = std::allocator<T>; 
using Point2D = std::pair<int, int>;


int main()    //测试
{        //设计随机种子
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, TestSize);
	int time1 = 0;    //用于记录总时长，计算平均时间
	int time2 = 0;

	cout << "test default allocator..." << endl;

	for (int count = 0; count < 12; count++)
	{
		long long start = (long long)clock();    //开始计时
		long long end;

		// vector creation，先测试默认的allocator
		using IntVec = std::vector<int, Allocator<int>>;
		std::vector<IntVec, Allocator<IntVec>> vecints(TestSize);
		for (int i = 0; i < TestSize; i++)    //生成一个int型的vector
			vecints[i].resize(dis(gen));

		using PointVec = std::vector<std::pair<int, int>, Allocator<std::pair<int, int>> >;
		std::vector<PointVec, Allocator<PointVec>> vecpts(TestSize);
		for (int i = 0; i < TestSize; i++)
			vecpts[i].resize(dis(gen));    //生成一个point型的vector

		// vector resize
		for (int i = 0; i < PickSize; i++)    //测试修改部分元素的大小
		{
			int idx = dis(gen) - 1;
			int size = dis(gen);
			vecints[idx].resize(size);
			vecpts[idx].resize(size);
		}

		// vector element assignment，测试两种vector的内存分配
		{
			int val = 10;
			int idx1 = dis(gen) - 1;
			int idx2 = vecints[idx1].size() / 2;
			vecints[idx1][idx2] = val;
			if (vecints[idx1][idx2] == val && count >= 2) 
				std::cout << "correct assignment in vecints: " << idx1 << std::endl;
			else if(count >= 2)
				std::cout << "incorrect assignment in vecints: " << idx1 << std::endl;
		}

		{
			std::pair<int, int> val(11, 15);
			int idx1 = dis(gen) - 1;
			int idx2 = vecpts[idx1].size() / 2;
			vecpts[idx1][idx2] = val;
			if (vecpts[idx1][idx2] == val && count >= 2)
				std::cout << "correct assignment in vecpts: " << idx1 << std::endl;
			else if(count >= 2)
				std::cout << "incorrect assignment in vecpts: " << idx1 << std::endl;
		}

		end = (long long)clock();    //停止计时
		if (count >= 2)
		{
			std::cout << "epoch: " << count - 1 << ", time: " << end - start << std::endl;
			time1 += end - start;    //记录总时长
		}
		vecints.clear();
		vecpts.clear();
	}

	cout << "test my allocator..." << endl;    //测试我们设计的Allocator，测试内容一致

	for (int count = 0; count < 12; count++)    //前两次测试的时间可能不稳定，不记录总时长
	{
		long long start = (long long)clock();
		long long end;

		// vector creation
		using IntVec = std::vector<int, MyAllocator<int>>;
		std::vector<IntVec, MyAllocator<IntVec>> vecints(TestSize);
		for (int i = 0; i < TestSize; i++)
			vecints[i].resize(dis(gen));

		using PointVec = std::vector<std::pair<int, int>, MyAllocator<std::pair<int, int>> >;
		std::vector<PointVec, MyAllocator<PointVec>> vecpts(TestSize);
		for (int i = 0; i < TestSize; i++)
			vecpts[i].resize(dis(gen));

		// vector resize
		for (int i = 0; i < PickSize; i++)
		{
			int idx = dis(gen) - 1;
			int size = dis(gen);
			vecints[idx].resize(size);
			vecpts[idx].resize(size);
		}

		// vector element assignment
		{
			int val = 10;
			int idx1 = dis(gen) - 1;
			int idx2 = vecints[idx1].size() / 2;
			vecints[idx1][idx2] = val;
			if (vecints[idx1][idx2] == val && count >= 2)
				std::cout << "correct assignment in vecints: " << idx1 << std::endl;
			else if (count >= 2)
				std::cout << "incorrect assignment in vecints: " << idx1 << std::endl;
		}

		{
			std::pair<int, int> val(11, 15);
			int idx1 = dis(gen) - 1;
			int idx2 = vecpts[idx1].size() / 2;
			vecpts[idx1][idx2] = val;
			if (vecpts[idx1][idx2] == val && count >= 2)
				std::cout << "correct assignment in vecpts: " << idx1 << std::endl;
			else if (count >= 2)
				std::cout << "incorrect assignment in vecpts: " << idx1 << std::endl;
		}

		end = (long long)clock();
		if (count >= 2)
		{
			std::cout << "epoch: " << count - 1 << ", time: " << end - start << std::endl;
			time2 += end - start;
		}
		vecints.clear();
		vecpts.clear();
	}
	//计算平均时长，效率提升情况，并输出
	std::cout << "\nAveragely, using default allocator costs: " << float(time1)/10 
		<< ". And my allocator just costs: " << float(time2)/10<< "." << endl 
		<< "My allocator can save " << 100-time2*100/time1 << "\% time." << endl;
	//system("pause");
	return 0;
}

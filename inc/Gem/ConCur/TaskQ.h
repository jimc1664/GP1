#ifndef GEM_CONCUR_TASKQ_H
#define GEM_CONCUR_TASKQ_H

#include "../Basic.h"

namespace Gem { 

class TaskQ {
	struct _declspec(align(16)) _TaskLink {
		union {
#ifdef _WINDOWS_
			SLIST_ENTRY Entry;
#endif //_WINDOWS_
			void * _Entry;
		};
	};	
public:
	struct Task : _TaskLink{
	protected:
		void freeMem();
		~Task() {}
		Template1 static T* alloc(){ return (T*)alloc(sizeof(T)); }
	private: friend class TaskQ;
		static ptr alloc(sizet s);
		virtual void proc(ptr cntx) = 0;
		virtual void destruct() = 0;
	};

	TaskQ();
	~TaskQ();
	void add(Task *t);
	void proc(ptr context);
	void procFwd(ptr context);
	bool procOne(ptr context);

private:	
	union {
#ifdef _WINDOWS_
		SLIST_HEADER Head;
#endif// _WINDOWS_
		u8 _Space[8];
	};
};

template<class Base>  class TaskHost {
public:
	class Task_Base : public TaskQ::Task { 
	protected:
		Task_Base() { TaskHost<Base>::task( this ); }
	};

	static void task( Task_Base *t ) {
		static_cast<TaskHost<Base>*>(&Singleton<Base>::Ptr)->Tasks.add(t);
	}
protected:
	TaskQ Tasks;
};

template<class T, class Cntx > struct TaskHlpr : public TaskHost<Cntx>::Task_Base {
	TaskHlpr( const T& f ) : Foo(f) {}
	void proc( ptr context ) override {
		Foo(*(Cntx*)context); 
		this->~TaskHlpr();
		freeMem();
	}
	void destruct() override { this->~TaskHlpr(); }
	static TaskHlpr* ctor( const T &a ) { return new(alloc<TaskHlpr>()) TaskHlpr(a); }
	T Foo;
};
#define TaskSchedule( Cntx, foo ) \
{ \
	auto lambda = [=](Cntx &cntx) { foo; }; \
	typedef decltype(lambda) LambdaT;\
	TaskHlpr<LambdaT,Cntx>::ctor( lambda ); \
} 0



}	

#endif //GEM_CONCUR_TASKQ_H
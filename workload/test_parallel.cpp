#include<omp.h>
#include<iostream>
#include"../other/atomic.hpp"
using namespace std;

void add_f(int id, int& flag)
{
	//++flag;
	// --flag;
	write_sub(&flag, 1);
	// write_add(&flag , 1);
	//cout << id << "--" << flag << endl;
}
int main()
{	
	int flag = 50000;
    omp_set_num_threads(20);
	#pragma omp parallel for
	for(int i=0;i<16000;++i)
	{
		add_f(omp_get_thread_num(), flag);
	}
	cout << flag << endl;
}
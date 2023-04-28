#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

int main(int argc, char *argv[])
{
	struct timeval begin_t, end_t;
	gettimeofday(&begin_t, NULL);//시작 시간을 가져옴

	ssu_score(argc, argv);

	gettimeofday(&end_t, NULL);//종료 시간을 가져옴
	ssu_runtime(&begin_t, &end_t);//프로그램 실행동안 소요한 시간을 출력해줌.

	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	end_t->tv_sec -= begin_t->tv_sec;//초 단위로 뺄셈

	if(end_t->tv_usec < begin_t->tv_usec){//usec는 micro second
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}

	end_t->tv_usec -= begin_t->tv_usec;
	printf("Runtime: %ld:%06d(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}

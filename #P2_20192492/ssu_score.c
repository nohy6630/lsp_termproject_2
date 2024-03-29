// add header files
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "ssu_score.h"
#include "blank.h"

extern struct ssu_scoreTable score_table[QNUM];
extern char id_table[SNUM][10];

struct ssu_scoreTable score_table[QNUM]; // 문제별 점수가 저장되어있는 구조체 배열
char id_table[SNUM][10];

char stuDir[BUFLEN];
char ansDir[BUFLEN];
char errorDir[BUFLEN];
char threadFiles[ARGNUM][FILELEN];
char printStudents[ARGNUM][FILELEN];
char iIDs[ARGNUM][FILELEN];

int eOption = false; // 각 option이 입력되면 true로 바뀜
int tOption = false;
int mOption = false;
int iOption = false;
int cOption = false;
int pOption = false;
int sOption = false;

char scoreFile[BUFLEN];
char sOptArg[2][100];

void ssu_score(int argc, char *argv[])
{
	char saved_path[BUFLEN];
	int i;
	int tmp_fd;

	tmp_fd = open("ssu_test.txt", O_RDONLY | O_CREAT | O_TRUNC, 0666);
	close(tmp_fd);
	tmp_fd = open("in.txt", O_RDONLY | O_CREAT | O_TRUNC, 0666);
	close(tmp_fd);

	for (i = 0; i < argc; i++)
	{
		if (!strcmp(argv[i], "-h"))
		{ //-h옵션이 있다면 사용법을 출력하고 함수 종료
			print_usage();
			return;
		}
	}

	memset(saved_path, 0, BUFLEN);
	if (argc >= 3 && strcmp(argv[1], "-i") != 0)
	{
		strcpy(stuDir, argv[1]); // stuDir 경로 값 할당
		strcpy(ansDir, argv[2]); // ansDir 경로 값 할당
		sprintf(scoreFile, "%s/score.csv", ansDir);
	}

	if (!check_option(argc, argv)) // 정의되지 않은 옵션인 경우에는 프로그램 종료시킴
		exit(1);

	if (!mOption && !eOption && !tOption && iOption && !strcmp(stuDir, "") && !strcmp(ansDir, ""))
	{
		do_iOption(iIDs); // i옵션 기능 실행
		return;
	}

	getcwd(saved_path, BUFLEN); // 현재 경로를 saved_path에 저장

	if (chdir(stuDir) < 0)
	{
		fprintf(stderr, "%s doesn't exist\n", stuDir); // stuDir이 존재하지 않을 경우 메시지 출력
		return;
	}
	getcwd(stuDir, BUFLEN); // 학생답안 디렉토리의 절대경로를 stuDir에 저장

	chdir(saved_path);
	if (chdir(ansDir) < 0)
	{
		fprintf(stderr, "%s doesn't exist\n", ansDir); // ansDir이 존재하지 않을 경우 메시지 출력
		return;
	}
	getcwd(ansDir, BUFLEN); // 정답 디렉토리의 절대경로를 stuDir에 저장

	chdir(saved_path);

	set_scoreTable(ansDir); // 점수테이블을 설정함(문제별 배점이 기록되있음)
	set_idTable(stuDir);	// 채점결과 테이블을 설정함

	if (mOption)
		do_mOption(); // 배점수정 기능인듯

	printf("grading student's test papers..\n");
	score_students(); // 이게 메인 로직인듯

	if (iOption)
		do_iOption(iIDs);

	return;
}

int check_option(int argc, char *argv[])
{
	int i, j, k;
	int c;
	int exist = 0;
	char tmp[BUFLEN];

	while ((c = getopt(argc, argv, "e:thmin:cps1")) != -1) // 옵션 처리
	{
		switch (c)
		{
		case '1':
			if(strcmp(sOptArg[1],"-1"))
			{
				fprintf(stderr, "error: 1 is not option. -1 is s option argument\n");
				exit(1);
			}
			break;
		case 's':
			sOption = true;
			i = optind; // getopt로 읽은 인덱스의 다음 인덱스
			j = 0;
			while (i < argc && (argv[i][0] != '-' || !strcmp(argv[i], "-1")))
			{
				if (i == optind)
				{
					if (!strcmp(argv[i], "stdid") || !strcmp(argv[i], "score"))
						strcpy(sOptArg[0], argv[i]);
					else
					{
						fprintf(stderr, "error: s option argument is stdid nor score\n");
						exit(1);
					}
				}
				else if (i == optind + 1)
				{
					if (!strcmp(argv[i], "1") || !strcmp(argv[i], "-1"))
						strcpy(sOptArg[1], argv[i]);
					else
					{
						fprintf(stderr, "error: s option argument is 1 nor -1\n");
						exit(1);
					}
				}
				else
				{
					fprintf(stderr, "error: too many argument in s option\n");
					exit(1);
				}
				i++;
			}
			if (sOptArg[0][0] == 0 || sOptArg[1][0] == 0)
			{
				fprintf(stderr, "error: s option need two arguments\n");
				exit(1);
			}
			break;
		case 'p':
			pOption = true;
			i = optind; // getopt로 읽은 인덱스의 다음 인덱스
			j = 0;

			while (i < argc && argv[i][0] != '-')
			{ // 옵션을 제외한 인자들 순회, 다른 옵션을 만나면 반복 중지

				if (j >= ARGNUM) // 인자 갯수 초과
					printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
				else
				{
					sprintf(tmp, "%s/%s", stuDir, argv[i]);
					if (access(tmp, F_OK) != 0)
					{
						fprintf(stderr, "error: %s student is not exist\n", argv[i]);
						exit(1);
					}
					if (printStudents[j][0] != 0)
					{
						fprintf(stderr, "error: too many variable argument groups\n");
						exit(1);
					}
					strcpy(printStudents[j], argv[i]);
				}
				i++;
				j++;
			}
			break;
		case 'c':
			cOption = true;
			i = optind; // getopt로 읽은 인덱스의 다음 인덱스
			j = 0;

			while (i < argc && argv[i][0] != '-')
			{					 // 옵션을 제외한 인자들 순회, 다른 옵션을 만나면 반복 중지
				if (j >= ARGNUM) // 인자 갯수 초과
					printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
				else
				{
					sprintf(tmp, "%s/%s", stuDir, argv[i]);
					if (access(tmp, F_OK) != 0)
					{
						fprintf(stderr, "error: %s student is not exist\n", argv[i]);
						exit(1);
					}
					if (printStudents[j][0] != 0)
					{
						fprintf(stderr, "error: too many variable argument groups\n");
						exit(1);
					}
					strcpy(printStudents[j], argv[i]);
				}
				i++;
				j++;
			}
			break;
		case 'm':
			mOption = true;
			break;
		case 'n':
			realpath(optarg, scoreFile);
			if (strrchr(scoreFile, '.') == NULL || strcmp(strrchr(scoreFile, '.'), ".csv"))
			{
				fprintf(stderr, "error: score file path is invalid\n");
				exit(1);
			}
			break;
		case 'e': // 에러메시지 출력 옵션
			eOption = true;
			strcpy(errorDir, optarg); // errorDir에 입력받은 경로 저장

			if (access(errorDir, F_OK) < 0)
			{
				if (mkdir(errorDir, 0755) == -1)
				{
					fprintf(stderr, "error: errorDir path is invalid\n");
					exit(1);
				}
			}
			else
			{
				rmdirs(errorDir); // 디렉토리가 이미 존재한다면 지워줌
				if (mkdir(errorDir, 0755) == -1)
				{
					fprintf(stderr, "error: errorDir path is invalid\n");
					exit(1);
				}
			}
			break;

		case 't': // 컴파일시 -lpthread옵션 추가하는 옵션
			tOption = true;
			i = optind; // getopt로 읽은 인덱스의 다음 인덱스
			j = 0;

			while (i < argc && argv[i][0] != '-')
			{ // 다음 옵션을 만날때까지 순회

				if (j >= ARGNUM) // 인자 갯수 초과
					printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
				else
				{
					sprintf(tmp, "%s/%s.c", ansDir, argv[i]);
					if (access(tmp, F_OK) != 0)
					{
						fprintf(stderr, "error: problem %s is not exist or can't compiled\n", argv[i]);
						exit(1);
					}
					strcpy(threadFiles[j], argv[i]); // 인자들을 순서대로 threadFiles배열에 저장
				}
				i++;
				j++;
			}
			break;

		case 'i': // 이번 과제에서는 사라진 옵션임
			iOption = true;
			i = optind;
			j = 0;

			while (i < argc && argv[i][0] != '-')
			{
				if (j >= ARGNUM)
					printf("Maximum Number of Argument Exceeded. :: %s\n", argv[i]);
				else
					strcpy(iIDs[j], argv[i]);
				i++;
				j++;
			}
			break;

		case '?': // 이상한 옵션이 들어오거나 옵션 형식이 맞지 않는 경우
			printf("Unkown option %c\n", optopt);
			return false;
		}
	}

	return true; // 입력된 옵션들이 모두 유효하기 때문에 true 반환
}

void do_iOption(char (*ids)[FILELEN])
{
	FILE *fp;
	char tmp[BUFLEN];
	char qname[QNUM][FILELEN];
	char *p, *id;
	int i, j;
	char first, exist;

	if ((fp = fopen(scoreFile, "r")) == NULL)
	{
		fprintf(stderr, "%s file doesn't exist\n", scoreFile);
		return;
	}

	// get qnames
	i = 0;
	fscanf(fp, "%s\n", tmp);
	strcpy(qname[i++], strtok(tmp, ","));

	while ((p = strtok(NULL, ",")) != NULL)
		strcpy(qname[i++], p);

	// print result
	i = 0;
	while (i++ <= ARGNUM - 1)
	{
		exist = 0;
		fseek(fp, 0, SEEK_SET);
		fscanf(fp, "%s\n", tmp);

		while (fscanf(fp, "%s\n", tmp) != EOF)
		{
			id = strtok(tmp, ",");

			if (!strcmp(ids[i - 1], id))
			{
				exist = 1;
				j = 0;
				first = 0;
				while ((p = strtok(NULL, ",")) != NULL)
				{
					if (atof(p) == 0)
					{
						if (!first)
						{
							printf("%s's wrong answer :\n", id);
							first = 1;
						}
						if (strcmp(qname[j], "sum"))
							printf("%s    ", qname[j]);
					}
					j++;
				}
				printf("\n");
			}
		}

		if (!exist)
			printf("%s doesn't exist!\n", ids[i - 1]);
	}

	fclose(fp);
}

void do_mOption()
{
	double newScore;
	char modiName[FILELEN];
	char filename[FILELEN];
	char *ptr;
	int i;

	ptr = malloc(sizeof(char) * FILELEN);

	while (1)
	{

		printf("Input question's number to modify >> ");
		scanf("%s", modiName);

		if (strcmp(modiName, "no") == 0)
			break;

		for (i = 0; i < sizeof(score_table) / sizeof(score_table[0]); i++)
		{
			strcpy(ptr, score_table[i].qname);
			ptr = strtok(ptr, ".");
			if (!strcmp(ptr, modiName))
			{
				printf("Current score : %.2f\n", score_table[i].score);
				printf("New score : ");
				scanf("%lf", &newScore);
				getchar();
				score_table[i].score = newScore;
				break;
			}
		}
	}

	sprintf(filename, "%s/%s", ansDir, "score_table.csv");
	write_scoreTable(filename); // 바뀐 점수를 csv파일에 다시 갱신하는 함수 호출
	free(ptr);
}

int is_exist(char (*src)[FILELEN], char *target)
{
	int i = 0;

	while (1)
	{
		if (i >= ARGNUM)
			return false;
		else if (!strcmp(src[i], ""))
			return false;
		else if (!strcmp(src[i++], target))
			return true;
	}
	return false;
}

void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	sprintf(filename, "%s/%s", ansDir, "score_table.csv");

	// check exist
	if (access(filename, F_OK) == 0)
		read_scoreTable(filename); // scoreTable이 이미 존재한다면 그대로 읽어오기
	else
	{
		make_scoreTable(ansDir);	// scoreTable만들기
		write_scoreTable(filename); // scoreTable 내 데이터 작성하기
	}
}

void read_scoreTable(char *path)
{
	FILE *fp;
	char qname[FILELEN];
	char score[BUFLEN];
	int idx = 0;

	if ((fp = fopen(path, "r")) == NULL)
	{
		fprintf(stderr, "file open error for %s\n", path);
		return;
	}

	while (fscanf(fp, "%[^,],%s\n", qname, score) != EOF)
	{ //,전 까지의 문자를 qname 이후 문자열은 score에 저장
		strcpy(score_table[idx].qname, qname);
		score_table[idx++].score = atof(score);
	}

	fclose(fp);
}

void make_scoreTable(char *ansDir)
{
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp, *c_dirp;
	DIR *dp, *c_dp;
	char *tmp;
	int idx = 0;
	int i;

	num = get_create_type(); // score table의 문제별 점수를 어떻게 부여할건지 사용자의 입력을 받아서 정함

	if (num == 1)
	{
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}

	if ((dp = opendir(ansDir)) == NULL)
	{
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}

	while ((dirp = readdir(dp)) != NULL)
	{ // ansDir 내부 파일 순회

		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		if ((type = get_file_type(dirp->d_name)) < 0) //.txt나 .c파일 이아니면 무시
			continue;

		strcpy(score_table[idx].qname, dirp->d_name); //.txt 파일, .c파일들의 이름을 score_table[idx].qname에 저장

		idx++;
	}

	closedir(dp);
	sort_scoreTable(idx); // 문제번호 순서대로 scoreTable 정렬

	for (i = 0; i < idx; i++)
	{
		type = get_file_type(score_table[i].qname);

		if (num == 1)
		{
			if (type == TEXTFILE)
				score = bscore;
			else if (type == CFILE)
				score = pscore;
		}
		else if (num == 2)
		{
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}

		score_table[i].score = score;
	}
}

void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);

	if ((fd = creat(filename, 0666)) < 0)
	{
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for (i = 0; i < num; i++)
	{
		if (score_table[i].score == 0) // score_table[i].score의 값을 설정하지 않았다면 기본값이 0그대로이므로 이 인덱스부터는 점수가 등록되지 않았으므로 루프 중단
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp));
	}

	close(fd);
}

void set_idTable(char *stuDir)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	if ((dp = opendir(stuDir)) == NULL)
	{
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while ((dirp = readdir(dp)) != NULL)
	{
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		stat(tmp, &statbuf);

		if (S_ISDIR(statbuf.st_mode))
			strcpy(id_table[num++], dirp->d_name); // 학생의 학번을 id_table에 저장
		else
			continue;
	}
	closedir(dp);

	sort_idTable(num);
}

void sort_idTable(int size)
{
	int i, j;
	char tmp[10];

	for (i = 0; i < size - 1; i++)
	{
		for (j = 0; j < size - 1 - i; j++)
		{
			if (strcmp(id_table[j], id_table[j + 1]) > 0)
			{ // id_table을 학번을 기준으로 오름차순 정렬
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j + 1]);
				strcpy(id_table[j + 1], tmp);
			}
		}
	}
}

void sort_scoreTable(int size)
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	for (i = 0; i < size - 1; i++)
	{
		for (j = 0; j < size - 1 - i; j++)
		{

			get_qname_number(score_table[j].qname, &num1_1, &num1_2);	  // 문제 번호를 가져옴
			get_qname_number(score_table[j + 1].qname, &num2_1, &num2_2); // 문제 번호를 가져옴

			if ((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2)))
			{ // 문제 번호의 오름차순으로 scoretable을 정렬

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j + 1], sizeof(score_table[0]));
				memcpy(&score_table[j + 1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];

	strncpy(dup, qname, strlen(qname));
	*num1 = atoi(strtok(dup, "-."));

	p = strtok(NULL, "-.");
	if (p == NULL)
		*num2 = 0;
	else
		*num2 = atoi(p); // 번호의 서브 문제가 있을경우 num2에 저장 시킴
}

int get_create_type()
{
	int num;

	while (1)
	{
		printf("score_table.csv file doesn't exist in TREUDIR!\n");
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);

		if (num != 1 && num != 2)
			printf("not correct number!\n");
		else
			break;
	}

	return num;
}

void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);
	stuScore *head = NULL;

	if ((fd = creat(scoreFile, 0666)) < 0)
	{
		fprintf(stderr, "creat error for %s", scoreFile);
		return;
	}
	write_first_row(fd);

	for (num = 0; num < size; num++)
	{
		if (!strcmp(id_table[num], ""))
			break;

		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp));

		stuScore *new = (stuScore *)malloc(sizeof(stuScore));
		new->next = NULL;
		strcpy(new->scoId, id_table[num]);
		score += score_student(fd, id_table[num], new); // 점수를 채점하면서 점수들 정보를 new노드에 저장
		push_list_2(&head, new);						// 링크드리스트에 학생채점정보 노드 추가
	}

	if (cOption)
		printf("Total average : %.2f\n", score / num);
	char resPath[BUFLEN];
	char errPath[BUFLEN];
	realpath(scoreFile, resPath);
	printf("result saved.. (%s)\n", resPath);
	if (eOption)
	{
		realpath(errorDir, errPath);
		printf("error saved.. (%s)\n", errPath);
	}

	if (sOption) // sOption이라면 링크드리스트를 정렬시키고 그 순서대로 다시 채점결과를 csv파일에 기록함
	{
		sort_list(head);
		lseek(fd, 0, SEEK_SET);
		ftruncate(fd,0);
		write_first_row(fd);
		write_sort_res(fd, head);
	}

	free_list_2(head); // 링크드리스트 메모리 해제
	close(fd);
}

double score_student(int fd, char *id, stuScore *node)
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN], tmp2[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);
	wrongProblem *head = NULL;

	for (i = 0; i < size; i++)
	{
		if (score_table[i].score == 0) // score_table 데이터 모두 읽음
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname); // stuDir/학번/답안파일이름

		if (access(tmp, F_OK) < 0)
			result = false;
		else
		{
			if ((type = get_file_type(score_table[i].qname)) < 0)
				continue;

			if (type == TEXTFILE)
				result = score_blank(id, score_table[i].qname); // 텍스트파일이면 빈칸채우기문제 로직
			else if (type == CFILE)
				result = score_program(id, score_table[i].qname); // c파일이면 소스코드문제 로직
		}

		if (result != true)
		{
			memset(tmp2, 0, sizeof(tmp2));
			memcpy(tmp2, score_table[i].qname, strlen(score_table[i].qname) - strlen(strrchr(score_table[i].qname, '.')));
			push_list(&head, tmp2, score_table[i].score);
		}

		if (result == false)
		{
			write(fd, "0,", 2);
			node->sco[i] = 0;
		}
		else
		{
			if (result == true)
			{
				score += score_table[i].score;
				node->sco[i] = score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			else if (result < 0)
			{
				score = score + score_table[i].score + result; // result는 감점 점수인듯
				node->sco[i] = score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}

	printf("%s is finished..", id);
	if (printStudents[0][0] == 0)
	{
		if (cOption && pOption)
		{
			printf(" score : %.2f, wrong problem : ", score);
			print_list(head);
		}
		else if (cOption)
			printf(" score : %.2f", score);
		else if (pOption)
		{
			printf(" wrong problem : ");
			print_list(head);
		}
	}
	else
	{
		for (int i = 0; i < ARGNUM; i++)
		{
			if (printStudents[i][0] == 0)
				break;
			if (!strcmp(id, printStudents[i]))
			{
				if (cOption && pOption)
				{
					printf(" score : %.2f, wrong problem : ", score);
					print_list(head);
				}
				else if (cOption)
					printf(" score : %.2f", score);
				else if (pOption)
				{
					printf(" wrong problem : ");
					print_list(head);
				}
			}
		}
	}
	free_list(head);
	printf("\n");

	sprintf(tmp, "%.2f\n", score); // 다음 행으로 전환
	node->scoSum = score;
	write(fd, tmp, strlen(tmp));

	return score;
}

void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	write(fd, ",", 1);

	for (i = 0; i < size; i++)
	{
		if (score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,", score_table[i].qname);
		write(fd, tmp, strlen(tmp));
	}
	write(fd, "sum\n", 4);
}

char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while (read(fd, &c, 1) > 0)
	{
		if (c == ':') //: 이후 부터는 또 다른 정답이므로 여기서 break
			break;

		result[idx++] = c;
	}
	if (result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0';

	return result;
}

int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.'))); // 확장자를 제외하고 문제파일이름을 qname에 복사

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	fd_std = open(tmp, O_RDONLY);
	strcpy(s_answer, get_answer(fd_std, s_answer));

	if (!strcmp(s_answer, ""))
	{ // 답안이 비어있다면 오답처리
		close(fd_std);
		return false;
	}

	if (!check_brackets(s_answer))
	{ // 괄호 형식이 유효하지 않다면 오답처리
		close(fd_std);
		return false;
	}

	strcpy(s_answer, ltrim(rtrim(s_answer))); // 양옆 공백 제거

	if (s_answer[strlen(s_answer) - 1] == ';')
	{ // 세미콜론이 있을경우 지우기
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	if (!make_tokens(s_answer, tokens))
	{ // 토큰들로 분리
		close(fd_std);
		return false;
	}

	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0); // 토큰들로 트리를 만듬

	sprintf(tmp, "%s/%s", ansDir, filename);
	fd_ans = open(tmp, O_RDONLY);

	while (1)
	{
		ans_root = NULL;
		result = true;

		for (idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		strcpy(a_answer, get_answer(fd_ans, a_answer));

		if (!strcmp(a_answer, ""))
			break;

		strcpy(a_answer, ltrim(rtrim(a_answer)));

		if (has_semicolon == false)
		{
			if (a_answer[strlen(a_answer) - 1] == ';')
				continue;
		}

		else if (has_semicolon == true)
		{
			if (a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';
		}

		if (!make_tokens(a_answer, tokens)) // 정답문자열을 토큰화
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0); // 토큰들로 트리를 만듬

		compare_tree(std_root, ans_root, &result);

		if (result == true)
		{ // 정답일 경우 메모리 정리해주고 true return
			close(fd_std);
			close(fd_ans);

			if (std_root != NULL)
				free_node(std_root);
			if (ans_root != NULL)
				free_node(ans_root);
			return true;
		}
	}

	close(fd_std);
	close(fd_ans);

	if (std_root != NULL)
		free_node(std_root);
	if (ans_root != NULL)
		free_node(ans_root);

	return false; // 오답일 경우 false return
}

double score_program(char *id, char *filename)
{
	double compile;
	int result;

	compile = compile_program(id, filename); // 컴파일시 에러가 있을경우 compile은 0, warning이 있을경우 그에 비례하여 음수로 나타남.

	if (compile == ERROR || compile == false) // 컴파일 에러시 오답 처리
		return false;

	result = execute_program(id, filename);

	if (!result)
		return false;

	if (compile < 0)
		return compile;

	return true;
}

int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	if (threadFiles[0][0] == 0)
		return true;

	for (i = 0; i < size; i++)
	{
		if (!strcmp(threadFiles[i], qname))
			return true;
	}
	return false;
}

double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.'))); // filename에서 확장자명을 지워 qname에 저장

	isthread = is_thread(qname); //-t옵션을 적용할 파일인지 체크하는 함수

	sprintf(tmp_f, "%s/%s", ansDir, filename);
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname);

	if (tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
	fd = creat(tmp_e, 0666);

	redirection(command, fd, STDERR); // 표준에러 디스크립터를 fd로 임시로 바꿔서 gcc명령여를 실행함. 즉 에러코드가 fd와 연결된 tmp_e파일에 들어가게 됨.
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	unlink(tmp_e);

	if (size > 0) // 오류 메시지가 있을경우 false return
		return false;

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if (tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if (size > 0)
	{				 // 오류 메시지 있을경우
		if (eOption) // e옵션이 있을경우 디렉토리를 만들어 그안에 에러메시지가 담겨있는 파일을 저장함
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if (access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			if (rename(tmp_f, tmp_e) == -1)
			{
				fprintf(stderr, "error: errorFile path is invalid\n");
				exit(1);
			}

			result = check_error_warning(tmp_e);
		}
		else
		{
			result = check_error_warning(tmp_f); // warning만큼 감점을 함 또는 error가 있을경우 그냥 0
			unlink(tmp_f);						 // e옵션 없으므로 에러메시지 파일 지움
		}

		return result;
	}

	unlink(tmp_f);
	return true;
}

double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if ((fp = fopen(filename, "r")) == NULL)
	{
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while (fscanf(fp, "%s", tmp) > 0)
	{
		if (!strcmp(tmp, "error:"))
			return ERROR;
		else if (!strcmp(tmp, "warning:"))
			warning += WARNING;
	}

	return warning;
}

int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname);
	fd = creat(ans_fname, 0666);

	sprintf(tmp, "%s/%s.exe", ansDir, qname);
	redirection(tmp, fd, STDOUT); // tmp를 실행하고 표준출력을 stdout파일에 하도록 지정
	close(fd);

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);

	start = time(NULL);
	redirection(tmp, fd, STDOUT); // tmp를 실행하고 표준출력을 stdout파일에 하도록 지정

	sprintf(tmp, "%s.stdexe", qname);
	while ((pid = inBackground(tmp)) > 0)
	{
		end = time(NULL);

		if (difftime(end, start) > OVER)
		{
			kill(pid, SIGKILL);
			close(fd);
			return false;
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname); // 실행결과가 저장된 파일끼리 비교
}

pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;

	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	sprintf(command, "ps | grep %s", name);
	redirection(command, fd, STDOUT);

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp));

	if (!strcmp(tmp, ""))
	{
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	return pid;
}

int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while (1)
	{
		while ((len1 = read(fd1, &c1, 1)) > 0)
		{
			if (c1 == ' ') // 공백일 경우 건너뛰기
				continue;
			else
				break;
		}
		while ((len2 = read(fd2, &c2, 1)) > 0)
		{
			if (c2 == ' ')
				continue;
			else
				break;
		}

		if (len1 == 0 && len2 == 0)
			break;

		to_lower_case(&c1); // 전부 소문자로 바꾸기
		to_lower_case(&c2);

		if (c1 != c2)
		{
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true;
}

void redirection(char *command, int new, int old)
{
	int saved;

	saved = dup(old);
	dup2(new, old);

	system(command);

	dup2(saved, old);
	close(saved);
}

int get_file_type(char *filename)
{
	char *extension = strrchr(filename, '.'); //.을 기준으로 오른쪽에있는 문자열만 가져오는듯

	if (!strcmp(extension, ".txt"))
		return TEXTFILE;
	else if (!strcmp(extension, ".c"))
		return CFILE;
	else
		return -1;
}

void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[50];

	if ((dp = opendir(path)) == NULL)
		return;

	while ((dirp = readdir(dp)) != NULL)
	{
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if (lstat(tmp, &statbuf) == -1)
			continue;

		if (S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);
		else
			unlink(tmp);
	}

	closedir(dp);
	rmdir(path);
}

void to_lower_case(char *c)
{
	if (*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

void print_usage()
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -m                modify question's score\n");
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	printf(" -t <IDS>          print ID's wrong questions\n");
	printf(" -h                print usage\n");
}

void push_list(wrongProblem **list, char *qname, double score)
{
	wrongProblem *new = malloc(sizeof(wrongProblem));
	strcpy(new->qname, qname);
	new->score = score;
	new->next = NULL;
	if (*list == NULL)
		*list = new;
	else
	{
		wrongProblem *node = *list;
		while (node->next != NULL)
			node = node->next;
		node->next = new;
	}
}

void print_list(wrongProblem *head)
{
	if (head == NULL)
		printf("head is null");
	while (head != NULL)
	{
		printf("%s(%g)", head->qname, head->score);
		if (head->next != NULL)
			printf(", ");
		head = head->next;
	}
}

void free_list(wrongProblem *head)
{
	if (head == NULL)
		return;
	if (head->next != NULL)
		free_list(head->next);
	free(head);
}

void push_list_2(stuScore **list, stuScore *new)
{
	if (*list == NULL)
		*list = new;
	else
	{
		stuScore *node = *list;
		while (node->next != NULL)
			node = node->next;
		node->next = new;
	}
}

void free_list_2(stuScore *head)
{
	if (head == NULL)
		return;
	if (head->next != NULL)
		free_list_2(head->next);
	free(head);
}

void swap_node(stuScore *a, stuScore *b)//링크드리스트 안 두 노드의 데이터를 스왑하는 함수
{
	stuScore tmp;
	
	tmp = *a;
	*a=*b;
	a->next=tmp.next;//next값은 swap이 되지 않고 그대로
	tmp.next=b->next;//next값은 swap이 되지 않고 그대로
	*b = tmp;
}

void sort_list(stuScore *head)
{
	int cnt = 0;
	while (score_table[cnt].score != 0)//총 문제 갯수 구하기
		cnt++;
	for (int i = 0; i < cnt-1; i++)//버블정렬이기에 n-1번 반복하여 리스트를 순회
	{
		stuScore *cur = head;
		while (cur!=NULL&&cur->next != NULL)
		{
			if (!strcmp(sOptArg[0], "stdid"))//학번을 기준으로 정렬
			{
				if (!strcmp(sOptArg[1], "1"))//오름차순 정렬
				{
					if (strcmp(cur->scoId, cur->next->scoId) > 0)
						swap_node(cur, cur->next);
				}
				else//내림차순 정렬
				{
					if (strcmp(cur->scoId, cur->next->scoId) < 0)
						swap_node(cur, cur->next);
				}
			}
			else//총점수를 기준으로 정렬
			{
				if (!strcmp(sOptArg[1], "1"))//오름차순 정렬
				{
					if (cur->scoSum > cur->next->scoSum)
						swap_node(cur, cur->next);
				}
				else//내림차순 정렬
				{
					if (cur->scoSum < cur->next->scoSum)
						swap_node(cur, cur->next);
				}
			}
			cur = cur->next;
		}
	}
}

void write_sort_res(int fd, stuScore *head)
{
	int cnt = 0;
	char tmp[BUFLEN];
	stuScore *cur = head;
	while (score_table[cnt].score != 0)//문제의 총갯수 구하기
		cnt++;
	while (cur != NULL)//정렬된 링크드리스트를 순회하면서 각 행에 학생들의 정보를 출력
	{
		sprintf(tmp, "%s,", cur->scoId);
		write(fd, tmp, strlen(tmp));
		for (int i = 0; i < cnt; i++)
		{
			if (cur->sco[i] == 0)
				write(fd, "0,", 2);//0점이라면 그냥 0 출력
			else
			{
				sprintf(tmp, "%.2f,", cur->sco[i]);
				write(fd, tmp, strlen(tmp));//0점이 아니라면 그 점수를 형식에 맞게 출력
			}
		}
		sprintf(tmp, "%.2f\n", cur->scoSum);//총합 출력후 개행
		write(fd, tmp, strlen(tmp));
		cur = cur->next;
	}
}

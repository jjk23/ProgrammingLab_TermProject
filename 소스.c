//  프로그래밍랩 - 10주
// 이 프로그램은 gotoxy()를 이용하여
// keyboard 방향키를 누르면 * 모양이 움직입니다.
// 이 프로그램을 이용하여 Term Project 에 활용하기 바랍니다.
//
#define _CRT_SECCURE_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <Windows.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <process.h>
#pragma comment(lib, "winmm.lib")

// 색상 정의
#define BLACK	0
#define BLUE1	1
#define GREEN1	2
#define CYAN1	3
#define RED1	4
#define MAGENTA1 5
#define YELLOW1	6
#define GRAY1	7
#define GRAY2	8
#define BLUE2	9
#define GREEN2	10
#define CYAN2	11
#define RED2	12
#define MAGENTA2 13
#define YELLOW2	14
#define WHITE	15

#define ESC 0x1b

#define UP  0x48 // Up key는 0xe0 + 0x48 두개의 값이 들어온다.
#define DOWN 0x50
#define LEFT 0x4b
#define RIGHT 0x4d

int x = 100, y = 39; //현재 위치를 저장
int movetype = 0;//0은 못움직임. 1은 빨간하트. 2는 파란하트
int n = 0;//공격 패턴 몇 번째인지 저장
int life = 5;//라이프 저장. 0이 되면 게임종료
int canHit = 1;//피해를 받을 수 있는지.(0이면 무적상태)
int pattern = 0;//패턴을 3개정도 만들고 10번정도 반복하면 필살기로(패턴 식별 코드) 1부터 시작
int rand1, rand2;//랜덤값 2개를 저장해놓는 전역변수
char gravity = 'y';//y는 -수직 방향으로, x는 +수평 방향으로 중력 작용
int war = 0;//경고때만 잠시 노데미지(1)
int r = 3;//패턴 3회동안 살면 엔딩
double speed = 0.0;
clock_t start_time;//패턴 시작시간 저장
clock_t abtime;//무적 시간초 세기

HANDLE sfx_thread = NULL;
void stopBGM() 
{
	mciSendStringA("stop bgm", NULL, 0, NULL);
	mciSendStringA("close bgm", NULL, 0, NULL);
}

void playBGM(const char* filename) //MCI기술 기반으로 만든 함수
{
	char cmd[1024];//cmd명령어를 저장할 변수(넉넉하게 1024는 잡아야함)
	mciSendStringA("close bgm", NULL, 0, NULL);//이미 재생중인 BGM이 있다면 닫음(중복재생 방지)

	sprintf_s(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias bgm", filename);//파일을 bgm이라는 이름으로 열기. mpegvideo는 mp3라는 뜻.
	mciSendStringA(cmd, NULL, 0, NULL);
	mciSendStringA("play bgm repeat", NULL, 0, NULL);//repeat는 무한 반복을 의미.
}

DWORD WINAPI Thread(LPVOID p) 
{
	char* filename = (char*)p;//매개변수로 받은 파일명 포인터를 char* 형태로 변환해서 저장
	char cmd[1024];

	sprintf_s(cmd, sizeof(cmd), "open \"%s\" type waveaudio alias sfx_temp", filename); //wav를 sfx_temp라는 별명으로 열기
	mciSendStringA(cmd, NULL, 0, NULL);
	sprintf_s(cmd, sizeof(cmd), "play sfx_temp");//재생 명령어
	mciSendStringA("play sfx_temp wait", NULL, 0, NULL);//소리가 끝날때까지 스레드를 멈추고 대기함
	mciSendStringA("close sfx_temp", NULL, 0, NULL);//소리가 끝나면 파일 닫기

	free(filename);//복사해둔 메모리 해제
	return 0;
}

void playSFX(const char* filename) 
{
	char* copy = _strdup(filename);  // 파일명 복사
	CreateThread(NULL, 0, Thread, copy, 0, NULL);//별도 스레드를 생성해서 재생
}

void removeCursor(void) { // 커서를 안보이게 한다

	CONSOLE_CURSOR_INFO curInfo;
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	curInfo.bVisible = 0;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}

void gotoxy(int x, int y) //내가 원하는 위치로 커서 이동
{
	COORD pos = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);// WIN32API 함수입니다. 이건 알필요 없어요
}

TCHAR GetCharAt(int x, int y) 
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);//콘솔 화면을 제어할 수 있는 핸들 가져옴(이게 있어야 화면 제어 가능)
	COORD coord = { (SHORT)x, (SHORT)y };//좌표(x,y)를 저장할 수 있는 변수 생성
	TCHAR ch;//반환할 값을 저장할 ch변수
	DWORD dwRead = 0;//ReadConsoleOutputCharacter함수를 실행하기 위해 실제로 몇 개의 문자를 읽었는지를 저장할 변수 선언.
	ReadConsoleOutputCharacter(hConsole, &ch, 1, coord, &dwRead);
	//hConsole(현재 콘솔)에서 읽어올 문자를 ch에 포인터로 저장, coord위치에서 읽음, 실제 몇 개 읽었는지 저장할 변수(dwRead)의 주소 할당
	return ch;
}

int IsSpaceAt(int x, int y)//isspace라는 함수랑 헷갈리지 않도록 주의 
{
	TCHAR ch = GetCharAt(x, y); //해당 좌표의 문자를 불러옴
	if (ch == ' ') 
	{
		return 1; //공백이면 1
	}
	return 0; //문자가 있으면 0
}


void textcolor(int fg_color, int bg_color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), fg_color | bg_color << 4);
}
void drawTitle(int x, int y) 
{
	gotoxy(x, y); printf(" █████  █████ ██████   █████ ██████████   ██████████ ███████████   ███████████   █████████   █████       ██████████");
	gotoxy(x, y + 1); printf("▒▒███  ▒▒███ ▒▒██████ ▒▒███ ▒▒███▒▒▒▒███ ▒▒███▒▒▒▒▒█▒▒███▒▒▒▒▒███ ▒█▒▒▒███▒▒▒█  ███▒▒▒▒▒███ ▒▒███       ▒▒███▒▒▒▒▒█");
	gotoxy(x, y + 2); printf(" ▒███   ▒███  ▒███▒███ ▒███  ▒███   ▒▒███ ▒███  █ ▒  ▒███    ▒███ ▒   ▒███  ▒  ▒███    ▒███  ▒███        ▒███  █ ▒ ");
	gotoxy(x, y + 3); printf(" ▒███   ▒███  ▒███▒▒███▒███  ▒███    ▒███ ▒██████    ▒██████████      ▒███     ▒███████████  ▒███        ▒██████   ");
	gotoxy(x, y + 4); printf(" ▒███   ▒███  ▒███ ▒▒██████  ▒███    ▒███ ▒███▒▒█    ▒███▒▒▒▒▒███     ▒███     ▒███▒▒▒▒▒███  ▒███        ▒███▒▒█   ");
	gotoxy(x, y + 5); printf(" ▒███   ▒███  ▒███  ▒▒█████  ▒███    ███  ▒███ ▒   █ ▒███    ▒███     ▒███     ▒███    ▒███  ▒███      █ ▒███ ▒   █");
	gotoxy(x, y + 6); printf(" ▒▒████████   █████  ▒▒█████ ██████████   ██████████ █████   █████    █████    █████   █████ ███████████ ██████████");
	gotoxy(x, y + 7); printf("  ▒▒▒▒▒▒▒▒   ▒▒▒▒▒    ▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒   ▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒   ▒▒▒▒▒    ▒▒▒▒▒    ▒▒▒▒▒   ▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒ ");

	gotoxy(x+50, y + 30); printf("Press Any Key To Start");
}
void drawbox()
{
	gotoxy(89, 30); printf("▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄\n");
	gotoxy(89, 31); printf("█                    █\n");
	gotoxy(89, 32); printf("█                    █\n");
	gotoxy(89, 33); printf("█                    █\n");
	gotoxy(89, 34); printf("█                    █\n");
	gotoxy(89, 35); printf("█                    █\n");
	gotoxy(89, 36); printf("█                    █\n");
	gotoxy(89, 37); printf("█                    █\n");
	gotoxy(89, 38); printf("█                    █\n");
	gotoxy(89, 39); printf("█                    █\n");
	gotoxy(89, 40); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀\n");
}
void drawsans1()
{
	gotoxy(81, 0); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
	gotoxy(81, 1); printf("        ▄▄▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄▄        \n");
	gotoxy(81, 2); printf("      ▄▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄      \n");
	gotoxy(81, 3); printf("     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓     \n");
	gotoxy(81, 4); printf("    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓    \n");
	gotoxy(81, 5); printf("    ▓▓▓▓▓▓▓▓▓▓▓▀▓▓▓▓▓▓▓▀▓▓▓▓▓▓▓▓▓▓▓    \n");
	gotoxy(81, 6); printf("    ▓▓▄▓▓▓▓▓▓▓▀▄▓▓▓▓▓▓▓▄▀▓▓▓▓▓▓▓▄▓▓    \n");
	gotoxy(81, 7); printf("     ▓▓▄▄      ▓▓▓▓  ▓▓▓      ▄▄▓▓     \n");
	gotoxy(81, 8); printf("      ▓▓▓▓▓▓▀▄▓▓▓▓    ▓▓▓▄▀▀▓▓▓▓▓      \n");
	gotoxy(81, 9); printf("     ▓▓▓▓▓ ▀▓▓▓▓▓▓▄▄▄▄▓▓▓▓▓▓▓ ▀▓▓▓     \n");
	gotoxy(81, 10); printf("     ▓▓▓▄▄ ▓ ▄▀▀▀▀▀▀▀▀▀▀▀▀▀▄  ▄▄▓▓     \n");
	gotoxy(81, 11); printf("      ▓▓▓▓▓▄ ▀▓▓ ▓▓ ▓▓▓ ▓▓ ▀▄▓▓▓▓  ▄▄▄▄\n");
	gotoxy(81, 12); printf("▄▓▓▓▄  ▀▓▓▓▓▓▓▄▄ ▀▀ ▀▄▄ ▀▄▄▓▓▓▓▀ ▄▓▓▓▀▀\n");
	gotoxy(81, 13); printf("▀▀▀▓▓▓▄▄  ▀▄▄▄▓▓▓▓▓▓▓▓▓▓▓▓▀▀▀   ▓▓▓▀  ▀\n");
	gotoxy(81, 14); printf("      ▀▀▓▓▄     ▄     ▄      ▄▄▓▓▀     \n");
	gotoxy(81, 15); printf("          ▀▓▓▄▄ ▀▓▓▓▓▓▀  ▄▄▄▓▀▀▀   ▄▀ \n");
	gotoxy(81, 16); printf("           ▓  ▓▀  ▀▀▀▀ ▀▓▀▓        ▓  \n");
	gotoxy(81, 17); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
	gotoxy(81, 18); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
	gotoxy(81, 19); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
}  //눈감은 버전
void drawsans2()
{	
	textcolor(RED1, 0);
	gotoxy(0, 0); printf(" ██  ██    ██  ██    ██  ██    ██  ██    ██  ██");
	gotoxy(0, 1); printf("████████  ████████  ████████  ████████  ████████");
	gotoxy(0, 2); printf(" ██████    ██████    ██████    ██████    ██████");
	gotoxy(0, 3); printf("  ████      ████      ████      ████      ████");
	gotoxy(0, 4); printf("   ██        ██        ██        ██        ██");
	textcolor(WHITE, 0);

	gotoxy(81, 0); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");	
	gotoxy(81, 1); printf("        ▄▄▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄▄        \n");
	gotoxy(81, 2); printf("      ▄▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄      \n");	
	gotoxy(81, 3); printf("     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓     \n");	
	gotoxy(81, 4); printf("    ▓▓▓▓▓▀▀▀▀▀▀▀▓▓▓▓▓▓▓▓▀▀▀▀▀▀▓▓▓▓▓    \n");	
	gotoxy(81, 5); printf("    ▓▓▀          ▓▓▓▓▓▓  ▄▓▓▓▄  ▀▓▓    \n");	
	gotoxy(81, 6); printf("    ▓▓           ▓▓▓▓▓▓ ▀▓▓▄▓▓▀  ▓▓    \n");	
	gotoxy(81, 7); printf("     ▓▓▄▄       ▓▓▓  ▓▓▓  ▀▀▀ ▄▄▓▓     \n");	
	gotoxy(81, 8); printf("      ▓▓▓▓▓▓▀▄▓▓▓▓    ▓▓▓▄▀▀▓▓▓▓▓      \n");	
	gotoxy(81, 9); printf("     ▓▓▓▓▓ ▀▓▓▓▓▓▓▄▄▄▄▓▓▓▓▓▓▓ ▀▓▓▓     \n");	
	gotoxy(81, 10); printf("     ▓▓▓▄▄ ▓ ▄▀▀▀▀▀▀▀▀▀▀▀▀▀▄  ▄▄▓▓     \n");	
	gotoxy(81, 11); printf("      ▓▓▓▓▓▄ ▀▓▓ ▓▓ ▓▓▓ ▓▓ ▀▄▓▓▓▓  ▄▄▄▄\n");	
	gotoxy(81, 12); printf("▄▓▓▓▄  ▀▓▓▓▓▓▓▄▄ ▀▀ ▀▄▄ ▀▄▄▓▓▓▓▀ ▄▓▓▓▀▀\n");	
	gotoxy(81, 13); printf("▀▀▀▓▓▓▄▄  ▀▄▄▄▓▓▓▓▓▓▓▓▓▓▓▓▀▀▀   ▓▓▓▀  ▀\n");	
	gotoxy(81, 14); printf("      ▀▀▓▓▄     ▄     ▄      ▄▄▓▓▀     \n");	
	gotoxy(81, 15); printf("          ▀▓▓▄▄ ▀▓▓▓▓▓▀  ▄▄▄▓▀▀▀   ▄▀ \n");	
	gotoxy(81, 16); printf("           ▓  ▓▀  ▀▀▀▀ ▀▓▀▓        ▓  \n");	
	gotoxy(81, 17); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");	
	gotoxy(81, 18); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");	
	gotoxy(81, 19); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
		
	drawbox();
	textcolor(RED1, 0);
	gotoxy(100, 35); printf("♥");
	textcolor(WHITE, 0);
	
}  //눈 뜬 버전

void clear(int x2, int y2)//전투화면 초기화
{
	for (int i = 23; i <= 45; i++)
	{
		gotoxy(20, i); printf("                                                                                                                                                     ");
	}
	drawbox();
	if (pattern == 1 || pattern==3)
	{
		movetype = 2;		
		x = 100; y = 39;
		gotoxy(100, 39);
	}
	else
	{
		gotoxy(x2, y2);
	}
	if (!canHit)
	{
		textcolor(WHITE, 0);
	}
	else
	{
		if (movetype == 1) textcolor(RED1, 0);
		else if (movetype == 2) textcolor(BLUE1, 0);
	}		
	if (gravity == 'y')
	{
		printf("♥");		
	}
	else
	{
		printf("❥");
	}
	
	textcolor(WHITE, 0);
}
void cleardialogue()
{
	gotoxy(86, 22); printf("                                      ");
}
void dialogue(int x,int y,char arr[100],int delay)
{	
	cleardialogue();
	gotoxy(x, y);
	for (int i = 0;i < strlen(arr);i++)
	{
		printf("%c", arr[i]);
		Sleep(delay);
	}
}
void getDamage()//피해를 입을 때마다 실행하는 함수
{
	playSFX("damaged.wav");
	if (canHit)
	{
		abtime = clock();//피격된 시간을 abtime에 저장.
		life -= 1;
		canHit = 0;		
		gotoxy(life  * 10, 0); printf("        ");//하트 없애는 작업*5
		gotoxy(life  * 10, 1); printf("        ");
		gotoxy(life  * 10, 2); printf("        ");
		gotoxy(life  * 10, 3); printf("        ");
		gotoxy(life  * 10, 4); printf("        ");
	}	
}
//1은 아래, 2는 오른쪽, 3은 왼쪽 봄
void drawGaster1(int x, int y)
{
	textcolor(WHITE, 0);
	gotoxy(x + -1, y + 0); printf("◣▄▲▄◢");
	gotoxy(x + -1, y + 1); printf("█○█○█");
	gotoxy(x - 1, y + 2);  printf("◥▀▼▀◤");
	gotoxy(x + 0, y + 3);   printf("◥▄◤");
}
void drawGaster2(int x, int y)
{
	textcolor(WHITE, 0);
	gotoxy(x , y + 0); printf("◥▄◣");
	gotoxy(x , y + 1); printf("▄○▄  ◣");
	gotoxy(x , y + 2); printf("◀█▶  █");
	gotoxy(x , y + 3); printf("▀○▀  ◤");
	gotoxy(x , y + 4); printf("◢▀◤");
}
void drawGaster3(int x, int y)
{
	textcolor(WHITE, 0);
	gotoxy(x, y + 0); printf("   ◢▄◤");
	gotoxy(x, y + 1); printf("◢  ▄○▄");
	gotoxy(x, y + 2); printf("█  ◀█▶");
	gotoxy(x, y + 3); printf("◥  ▀○▀");
	gotoxy(x, y + 4); printf("   ◥▀◣");
}
void drawlazer1(int x)
{
	playSFX("shot.wav");
	for (int i = 30;i <= 45;i++)
	{
		gotoxy(x-1, i); printf("█████");//매개변수로 가스터와 같은 좌표로 설정해도 제대로 나오도록 -1
	}
}
void drawlazer2(int y)
{
	playSFX("shot.wav");
	for (int i = 0;i <= 60;i++)
	{
		gotoxy(87 + i, y+1); printf("█");
		gotoxy(87 + i, y+2); printf("█");
		gotoxy(87 + i, y+3); printf("█");
	}
}
void drawlazer3(int y)
{
	playSFX("shot.wav");
	for (int i = 60;i >= 0;i--)
	{
		gotoxy(112 - i, y + 1); printf("█");
		gotoxy(112 - i, y + 2); printf("█");
		gotoxy(112 - i, y + 3); printf("█");
	}
}
void drawheart(int x2, int y2)
{
	textcolor(RED1, 0);
	gotoxy(x2, y2+0); printf("         █████████     █████████         \n");
	gotoxy(x2, y2+1); printf("      ██████████████ ██████████████      \n");
	gotoxy(x2, y2+2); printf("     ███████████████████████████████     \n");
	gotoxy(x2, y2+3); printf("    █████████████████████████████████    \n");
	gotoxy(x2, y2+4); printf("    █████████████████████████████████    \n");
	gotoxy(x2, y2+5); printf("    █████████████████████████████████    \n");
	gotoxy(x2, y2+6); printf("     ███████████████████████████████     \n");
	gotoxy(x2, y2+7); printf("     ███████████████████████████████     \n");
	gotoxy(x2, y2+8); printf("      █████████████████████████████      \n");
	gotoxy(x2, y2+9); printf("       ███████████████████████████       \n");
	gotoxy(x2, y2+10);printf("        █████████████████████████        \n");
	gotoxy(x2, y2+11);printf("         ███████████████████████         \n");
	gotoxy(x2, y2+12);printf("           ███████████████████           \n");
	gotoxy(x2, y2+13);printf("             ███████████████             \n");
	gotoxy(x2, y2+14);printf("               ███████████               \n");
	gotoxy(x2, y2+15);printf("                 ███████                 \n");
	gotoxy(x2, y2+16);printf("                   ███                   \n");
	gotoxy(x2, y2+17);printf("                    █                    \n");
}
void drawheart2(int x2, int y2)
{
	textcolor(RED1, 0);
	x2 += 3;
	gotoxy(x2, y2 + 0); printf("      █████████     █████████      \n");
	gotoxy(x2, y2 + 1); printf("   ██████████████  █████████████   \n");
	gotoxy(x2, y2 + 2); printf("  ██████████████  ███████████████  \n");
	gotoxy(x2, y2 + 3); printf(" ██████████████  █████████████████ \n");
	gotoxy(x2, y2 + 4); printf(" ███████████████  ████████████████ \n");
	gotoxy(x2, y2 + 5); printf(" ████████████████  ███████████████ \n");
	gotoxy(x2, y2 + 6); printf("  ████████████████  █████████████  \n");
	gotoxy(x2, y2 + 7); printf("  ███████████████  ██████████████  \n");
	gotoxy(x2, y2 + 8); printf("   █████████████  ██████████████   \n");
	gotoxy(x2, y2 + 9); printf("    ███████████  ██████████████    \n");
	gotoxy(x2, y2 + 10);printf("     ███████████  ████████████     \n");
	gotoxy(x2, y2 + 11);printf("      ███████████  ██████████      \n");
	gotoxy(x2, y2 + 12);printf("        ██████████  ███████        \n");
	gotoxy(x2, y2 + 13);printf("          ███████  ██████          \n");
	gotoxy(x2, y2 + 14);printf("            ████  █████            \n");
	gotoxy(x2, y2 + 15);printf("              ███  ██              \n");
	gotoxy(x2, y2 + 16);printf("                ██                 \n");
	gotoxy(x2, y2 + 17);printf("                 █                 \n");
}
void drawheart3(int x2, int y2)
{
	textcolor(RED1, 0);
	x2 += 3;
	gotoxy(x2, y2 + 0); printf("      █████████     █████████      \n");
	gotoxy(x2, y2 + 1); printf("   ██████████████  █████████████   \n");
	gotoxy(x2, y2 + 2); printf("  ██  ██████████  ███████████████  \n");
	gotoxy(x2, y2 + 3); printf(" ██  █  ███████  █████████████████ \n");
	gotoxy(x2, y2 + 4); printf(" █  ███  ████     ████████████████ \n");
	gotoxy(x2, y2 + 5); printf("   █████  ██  ███  ████████  █████ \n");
	gotoxy(x2, y2 + 6); printf("  ███████    █████  ██████    ███  \n");
	gotoxy(x2, y2 + 7); printf("  ████████  █████  ██████  ██  ██  \n");
	gotoxy(x2, y2 + 8); printf("   █████████████    ████  ████     \n");
	gotoxy(x2, y2 + 9); printf("    ███  ██████  ██  ██  ██████    \n");
	gotoxy(x2, y2 + 10);printf("       █  ████    ██    ██████     \n");
	gotoxy(x2, y2 + 11);printf("      ███  ██  ██  ██  ██████      \n");
	gotoxy(x2, y2 + 12);printf("        ██    ████  ███████        \n");
	gotoxy(x2, y2 + 13);printf("          █  ████  ██████          \n");
	gotoxy(x2, y2 + 14);printf("            ████  █████            \n");
	gotoxy(x2, y2 + 15);printf("              ███  ██              \n");
	gotoxy(x2, y2 + 16);printf("                ██                 \n");
	gotoxy(x2, y2 + 17);printf("                 █                 \n");
}
void drawgameover(int x2, int y2)
{
	textcolor(WHITE, 0);
	x2 += 3;
	gotoxy(x2, y2 + 0); printf("   █████████    █████████   ██████   ██████ ██████████");
	gotoxy(x2, y2 + 1); printf("  ███▒▒▒▒▒███  ███▒▒▒▒▒███ ▒▒██████ ██████ ▒▒███▒▒▒▒▒█\n");
	gotoxy(x2, y2 + 2); printf(" ███     ▒▒▒  ▒███    ▒███  ▒███▒█████▒███  ▒███  █ ▒ ");
	gotoxy(x2, y2 + 3); printf("▒███          ▒███████████  ▒███▒▒███ ▒███  ▒██████   ");
	gotoxy(x2, y2 + 4); printf("▒███    █████ ▒███▒▒▒▒▒███  ▒███ ▒▒▒  ▒███  ▒███▒▒█   ");
	gotoxy(x2, y2 + 5); printf("▒▒███  ▒▒███  ▒███    ▒███  ▒███      ▒███  ▒███ ▒   █");
	gotoxy(x2, y2 + 6); printf(" ▒▒█████████  █████   █████ █████     █████ ██████████");
	gotoxy(x2, y2 + 7); printf("  ▒▒▒▒▒▒▒▒▒  ▒▒▒▒▒   ▒▒▒▒▒ ▒▒▒▒▒     ▒▒▒▒▒ ▒▒▒▒▒▒▒▒▒▒ ");
	gotoxy(x2, y2 + 10); printf("                                                                                               ");
	gotoxy(x2, y2 + 11);printf("    ███████    █████   █████ ██████████ ███████████   ");
	gotoxy(x2, y2 + 12);printf("  ███▒▒▒▒▒███ ▒▒███   ▒▒███ ▒▒███▒▒▒▒▒█▒▒███▒▒▒▒▒███  ");
	gotoxy(x2, y2 + 13);printf(" ███     ▒▒███ ▒███    ▒███  ▒███  █ ▒  ▒███    ▒███  ");
	gotoxy(x2, y2 + 14);printf("▒███      ▒███ ▒███    ▒███  ▒██████    ▒██████████   ");
	gotoxy(x2, y2 + 15);printf("▒███      ▒███ ▒▒███   ███   ▒███▒▒█    ▒███▒▒▒▒▒███  ");
	gotoxy(x2, y2 + 16);printf("▒▒███     ███   ▒▒▒█████▒    ▒███ ▒   █ ▒███    ▒███  ");
	gotoxy(x2, y2 + 17);printf(" ▒▒▒███████▒      ▒▒███      ██████████ █████   █████ ");
	gotoxy(x2, y2 + 18);printf("   ▒▒▒▒▒▒▒         ▒▒▒      ▒▒▒▒▒▒▒▒▒▒ ▒▒▒▒▒   ▒▒▒▒▒  ");
	gotoxy(x2, y2 + 19); printf("                                                                                               ");
	gotoxy(x2, y2 + 20); printf("                                                                                               ");
	gotoxy(x2, y2 + 21); printf("                                                                                               ");
	gotoxy(x2, y2 + 22); printf("                                                                                               ");
	gotoxy(x2, y2 + 23); printf("                                                                                               ");
	gotoxy(x2, y2 + 24); printf("                                                                                               ");
	gotoxy(x2, y2 + 25); printf("                                                                                               ");
	gotoxy(x2, y2 + 26); printf("                                                                                               ");
	gotoxy(x2, y2 + 27); printf("                                                                                               ");
}




//앞에 문자는 패턴 종류, 뒤 숫자는 나오는 패턴의 순서
void bone1(double time)//밑에서 뼈 공격
{		
	if (time >= 2.08 && n == 4)
	{
		gotoxy(90, 37); printf("                    ");
		gotoxy(90, 38); printf("                    ");
		gotoxy(90, 39); printf("                    ");
		n++;
	}
	else if (time >= 1.06 && n == 3)
	{
		gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 38); printf("||||||||||||||||||||");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		movetype = 1;
		playSFX("ding.wav");
		n++;
	}
	else if (time >= 1.04 && n == 2)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (time >= 1.02 && n == 1)
	{
		gotoxy(90, 37); printf("                    ");
		gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		n++;
	}
	else if (time >= 0.02&&n==0)
	{
		playSFX("warning.wav");
		textcolor(RED1, 0);
		gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
		textcolor(WHITE, 0);
		n++;
	}
}    
void bone2(double time)//오프닝 뼈 지그재그
{
	if (time >= 6.0 && n == 45)
	{
		gotoxy(90, 31); printf("                    ");
		gotoxy(90, 32); printf("                    ");
		gotoxy(90, 33); printf("                    ");
		gotoxy(90, 34); printf("                    ");
		gotoxy(90, 35); printf("                    ");
		gotoxy(90, 36); printf("                    ");
		gotoxy(90, 37); printf("                    ");
		gotoxy(90, 38); printf("                    ");
		gotoxy(90, 39); printf("                    ");
		n++;
	}
	else if (time >= 5.9 && n == 44)
	{
		gotoxy(90, 31); printf("                   |");
		gotoxy(90, 32); printf("                   |");
		gotoxy(90, 33); printf("                   |");
		gotoxy(90, 34); printf("                   |");
		gotoxy(90, 35); printf("                   Ʌ");
		gotoxy(90, 36); printf("                    ");
		gotoxy(90, 37); printf("                    ");
		gotoxy(90, 38); printf("                    ");
		gotoxy(90, 39); printf("                   V");
		n++;
	}
	else if (time >= 5.8 && n == 43)
	{
		gotoxy(90, 31); printf("                  ||");
		gotoxy(90, 32); printf("                  ||");
		gotoxy(90, 33); printf("                  ||");
		gotoxy(90, 34); printf("                  |Ʌ");
		gotoxy(90, 35); printf("                  Ʌ ");
		gotoxy(90, 36); printf("                    ");
		gotoxy(90, 37); printf("                    ");
		gotoxy(90, 38); printf("                   V");
		gotoxy(90, 39); printf("                  V|");
		n++;
	}
	else if (time >= 5.7 && n == 42)
	{
		gotoxy(90, 31); printf("                 |||");
		gotoxy(90, 32); printf("                 |||");
		gotoxy(90, 33); printf("                 ||Ʌ");
		gotoxy(90, 34); printf("                 |Ʌ ");
		gotoxy(90, 35); printf("                 Ʌ  ");
		gotoxy(90, 36); printf("                    ");
		gotoxy(90, 37); printf("                   V");
		gotoxy(90, 38); printf("                  V|");
		gotoxy(90, 39); printf("                 V||");
		n++;
	}
	else if (time >= 5.6 && n == 41)
	{
		gotoxy(90, 31); printf("                ||||");
		gotoxy(90, 32); printf("                |||Ʌ");
		gotoxy(90, 33); printf("                ||Ʌ ");
		gotoxy(90, 34); printf("                |Ʌ  ");
		gotoxy(90, 35); printf("                Ʌ   ");
		gotoxy(90, 36); printf("                   V");
		gotoxy(90, 37); printf("                  V|");
		gotoxy(90, 38); printf("                 V||");
		gotoxy(90, 39); printf("                V|||");
		n++;
	}
	else if (time >= 5.5 && n == 40)
	{
		gotoxy(90, 31); printf("               ||||Ʌ");
		gotoxy(90, 32); printf("               |||Ʌ ");
		gotoxy(90, 33); printf("               ||Ʌ  ");
		gotoxy(90, 34); printf("               |Ʌ   ");
		gotoxy(90, 35); printf("               Ʌ   V");
		gotoxy(90, 36); printf("                  V|");
		gotoxy(90, 37); printf("                 V||");
		gotoxy(90, 38); printf("                V|||");
		gotoxy(90, 39); printf("               V||||");
		n++;
	}
	else if (time >= 5.4 && n == 39)
	{
		gotoxy(90, 31); printf("              ||||Ʌ|");
		gotoxy(90, 32); printf("              |||Ʌ Ʌ");
		gotoxy(90, 33); printf("              ||Ʌ   ");
		gotoxy(90, 34); printf("              |Ʌ    ");
		gotoxy(90, 35); printf("              Ʌ   V ");
		gotoxy(90, 36); printf("                 V|V");
		gotoxy(90, 37); printf("                V|||");
		gotoxy(90, 38); printf("               V||||");
		gotoxy(90, 39); printf("              V|||||");
		n++;
	}
	else if (time >= 5.3 && n == 38)
	{
		gotoxy(90, 31); printf("             ||||Ʌ||");
		gotoxy(90, 32); printf("             |||Ʌ Ʌ|");
		gotoxy(90, 33); printf("             ||Ʌ   Ʌ");
		gotoxy(90, 34); printf("             |Ʌ     ");
		gotoxy(90, 35); printf("             Ʌ   V  ");
		gotoxy(90, 36); printf("                V|V ");
		gotoxy(90, 37); printf("               V|||V");
		gotoxy(90, 38); printf("              V|||||");
		gotoxy(90, 39); printf("             V||||||");
		n++;
	}
	else if (time >= 5.2 && n == 37)
	{
		gotoxy(90, 31); printf("            ||||Ʌ|||");
		gotoxy(90, 32); printf("            |||Ʌ Ʌ||");
		gotoxy(90, 33); printf("            ||Ʌ   Ʌ|");
		gotoxy(90, 34); printf("            |Ʌ     Ʌ");
		gotoxy(90, 35); printf("            Ʌ   V   ");
		gotoxy(90, 36); printf("               V|V  ");
		gotoxy(90, 37); printf("              V|||V ");
		gotoxy(90, 38); printf("             V|||||V");
		gotoxy(90, 39); printf("            V|||||||");
		n++;
	}
	else if (time >= 5.1 && n == 36)
	{
		gotoxy(90, 31); printf("           ||||Ʌ||||");
		gotoxy(90, 32); printf("           |||Ʌ Ʌ|||");
		gotoxy(90, 33); printf("           ||Ʌ   Ʌ||");
		gotoxy(90, 34); printf("           |Ʌ     Ʌ|");
		gotoxy(90, 35); printf("           Ʌ   V   Ʌ");
		gotoxy(90, 36); printf("              V|V   ");
		gotoxy(90, 37); printf("             V|||V  ");
		gotoxy(90, 38); printf("            V|||||V ");
		gotoxy(90, 39); printf("           V|||||||V");
		n++;
	}
	else if (time >= 5.0 && n == 35)
	{
		gotoxy(90, 31); printf("          ||||Ʌ|||||");
		gotoxy(90, 32); printf("          |||Ʌ Ʌ||||");
		gotoxy(90, 33); printf("          ||Ʌ   Ʌ|||");
		gotoxy(90, 34); printf("          |Ʌ     Ʌ|Ʌ");
		gotoxy(90, 35); printf("          Ʌ   V   Ʌ ");
		gotoxy(90, 36); printf("             V|V    ");
		gotoxy(90, 37); printf("            V|||V   ");
		gotoxy(90, 38); printf("           V|||||V V");
		gotoxy(90, 39); printf("          V|||||||V|");
		n++;
	}
	else if (time >= 4.9 && n == 34)
	{
		gotoxy(90, 31); printf("         ||||Ʌ||||||");
		gotoxy(90, 32); printf("         |||Ʌ Ʌ|||||");
		gotoxy(90, 33); printf("         ||Ʌ   Ʌ|||Ʌ");
		gotoxy(90, 34); printf("         |Ʌ     Ʌ|Ʌ ");
		gotoxy(90, 35); printf("         Ʌ   V   Ʌ  ");
		gotoxy(90, 36); printf("            V|V     ");
		gotoxy(90, 37); printf("           V|||V   V");
		gotoxy(90, 38); printf("          V|||||V V|");
		gotoxy(90, 39); printf("         V|||||||V||");
		n++;
	}
	else if (time >= 4.8 && n == 33)
	{
		gotoxy(90, 31); printf("        ||||Ʌ|||||||");
		gotoxy(90, 32); printf("        |||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("        ||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("        |Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("        Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("           V|V     V");
		gotoxy(90, 37); printf("          V|||V   V|");
		gotoxy(90, 38); printf("         V|||||V V||");
		gotoxy(90, 39); printf("        V|||||||V|||");
		n++;
	}
	else if (time >= 4.7 && n == 32)
	{
		gotoxy(90, 31); printf("       ||||Ʌ|||||||Ʌ");
		gotoxy(90, 32); printf("       |||Ʌ Ʌ|||||Ʌ ");
		gotoxy(90, 33); printf("       ||Ʌ   Ʌ|||Ʌ  ");
		gotoxy(90, 34); printf("       |Ʌ     Ʌ|Ʌ   ");
		gotoxy(90, 35); printf("       Ʌ   V   Ʌ   V");
		gotoxy(90, 36); printf("          V|V     V|");
		gotoxy(90, 37); printf("         V|||V   V||");
		gotoxy(90, 38); printf("        V|||||V V|||");
		gotoxy(90, 39); printf("       V|||||||V||||");
		n++;
	}
	else if (time >= 4.6 && n == 31)
	{
		gotoxy(90, 31); printf("      ||||Ʌ|||||||Ʌ|");
		gotoxy(90, 32); printf("      |||Ʌ Ʌ|||||Ʌ Ʌ");
		gotoxy(90, 33); printf("      ||Ʌ   Ʌ|||Ʌ   ");
		gotoxy(90, 34); printf("      |Ʌ     Ʌ|Ʌ    ");
		gotoxy(90, 35); printf("      Ʌ   V   Ʌ   V ");
		gotoxy(90, 36); printf("         V|V     V|V");
		gotoxy(90, 37); printf("        V|||V   V|||");
		gotoxy(90, 38); printf("       V|||||V V||||");
		gotoxy(90, 39); printf("      V|||||||V|||||");
		n++;
	}
	else if (time >= 4.5 && n == 30)
	{
		gotoxy(90, 31); printf("     ||||Ʌ|||||||Ʌ||");
		gotoxy(90, 32); printf("     |||Ʌ Ʌ|||||Ʌ Ʌ|");
		gotoxy(90, 33); printf("     ||Ʌ   Ʌ|||Ʌ   Ʌ");
		gotoxy(90, 34); printf("     |Ʌ     Ʌ|Ʌ     ");
		gotoxy(90, 35); printf("     Ʌ   V   Ʌ   V  ");
		gotoxy(90, 36); printf("        V|V     V|V");
		gotoxy(90, 37); printf("       V|||V   V|||V");
		gotoxy(90, 38); printf("      V|||||V V|||||");
		gotoxy(90, 39); printf("     V|||||||V||||||");
		n++;
	}
	else if (time >= 4.4 && n == 29)
	{
		gotoxy(90, 31); printf("    ||||Ʌ|||||||Ʌ|||"); 
		gotoxy(90, 32); printf("    |||Ʌ Ʌ|||||Ʌ Ʌ||"); 
		gotoxy(90, 33); printf("    ||Ʌ   Ʌ|||Ʌ   Ʌ|"); 
		gotoxy(90, 34); printf("    |Ʌ     Ʌ|Ʌ     Ʌ"); 
		gotoxy(90, 35); printf("    Ʌ   V   Ʌ   V   "); 
		gotoxy(90, 36); printf("       V|V     V|V "); 
		gotoxy(90, 37); printf("      V|||V   V|||V");
		gotoxy(90, 38); printf("     V|||||V V|||||V"); 
		gotoxy(90, 39); printf("    V|||||||V|||||||");
		n++;
	}
	else if (time >= 4.3 && n == 27)
	{
		gotoxy(90, 31); printf("   ||||Ʌ|||||||Ʌ||||"); 
		gotoxy(90, 32); printf("   |||Ʌ Ʌ|||||Ʌ Ʌ|||");
		gotoxy(90, 33); printf("   ||Ʌ   Ʌ|||Ʌ   Ʌ||"); 
		gotoxy(90, 34); printf("   |Ʌ     Ʌ|Ʌ     Ʌ|"); 
		gotoxy(90, 35); printf("   Ʌ   V   Ʌ   V   Ʌ"); 
		gotoxy(90, 36); printf("      V|V     V|V   "); 
		gotoxy(90, 37); printf("     V|||V   V|||V "); 
		gotoxy(90, 38); printf("    V|||||V V|||||V "); 
		gotoxy(90, 39); printf("   V|||||||V|||||||V"); 
		n+=2;
	}
	else if (time >= 4.2 && n == 26)
	{
		gotoxy(90, 31); printf("  ||||Ʌ|||||||Ʌ|||||"); 
		gotoxy(90, 32); printf("  |||Ʌ Ʌ|||||Ʌ Ʌ||||"); 
		gotoxy(90, 33); printf("  ||Ʌ   Ʌ|||Ʌ   Ʌ|||"); 
		gotoxy(90, 34); printf("  |Ʌ     Ʌ|Ʌ     Ʌ|Ʌ"); 
		gotoxy(90, 35); printf("  Ʌ   V   Ʌ   V   Ʌ "); 
		gotoxy(90, 36); printf("     V|V     V|V    "); 
		gotoxy(90, 37); printf("    V|||V   V|||V   ");
		gotoxy(90, 38); printf("   V|||||V V|||||V V"); 
		gotoxy(90, 39); printf("  V|||||||V|||||||V|"); 
		n++;
	}
	else if (time >= 4.1 && n == 25)
	{
		gotoxy(90, 31); printf(" ||||Ʌ|||||||Ʌ||||||"); 
		gotoxy(90, 32); printf(" |||Ʌ Ʌ|||||Ʌ Ʌ|||||");
		gotoxy(90, 33); printf(" ||Ʌ   Ʌ|||Ʌ   Ʌ|||Ʌ"); 
		gotoxy(90, 34); printf(" |Ʌ     Ʌ|Ʌ     Ʌ|Ʌ ");
		gotoxy(90, 35); printf(" Ʌ   V   Ʌ   V   Ʌ  "); 
		gotoxy(90, 36); printf("    V|V     V|V     ");
		gotoxy(90, 37); printf("   V|||V   V|||V   V"); 
		gotoxy(90, 38); printf("  V|||||V V|||||V V|");
		gotoxy(90, 39); printf(" V|||||||V|||||||V||"); 
		n++;
	}
	else if (time >= 4.0 && n == 24)
	{
		gotoxy(90, 31); printf("||||Ʌ|||||||Ʌ|||||||");
		gotoxy(90, 32); printf("|||Ʌ Ʌ|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("||Ʌ   Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("|Ʌ     Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("Ʌ   V   Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("   V|V     V|V     V");
		gotoxy(90, 37); printf("  V|||V   V|||V   V|");
		gotoxy(90, 38); printf(" V|||||V V|||||V V||");
		gotoxy(90, 39); printf("V|||||||V|||||||V|||");
		n++;
	}
	else if (time >= 3.9 && n == 23)
	{
		gotoxy(90, 31); printf("|||Ʌ|||||||Ʌ|||||||");
		gotoxy(90, 32); printf("||Ʌ Ʌ|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("|Ʌ   Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("Ʌ     Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("   V   Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("  V|V     V|V     V");
		gotoxy(90, 37); printf(" V|||V   V|||V   V|");
		gotoxy(90, 38); printf("V|||||V V|||||V V||");
		gotoxy(90, 39); printf("|||||||V|||||||V|||");
		n++;
	}
	else if (time >= 3.8 && n == 22)
	{
		gotoxy(90, 31); printf("||Ʌ|||||||Ʌ|||||||");
		gotoxy(90, 32); printf("|Ʌ Ʌ|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("Ʌ   Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("     Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("  V   Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf(" V|V     V|V     V");
		gotoxy(90, 37); printf("V|||V   V|||V   V|");
		gotoxy(90, 38); printf("|||||V V|||||V V||");
		gotoxy(90, 39); printf("||||||V|||||||V|||");
		n++;
	}
	else if (time >= 3.7 && n == 21)
	{
		gotoxy(90, 31); printf("|Ʌ|||||||Ʌ|||||||");
		gotoxy(90, 32); printf("Ʌ Ʌ|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("   Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("    Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf(" V   Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("V|V     V|V     V");
		gotoxy(90, 37); printf("|||V   V|||V   V|");
		gotoxy(90, 38); printf("||||V V|||||V V||");
		gotoxy(90, 39); printf("|||||V|||||||V|||");
		n++;
	}
	else if (time >= 3.6 && n == 20)
	{
		gotoxy(90, 31); printf("Ʌ|||||||Ʌ|||||||");
		gotoxy(90, 32); printf(" Ʌ|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("  Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("   Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("V   Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("|V     V|V     V");
		gotoxy(90, 37); printf("||V   V|||V   V|");
		gotoxy(90, 38); printf("|||V V|||||V V||");
		gotoxy(90, 39); printf("||||V|||||||V|||");
		n++;
	}
	else if (time >= 3.5 && n == 19)
	{
		gotoxy(90, 31); printf("|||||||Ʌ|||||||");
		gotoxy(90, 32); printf("Ʌ|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf(" Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("  Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("   Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("V     V|V     V");
		gotoxy(90, 37); printf("|V   V|||V   V|");
		gotoxy(90, 38); printf("||V V|||||V V||");
		gotoxy(90, 39); printf("|||V|||||||V|||");
		n++;
	}
	else if (time >= 3.4 && n == 18)
	{
		gotoxy(90, 31); printf("||||||Ʌ|||||||");
		gotoxy(90, 32); printf("|||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("Ʌ|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf(" Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("  Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("     V|V     V");
		gotoxy(90, 37); printf("V   V|||V   V|");
		gotoxy(90, 38); printf("|V V|||||V V||");
		gotoxy(90, 39); printf("||V|||||||V|||");
		n++;
	}
	else if (time >= 3.3 && n == 17)
	{
		gotoxy(90, 31); printf("|||||Ʌ|||||||");
		gotoxy(90, 32); printf("||||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("|||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("Ʌ|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf(" Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("    V|V     V");
		gotoxy(90, 37); printf("   V|||V   V|");
		gotoxy(90, 38); printf("V V|||||V V||");
		gotoxy(90, 39); printf("|V|||||||V|||");
		n++;
	}
	else if (time >= 3.2 && n == 16)
	{
		gotoxy(90, 31); printf("||||Ʌ|||||||");
		gotoxy(90, 32); printf("|||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("||Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("|Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("Ʌ   V   Ʌ   ");
		gotoxy(90, 36); printf("   V|V     V");
		gotoxy(90, 37); printf("  V|||V   V|");
		gotoxy(90, 38); printf(" V|||||V V||");
		gotoxy(90, 39); printf("V|||||||V|||");
		n++;
	}
	else if (time >= 3.1 && n == 15)
	{
		gotoxy(90, 31); printf("|||Ʌ|||||||");
		gotoxy(90, 32); printf("||Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("|Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("Ʌ     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("   V   Ʌ   ");
		gotoxy(90, 36); printf("  V|V     V");
		gotoxy(90, 37); printf(" V|||V   V|");
		gotoxy(90, 38); printf("V|||||V V||");
		gotoxy(90, 39); printf("|||||||V|||");
		n++;
	}
	else if (time >= 3.0 && n == 14)
	{
		gotoxy(90, 31); printf("||Ʌ|||||||");
		gotoxy(90, 32); printf("|Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("Ʌ   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("     Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("  V   Ʌ   ");
		gotoxy(90, 36); printf(" V|V     V");
		gotoxy(90, 37); printf("V|||V   V|");
		gotoxy(90, 38); printf("|||||V V||");
		gotoxy(90, 39); printf("||||||V|||");
		n++;
	}
	else if (time >= 2.9 && n == 13)
	{
		gotoxy(90, 31); printf("|Ʌ|||||||");
		gotoxy(90, 32); printf("Ʌ Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("   Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("    Ʌ|Ʌ  ");
		gotoxy(90, 35); printf(" V   Ʌ   ");
		gotoxy(90, 36); printf("V|V     V");
		gotoxy(90, 37); printf("|||V   V|");
		gotoxy(90, 38); printf("||||V V||");
		gotoxy(90, 39); printf("|||||V|||");
		n++;
	}
	else if (time >= 2.8 && n == 12)
	{
		gotoxy(90, 31); printf("Ʌ|||||||");
		gotoxy(90, 32); printf(" Ʌ|||||Ʌ");
		gotoxy(90, 33); printf("  Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("   Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("V   Ʌ   ");
		gotoxy(90, 36); printf("|V     V");
		gotoxy(90, 37); printf("||V   V|");
		gotoxy(90, 38); printf("|||V V||");
		gotoxy(90, 39); printf("||||V|||");
		n++;
	}
	else if (time >= 2.7 && n == 11)
	{
		gotoxy(90, 31); printf("|||||||");
		gotoxy(90, 32); printf("Ʌ|||||Ʌ");
		gotoxy(90, 33); printf(" Ʌ|||Ʌ ");
		gotoxy(90, 34); printf("  Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("   Ʌ   ");
		gotoxy(90, 36); printf("V     V");
		gotoxy(90, 37); printf("|V   V|");
		gotoxy(90, 38); printf("||V V||");
		gotoxy(90, 39); printf("|||V|||");
		n++;
	}
	else if (time >= 2.6 && n == 10)
	{
		gotoxy(90, 31); printf("||||||");
		gotoxy(90, 32); printf("|||||Ʌ");
		gotoxy(90, 33); printf("Ʌ|||Ʌ ");
		gotoxy(90, 34); printf(" Ʌ|Ʌ  ");
		gotoxy(90, 35); printf("  Ʌ   ");
		gotoxy(90, 36); printf("     V");
		gotoxy(90, 37); printf("V   V|");
		gotoxy(90, 38); printf("|V V||");
		gotoxy(90, 39); printf("||V|||");
		n++;
	}
	else if (time >= 2.5 && n == 9)
	{
		gotoxy(90, 31); printf("|||||");
		gotoxy(90, 32); printf("||||Ʌ");
		gotoxy(90, 33); printf("|||Ʌ ");
		gotoxy(90, 34); printf("Ʌ|Ʌ  ");
		gotoxy(90, 35); printf(" Ʌ   ");
		gotoxy(90, 36); printf("    V");
		gotoxy(90, 37); printf("   V|");
		gotoxy(90, 38); printf("V V||");
		gotoxy(90, 39); printf("|V|||");
		n++;
	}
	else if (time >= 2.4 && n == 8)
	{
		gotoxy(90, 31); printf("||||");
		gotoxy(90, 32); printf("|||Ʌ");
		gotoxy(90, 33); printf("||Ʌ ");
		gotoxy(90, 34); printf("|Ʌ  ");
		gotoxy(90, 35); printf("Ʌ   ");
		gotoxy(90, 36); printf("   V");
		gotoxy(90, 37); printf("  V|");
		gotoxy(90, 38); printf(" V||");
		gotoxy(90, 39); printf("V|||");
		n++;
	}
	else if (time >= 2.3 && n == 7)
	{
		gotoxy(90, 31); printf("|||");
		gotoxy(90, 32); printf("||Ʌ");
		gotoxy(90, 33); printf("|Ʌ ");
		gotoxy(90, 34); printf("Ʌ  ");
		gotoxy(90, 35); printf("   ");
		gotoxy(90, 36); printf("  V");
		gotoxy(90, 37); printf(" V|");
		gotoxy(90, 38); printf("V||");
		gotoxy(90, 39); printf("|||");
		n++;
	}
	else if (time >= 2.2 && n == 6)
	{
		gotoxy(90, 31); printf("||");
		gotoxy(90, 32); printf("|Ʌ");
		gotoxy(90, 33); printf("Ʌ ");
		gotoxy(90, 34); printf("  ");
		gotoxy(90, 35); printf("  ");
		gotoxy(90, 36); printf(" V");
		gotoxy(90, 37); printf("V|");
		gotoxy(90, 38); printf("||");
		gotoxy(90, 39); printf("||");
		n++;
	}
	else if (time >= 2.1 && n == 5)
	{
		playSFX("charge.wav");
		gotoxy(90, 31); printf("|");
		gotoxy(90, 32); printf("Ʌ");
		gotoxy(90, 33); printf(" ");
		gotoxy(90, 34); printf(" ");
		gotoxy(90, 35); printf(" ");
		gotoxy(90, 36); printf("V");
		gotoxy(90, 37); printf("|");
		gotoxy(90, 38); printf("|");
		gotoxy(90, 39); printf("|");
		n++;
	}
}
void gaster3(double time,int x,int y)
{
	if (n <= 47)
	{
		if (time >= 7.0 && n == 47)
		{
			clear(x, y);
			n++;
		}
		else if (time >= 6.2 && n == 46)
		{
			drawlazer1(91);
			drawlazer1(106);
			drawlazer2(30);
			drawlazer3(36);
			n++;
		}
		else if (time >= 5.25)
		{
			gotoxy(90, 25); printf("     ");
			gotoxy(105, 25); printf("     ");
			for (int i = 30; i <= 35; i++)
			{
				gotoxy(80, i); printf(" ");
				gotoxy(119, i + 6); printf(" ");
			}
			drawGaster1(91, 26);
			drawGaster1(106, 26);
			drawGaster2(81, 30);
			drawGaster3(113, 36);
		}
		else if (time >= 5.2)
		{
			gotoxy(90, 24); printf("     ");
			gotoxy(105, 24); printf("     ");
			for (int i = 30; i <= 35; i++)
			{
				gotoxy(79, i); printf(" ");
				gotoxy(120, i + 6); printf(" ");
			}
			drawGaster1(91, 25);
			drawGaster1(106, 25);
			drawGaster2(80, 30);
			drawGaster3(114, 36);
		}
		else if (time >= 5.15)
		{
			gotoxy(90, 23); printf("     ");
			gotoxy(105, 23); printf("     ");
			for (int i = 30; i <= 35; i++)
			{
				gotoxy(78, i); printf(" ");
				gotoxy(121, i + 6); printf(" ");
			}
			drawGaster1(91, 24);
			drawGaster1(106, 24);
			drawGaster2(79, 30);
			drawGaster3(115, 36);
		}
		else if (time >= 5.0)
		{
			playSFX("charge.wav");
			drawGaster1(91, 23);
			drawGaster1(106, 23);
			drawGaster2(78, 30);
			drawGaster3(116, 36);
		}
	}	
}
void gaster4(double time, int x, int y)
{
	if (time >= 9 && n == 53)
	{
		clear(x, y);
		n++;
	}
	else if (time >= 8.5 && n == 52)
	{
		drawlazer1(98);
		drawlazer2(33);
		n++;
	}
	else if (time >= 7.55 && n == 51)
	{
		gotoxy(97, 25); printf("     ");
		for (int i = 33; i <= 38; i++)
		{
			gotoxy(80, i); printf(" ");
		}
		drawGaster1(98, 26);
		drawGaster2(81, 33);
		n++;
	}
	else if (time >= 7.5 && n == 50)
	{
		gotoxy(97, 24); printf("     ");
		for (int i = 33; i <= 38; i++)
		{
			gotoxy(79, i); printf(" ");
		}
		drawGaster1(98, 25);
		drawGaster2(80, 33);
		n++;
	}
	else if (time >= 7.45 && n == 49)
	{
		gotoxy(97, 23); printf("     ");
		for (int i = 33; i <= 38; i++)
		{
			gotoxy(78, i); printf(" ");
		}
		drawGaster1(98, 24);
		drawGaster2(79, 33);
		n++;
	}
	else if (time >= 7.3 && n == 48)
	{
		playSFX("charge.wav");
		drawGaster1(98, 23);
		drawGaster2(78, 33);
		n++;
	}
}
void gaster5(double time, int x, int y)
{
	if (time >= 11 && n == 59)
	{
		clear(x, y);
		n++;
	}
	else if (time >= 10.3 && n == 58)
	{
		drawlazer1(91);
		drawlazer1(106);
		drawlazer2(30);
		drawlazer3(36);
		n++;
	}
	else if (time >= 9.35 && n == 57)
	{
		gotoxy(90, 25); printf("     ");
		gotoxy(105, 25); printf("     ");
		for (int i = 30; i <= 35; i++)
		{
			gotoxy(80, i); printf(" ");
			gotoxy(119, i + 6); printf(" ");
		}
		drawGaster1(91, 26);
		drawGaster1(106, 26);
		drawGaster2(81, 30);
		drawGaster3(113, 36);
		n++;
	}
	else if (time >= 9.3 && n == 56)
	{
		gotoxy(90, 24); printf("     ");
		gotoxy(105, 24); printf("     ");
		for (int i = 30; i <= 35; i++)
		{
			gotoxy(79, i); printf(" ");
			gotoxy(120, i + 6); printf(" ");
		}
		drawGaster1(91, 25);
		drawGaster1(106, 25);
		drawGaster2(80, 30);
		drawGaster3(114, 36);
		n++;
	}
	else if (time >= 9.25 && n == 55)
	{
		gotoxy(90, 23); printf("     ");
		gotoxy(105, 23); printf("     ");
		for (int i = 30; i <= 35; i++)
		{
			gotoxy(78, i); printf(" ");
			gotoxy(121, i + 6); printf(" ");
		}
		drawGaster1(91, 24);
		drawGaster1(106, 24);
		drawGaster2(79, 30);
		drawGaster3(115, 36);
		n++;
	}
	else if (time >= 9.1 && n == 54)
	{
		playSFX("charge.wav");
		drawGaster1(91, 23);
		drawGaster1(106, 23);
		drawGaster2(78, 30);
		drawGaster3(116, 36);
		n++;
	}
}
void gaster6(double time, int x, int y)
{
	if (time >= 15 && n == 65)
	{		
		n=100;//앞으로 패턴은 100부터 시작
		pattern = rand() % 3 + 1;//1~3중 랜덤 패턴 실행
		clear(x, y);
		playBGM("MEGALOVANIA.mp3");
	}
	else if (time >= 12.3 && n == 64)
	{
		drawlazer2(32);
		drawlazer3(34);
		n++;
	}
	else if (time >= 11.35 && n == 63)
	{
		for (int i = 32; i <= 37; i++)
		{
			gotoxy(80, i); printf(" ");
			gotoxy(119, i + 2); printf(" ");
		}
		drawGaster2(81, 32);
		drawGaster3(113, 34);
		n++;
	}
	else if (time >= 11.3 && n == 62)
	{
		for (int i = 32; i <= 37; i++)
		{
			gotoxy(79, i); printf(" ");
			gotoxy(120, i + 2); printf(" ");
		}
		drawGaster2(80, 32);
		drawGaster3(114, 34);
		n++;
	}
	else if (time >= 11.25 && n == 61)
	{
		for (int i = 32; i <= 37; i++)
		{
			gotoxy(78, i); printf(" ");
			gotoxy(121, i + 2); printf(" ");
		}
		drawGaster2(79, 32);
		drawGaster3(115, 34);
		n++;
	}
	else if (time >= 11.1 && n == 60)
	{
		playSFX("charge.wav");
		drawGaster2(78, 32);
		drawGaster3(116, 34);
		n++;
	}
}
void jumpPat(double t, int x, int y)
{
	if (n == 100)
	{
		start_time = clock();
		n++;		
		movetype = 2;
	}

	if (t >= 17 && n == 130)
	{
		r--;
		n = 100;//앞으로 패턴은 100부터 시작
		srand(time(NULL));
		pattern = rand() % 3 + 1;//1~3중 랜덤 패턴 실행
		clear(x, y);
	}
	else if (t >= 15.82 && n == 129)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 15.62 && n == 128)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 14.62 && n == 127)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 14.42 && n == 126)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 13.42 && n == 125)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 13.22 && n == 124)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 12.22 && n == 123)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 12.02 && n == 122)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 11.02 && n == 121)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 10.82 && n == 120)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 9.82 && n == 119)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 9.62 && n == 118)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 8.62 && n == 117)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 8.42 && n == 116)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 7.42 && n == 115)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 7.22 && n == 114)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 6.22 && n == 113)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 6.02 && n == 112)
	{
		gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
		gotoxy(90, 39); printf("||||||||||||||||||||");
		n++;
	}
	else if (t >= 5.02 && n == 111)
	{
		gotoxy(90, 38); printf("V  V  V  V  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |  |  |  |");
		n++;
	}
	else if (t >= 4.52 && n == 110)
	{
		gotoxy(90, 38); printf("V  V  V  V"); gotoxy(100, 38); printf("  V  V  V");
		gotoxy(90, 39); printf("|  |  |  |"); gotoxy(100, 39); printf("  |  |  |");
		n++;
	}
	else if (t >= 4.02 && n == 109)
	{
		gotoxy(90, 38); printf("  V  V  V"); gotoxy(100, 38); printf("V  V  V  ");
		gotoxy(90, 39); printf("  |  |  |"); gotoxy(100, 39); printf("|  |  |  ");
		n++;
	}
	else if (t >= 3.52 && n == 108)
	{
		gotoxy(90, 38); printf(" V  V  V"); gotoxy(101, 38); printf("V  V  V ");
		gotoxy(90, 39); printf(" |  |  |"); gotoxy(101, 39); printf("|  |  | ");
		n++;
	}
	else if (t >= 3.02 && n == 107)
	{
		gotoxy(90, 38); printf("V  V  V"); gotoxy(102, 38); printf("V  V  V");
		gotoxy(90, 39); printf("|  |  |"); gotoxy(102, 39); printf("|  |  |");
		n++;
	}
	else if (t >=2.52 && n == 106)
	{
		gotoxy(90, 38); printf("  V  V"); gotoxy(103, 38); printf("V  V  ");
		gotoxy(90, 39); printf("  |  |"); gotoxy(103, 39); printf("|  |  ");
		n++;
	}
	else if (t >= 2.02 && n == 105)
	{
		gotoxy(90, 38); printf(" V  V"); gotoxy(104, 38); printf("V  V ");
		gotoxy(90, 39); printf(" |  |"); gotoxy(104, 39); printf("|  | ");
		n++;
	}
	else if (t >= 1.52 && n == 104)
	{
		gotoxy(90, 38); printf("V  V"); gotoxy(105, 38); printf("V  V");
		gotoxy(90, 39); printf("|  |"); gotoxy(105, 39); printf("|  |");
		n++;
	}
	else if (t >= 1.02 && n == 103)
	{
		gotoxy(90, 38); printf("  V"); gotoxy(106, 38); printf("V  ");
		gotoxy(90, 39); printf("  |"); gotoxy(106, 39); printf("|  ");
		n++;
	}
	else if (t >= 0.52 && n == 102)
	{
		gotoxy(90, 38); printf(" V"); gotoxy(107, 38); printf("V ");
		gotoxy(90, 39); printf(" |"); gotoxy(107, 39); printf("| ");
		n++;
	}
	else if (t >= 0.02 && n == 101)
	{
		gotoxy(90, 38); printf("V"); gotoxy(108, 38); printf("V");
		gotoxy(90, 39); printf("|"); gotoxy(108, 39); printf("|");
		n++;
	}
}
void lazerPat(double t, int x, int y)
{
	if (n == 100)
	{
		start_time = clock();
		n++;
		movetype = 1;
	}

	if (t >= 13 && n == 138)
	{
		r--;
		n = 100;//앞으로 패턴은 100부터 시작
		srand(time(NULL));
		pattern = rand() % 3 + 1;//1~3중 랜덤 패턴 실행
		clear(x, y);
	}
	else if (t >= 10.30 && n == 137)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 9.80 && n == 136)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 9.75 && n == 135)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 9.70 && n == 134)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 9.55 && n == 133)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 9.00 && n == 132)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 8.95 && n == 131)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 8.90 && n == 130)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 8.70 && n == 129)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 8.10 && n == 128)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 8.05 && n == 127)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 8.00 && n == 126)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 7.75 && n == 125)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 7.10 && n == 124)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 7.05 && n == 123)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 7.00 && n == 122)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 6.70 && n == 121)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 6.00 && n == 120)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 5.95 && n == 119)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 5.90 && n == 118)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 5.55 && n == 117)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 4.80 && n == 116)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 4.75 && n == 115)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 4.70 && n == 114)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 4.30 && n == 113)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 3.50 && n == 112)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 3.45 && n == 111)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 3.40 && n == 110)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 2.95 && n == 109)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 2.10 && n == 108)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 2.05 && n == 107)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 2.00 && n == 106)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 1.50 && n == 105)
	{
		drawlazer1(91 + rand1 * 7);
		drawlazer2(30 + rand2 * 3);
		srand(time(NULL));
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
	else if (t >= 0.60 && n == 104)
	{
		gotoxy(90 + rand1 * 7, 25); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(80, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 26);
		drawGaster2(81, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 0.55 && n == 103)
	{
		gotoxy(90 + rand1 * 7, 24); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(79, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 25);
		drawGaster2(80, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 0.50 && n == 102)
	{
		clear(x, y);
		playSFX("charge.wav");
		gotoxy(90 + rand1 * 7, 23); printf("     ");
		for (int i = 30 + rand2 * 3; i <= 35 + rand2 * 3; i++) { gotoxy(78, i); printf(" "); }
		drawGaster1(91 + rand1 * 7, 24);
		drawGaster2(79, 30 + rand2 * 3);
		n++;
	}
	else if (t >= 0.02 && n == 101)
	{
		srand(time(NULL));
		rand1 = rand() % 3;
		rand2 = rand() % 3;
		n++;
	}
}
void gravityPat(double t, int x2, int y2)
{
	if (n == 100)
	{
		start_time = clock();
		n++;		
		movetype = 2;
	}

	if (t >= 17 && n == 132)
	{
		r--;
		n = 100;//앞으로 패턴은 100부터 시작
		srand(time(NULL));
		pattern = rand() % 3 + 1;//1~3중 랜덤 패턴 실행
		clear(x, y);
	}
	else if (t >= 15.75 && n == 131)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 38); printf("                    ");
			gotoxy(90, 39); printf("                    ");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("      ");
			}
		}
		Sleep(500);
		n++;
	}
	else if (t >= 15.73 && n == 130)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 38); printf("||||||||||||||||||||");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf(">-----");
			}
		}
		playSFX("ding.wav");
		n++;
	}
	else if (t >= 15.71 && n == 129)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(106, i); printf(">---");
			}
		}
		n++;
	}
	else if (t >= 15.69 && n == 128)
	{
		war = 0;
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(102, i); printf(" ");
			}
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(108, i); printf(">-");
			}
		}
		n++;
	}
	else if (t >= 14.19 && n == 127)
	{
		clear(x, y);
		playSFX("charge.wav");
		movetype = 2;
		war = 1;
		if (rand1 == 0)
		{
			gravity = 'y';
			int tmp = y;
			gotoxy(x, y); printf("♥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
			textcolor(WHITE, 0);
		}
		else
		{
			gravity = 'x';
			int tmp = x;
			gotoxy(x, y); printf("❥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("█");
			}
			textcolor(WHITE, 0);
		}
		n++;
	}
	else if (t >= 13.14 && n == 126)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 38); printf("                    ");
			gotoxy(90, 39); printf("                    ");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("      ");
			}
		}
		srand(time(NULL));
		rand1 = 0;
		Sleep(500);
		n++;
	}
	else if (t >= 13.12 && n == 125)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 38); printf("||||||||||||||||||||");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf(">-----");
			}
		}
		playSFX("ding.wav");
		n++;
	}
	else if (t >= 13.10 && n == 124)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(106, i); printf(">---");
			}
		}
		n++;
	}
	else if (t >= 13.08 && n == 123)
	{
		war = 0;
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(102, i); printf(" ");
			}
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(108, i); printf(">-");
			}
		}
		n++;
	}
	else if (t >= 11.08 && n == 122)
	{
		clear(x, y);
		playSFX("charge.wav");
		movetype = 2;
		war = 1;
		if (rand1 == 0)
		{
			gravity = 'y';
			int tmp = y;
			gotoxy(x, y); printf("♥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
			textcolor(WHITE, 0);
		}
		else
		{
			gravity = 'x';
			int tmp = x;
			gotoxy(x, y); printf("❥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("█");
			}
			textcolor(WHITE, 0);
		}
		n++;
	}
	else if (t >= 10.53 && n == 121)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 38); printf("                    ");
			gotoxy(90, 39); printf("                    ");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("      ");
			}
		}
		srand(time(NULL));
		rand1 = rand() % 2;
		Sleep(500);
		n++;
	}
	else if (t >= 10.51 && n == 120)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 38); printf("||||||||||||||||||||");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf(">-----");
			}
		}
		playSFX("ding.wav");
		n++;
	}
	else if (t >= 10.49 && n == 119)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(106, i); printf(">---");
			}
		}
		n++;
	}
	else if (t >= 10.47 && n == 118)
	{
		war = 0;
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(102, i); printf(" ");
			}
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(108, i); printf(">-");
			}
		}
		n++;
	}
	else if (t >= 8.47 && n == 117)
	{
		clear(x, y);
		playSFX("charge.wav");
		movetype = 2;
		war = 1;
		if (rand1 == 0)
		{
			gravity = 'y';
			int tmp = y;
			gotoxy(x, y); printf("♥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
			textcolor(WHITE, 0);
		}
		else
		{
			gravity = 'x';
			int tmp = x;
			gotoxy(x, y); printf("❥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("█");
			}
			textcolor(WHITE, 0);
		}
		n++;
	}
	else if (t >= 7.92 && n == 116)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 38); printf("                    ");
			gotoxy(90, 39); printf("                    ");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("      ");
			}
		}
		srand(time(NULL));
		rand1 = rand() % 2;
		Sleep(500);
		n++;
	}
	else if (t >= 7.90 && n == 115)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 38); printf("||||||||||||||||||||");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf(">-----");
			}
		}
		playSFX("ding.wav");
		n++;
	}
	else if (t >= 7.88 && n == 114)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(106, i); printf(">---");
			}
		}
		n++;
	}
	else if (t >= 7.86 && n == 113)
	{
		war = 0;
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(102, i); printf(" ");
			}
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(108, i); printf(">-");
			}
		}
		n++;
	}
	else if (t >= 5.86 && n == 112)
	{
		clear(x, y);
		playSFX("charge.wav");
		movetype = 2;
		war = 1;
		if (rand1 == 0)
		{
			gravity = 'y';
			int tmp = y;
			gotoxy(x, y); printf("♥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
			textcolor(WHITE, 0);
		}
		else
		{
			gravity = 'x';
			int tmp = x;
			gotoxy(x, y); printf("❥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("█");
			}
			textcolor(WHITE, 0);
		}
		n++;
	}
	else if (t >= 5.31 && n == 111)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 38); printf("                    ");
			gotoxy(90, 39); printf("                    ");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("      ");
			}
		}
		srand(time(NULL));
		rand1 = rand() % 2;
		Sleep(500);
		n++;
	}
	else if (t >= 5.29 && n == 110)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 38); printf("||||||||||||||||||||");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf(">-----");
			}
		}
		playSFX("ding.wav");
		n++;
	}
	else if (t >= 5.27 && n == 109)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(106, i); printf(">---");
			}
		}
		n++;
	}
	else if (t >= 5.25 && n == 108)
	{
		war = 0;
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(102, i); printf(" ");
			}
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(108, i); printf(">-");
			}
		}
		n++;
	}
	else if (t >= 3.25 && n == 107)
	{
		clear(x, y);
		playSFX("charge.wav");
		movetype = 2;
		war = 1;
		if (rand1 == 0)
		{
			gravity = 'y';
			int tmp = y;
			gotoxy(x, y); printf("♥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
			textcolor(WHITE, 0);
		}
		else
		{
			gravity = 'x';
			int tmp = x;
			gotoxy(x, y); printf("❥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("█");
			}
			textcolor(WHITE, 0);
		}
		n++;
	}
	else if (t >= 2.7 && n == 106)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 38); printf("                    ");
			gotoxy(90, 39); printf("                    ");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("      ");
			}
		}
		srand(time(NULL));
		rand1 = rand() % 2;
		Sleep(500);
		n++;
	}
	else if (t >= 2.54 && n == 105)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 38); printf("||||||||||||||||||||");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf(">-----");
			}
		}
		playSFX("ding.wav");
		n++;
	}
	else if (t >= 2.52 && n == 104)
	{
		if (rand1 == 0)
		{
			gotoxy(90, 38); printf("VVVVVVVVVVVVVVVVVVVV");
			gotoxy(90, 39); printf("||||||||||||||||||||");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(106, i); printf(">---");
			}
		}
		n++;
	}
	else if (t >= 2.5 && n == 103)
	{
		war = 0;
		if (rand1 == 0)
		{
			gotoxy(90, 37); printf("                    ");
			gotoxy(90, 39); printf("VVVVVVVVVVVVVVVVVVVV");
		}
		else
		{
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(102, i); printf(" ");
			}
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(108, i); printf(">-");
			}
		}
		n++;
	}
	else if (t >= 0.5 && n == 102)
	{
		playSFX("charge.wav");
		movetype = 2;
		war = 1;
		if (rand1 == 0)
		{
			gravity = 'y';
			int tmp = y;
			gotoxy(x, y); printf("♥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			gotoxy(90, 37); printf("▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀");
			textcolor(WHITE, 0);
		}
		else
		{
			gravity = 'x';
			int tmp = x;
			gotoxy(x, y); printf("❥");
			speed = 2.0;
			playSFX("warning.wav");
			textcolor(RED1, 0);
			for (int i = 31; i <= 39; i++)
			{
				gotoxy(104, i); printf("█");
			}
			textcolor(WHITE, 0);
		}
		n++;
	}
	else if (t >= 0.02 && n == 101)
	{
		srand(time(NULL));
		rand1 = rand() % 2;//0또는 1로 설정해서 중력을 어느 방향으로 작용시킬지 결정		
		n++;
	}
}

void opening(int n)//n만큼 빠르게 시행(디버그용)
{	
	drawsans1();	
	playSFX("talk.wav");
	dialogue(91, 22, "정말 아름다운 날이야.", 50);	
	Sleep(1000-n);
	playSFX("talk.wav");
	dialogue(86, 22, "새들은 지저귀고, 꽃들은 피어나고...", 50);	
	Sleep(1000-n);
	playSFX("talk.wav");
	dialogue(89, 22, "이런날 너 같은 아이들은", 50);	
	Sleep(1000-n);	
	system("cls");
	playSFX("blink.wav");
	Sleep(500-n);
	playSFX("blink.wav");
	drawsans2();
	Sleep(1000-n);
	dialogue(87, 22, "지 옥 에 서 불 타 버 려 야 해 .", 100);//100이 기본
	Sleep(1000-n);
	cleardialogue();
	textcolor(BLUE1, 0);
	gotoxy(100, 35); printf("♥");
	playSFX("charge.wav");
	for (int i = 0;i < 4;i++)
	{
		Sleep(20-i*3);
		gotoxy(100, 35+i); printf(" ");
		gotoxy(100, 36+i); printf("♥");
	}	
	playSFX("slam.wav");
	movetype = 2;
	textcolor(RED1, 0);
	Sleep(100);
}

void main()
{	
	int xlim1 = 90, xlim2 = 109, ylim1 = 31, ylim2 = 39;//x,y제한치 변수로 설정
	double charge = 0.0, jumpPower = 0.0;//차지 시작한 시간,점프 힘,속도
	int isGround = 1, isCharge = 1;//땅에 붙어있는지, 차징중인지		
	removeCursor();
	drawTitle(35, 10);//Title표시	
	playBGM("TITLE.mp3");
	_getch();//아무 키나 누르면 다음 화면으로
	playSFX("intro.wav");
	system("cls");	
	stopBGM();
	Sleep(3000);
	playSFX("blink.wav");	
	opening(0);	
	start_time = clock();	
	int oldx = x, oldy = y; //이전 위치 저장을 위한 변수
	while (1) {
		double during = ((double)(clock() - start_time) / CLOCKS_PER_SEC); //얼마나 시간이 경과했는지		
		oldx = x;//이전 위치 저장
		oldy = y;
		if (movetype == 1)//빨간 하트일 때
		{
			if (GetAsyncKeyState(VK_UP) & 0x8000 && y > ylim1)//GetAsyncKeyState(VK_UP)함수는 해당 키를 누르면 0x8000를 반환함. 따라서 &연산하면 1.
			{
				y -= 1;
			}
			else if (GetAsyncKeyState(VK_DOWN) & 0x8000 && y < ylim2)
			{
				y += 1;
			}
			else if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && x < xlim2)
			{
				x += 1;
			}
			else if (GetAsyncKeyState(VK_LEFT) & 0x8000 && x > xlim1)
			{
				x -= 1;
			}
		}
		else if (movetype == 2)//파란하트
		{
			if (gravity == 'y')
			{
				if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && x < xlim2)
				{
					x += 1;
				}
				else if (GetAsyncKeyState(VK_LEFT) & 0x8000 && x > xlim1)
				{
					x -= 1;
				}
				if (isGround && GetAsyncKeyState(VK_UP) & 0x8000)
				{
					if (!isCharge) //차지 시작
					{
						charge = during;
						isCharge = 1;
						jumpPower = 0.0;
					}
					double chargeTime = during - charge;
					jumpPower = chargeTime * 3.0;
					if (jumpPower > 2.0) jumpPower = 2.0;
				}
				else if (isCharge) //키를 뗌 (점프 실행)
				{
					if (jumpPower > 0.0)
					{
						speed = -jumpPower;						
					}
					isCharge = 0;
					jumpPower = 0.0;
				}
				if (y == ylim2 && speed == 0)//바닥에 닿으면 1 아니면 0
				{
					isGround = 1;
				}
				else
				{
					isGround = 0;
				}
				if (!isGround)
				{
					speed += 0.2;
					if (speed == 0)speed += 0.2;
					y += (int)speed;				
					if (y >= ylim2)
					{
						y = ylim2;
						speed = 0;
						isGround = 1;
					}
				}
			}
			else if(gravity=='x')
			{
				if (GetAsyncKeyState(VK_UP) & 0x8000 && y > ylim1)
				{
					y -= 1;
				}
				else if (GetAsyncKeyState(VK_DOWN) & 0x8000 && y < ylim2)
				{
					y += 1;
				}
				if (isGround && GetAsyncKeyState(VK_LEFT) & 0x8000)
				{
					if (!isCharge) //차지 시작
					{
						charge = during;
						isCharge = 1;
						jumpPower = 0.0;
					}
					double chargeTime = during - charge;
					jumpPower = chargeTime * 3.0;
					if (jumpPower > 2.5) jumpPower = 2.5;
				}
				else if (isCharge) //키를 뗌 (점프 실행)
				{
					if (jumpPower > 0.0)
					{
						speed = -jumpPower;
					}
					isCharge = 0;
					jumpPower = 0.0;
				}
				if (x == xlim2&&speed==0)//바닥에 닿으면 1 아니면 0
				{
					isGround = 1;
				}
				else
				{
					isGround = 0;
				}
				if (!isGround)
				{
					speed += 0.2;
					if (speed == 0)speed += 0.2;
					x += (int)speed;			
					if (x >= xlim2)
					{
						x = xlim2;
						speed = 0;
						isGround = 1;
					}
				}
			}
			
		}
		if (movetype == 2)//파란 하트일 때 중력 작용+점프
		{			
			if (gravity == 'y')
			{
				if (isCharge && GetAsyncKeyState(VK_UP) & 0x8000)//계속 위 화살표를 누르고 있는가 
				{
					double chargeTime = during - charge;
					jumpPower = chargeTime * 3.0;  //최대 점프 힘 조정 
					if (jumpPower > 2.0) jumpPower = 2.0;  //최대 높이 제한
				}
				else if (isCharge)//키를 뗐을 때
				{
					if (jumpPower > 0.0)
					{
						speed = -jumpPower;  //위로 속도 부여
						isGround = 0;
					}
					isCharge = 0;
					jumpPower = 0.0;
				}
			}
			else if (gravity == 'x')
			{
				if (isCharge && GetAsyncKeyState(VK_LEFT) & 0x8000)//계속 왼쪽 화살표를 누르고 있는가 
				{
					double chargeTime = during - charge;
					jumpPower = chargeTime * 3.0;  //최대 점프 힘 조정 
					if (jumpPower > 2.5) jumpPower = 2.5;  //최대 높이 제한
				}
				else if (isCharge)//키를 뗐을 때
				{
					if (jumpPower > 0.0)
					{
						speed = -jumpPower;  //위로 속도 부여
						isGround = 0;
					}
					isCharge = 0;
					jumpPower = 0.0;
				}
			}
		}
		if (oldx != x || oldy != y)//이동했다면 하트 출력
		{
			gotoxy(oldx, oldy);
			printf(" ");		
			if (!canHit)
			{
				textcolor(WHITE, 0);
			}
			else
			{
				if (movetype == 1) textcolor(RED1, 0);
				else if (movetype == 2) textcolor(BLUE1, 0);
			}

			if (!IsSpaceAt(x, y)&&canHit)//이동할 좌표가 공백이 아니라면(충돌한다면) + 무적이 아니라면 
			{
				if (n != 1)//경고표시랑은 부딪혀도 데미지 안입게
				{
					if (war != 1)
					{
						getDamage();
					}					
				}
			}
			else//else로 나누지 않으면 라이프 부분에 하트가 찍히는 버그 발생
			{
				gotoxy(x, y);
				if (gravity == 'y')
				{
					printf("♥");
				}
				else
				{
					printf("❥");
				}
				textcolor(WHITE, 0);
			}			
		}
		else
		{
			char ch = GetCharAt(x, y);
			if (ch != 'e' && canHit && ch!=' ')//♥는 e로 인식함. 즉, 현재 위치에 하트가 아니라 다른 문자가 있고, 무적이 아니라면 피해
			{
				if (n != 1)//경고표시랑은 부딪혀도 데미지 안입게
				{
					getDamage();
				}
			}
		}
		if (canHit == 0)//5초 무적 구현
		{
			double abtimer = ((double)(clock() - abtime) / CLOCKS_PER_SEC);//무적 시간초 계산
			if (abtimer >= 5.0)
			{
				canHit = 1;
			}
		}					
		bone1(during);
		bone2(during);
		gaster3(during,x,y);
		gaster4(during, x, y);
		gaster5(during, x, y);
		gaster6(during, x, y);
		if (r == 0 || life==0)//라운드를 다 버티거나, 체력이 0이 되면 게임종료
		{
			break;
		}
		else if (pattern == 1)
		{
			jumpPat(during, x, y);
		}
		else if (pattern == 2)
		{
			lazerPat(during, x, y);
		}
		else if (pattern == 3)
		{
			gravityPat(during, x, y);
		}
		Sleep(30);
	}

	system("cls");
	if (life == 0)
	{
		playBGM("DETERMINATION.mp3");
		for (int i = 0; i < 7; i++)
		{
			drawheart(74, 20);
			Sleep(100);
			drawheart(76, 20);
			Sleep(100);
		}
		drawheart2(75, 20);
		Sleep(1200);
		drawheart3(75, 20);
		Sleep(1100);
		drawgameover(66, 10);
		while (_kbhit()) _getch();
		while (!_kbhit()) {
			Sleep(100);
		}
		_getch();
		playSFX("intro.wav");
		system("cls");
		stopBGM();
		Sleep(3000);
	}
	else if(r==0)
	{
		stopBGM();
		gotoxy(81, 0); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
		gotoxy(81, 1); printf("        ▄▄▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄▄        \n");
		gotoxy(81, 2); printf("      ▄▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▄      \n");
		gotoxy(81, 3); printf("     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓     \n");
		gotoxy(81, 4); printf("    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓    \n");
		gotoxy(81, 5); printf("    ▓▓▓▓▓▓▓▓▓▓▓▀▓▓▓▓▓▓▓▀▓▓▓▓▓▓▓▓▓▓▓    \n");
		gotoxy(81, 6); printf("    ▓▓▄▓▓▓▓▓▓▓▀▄▓▓▓▓▓▓▓▄▀▓▓▓▓▓▓▓▄▓▓    \n");
		gotoxy(81, 7); printf("     ▓▓▄▄      ▓▓▓▓  ▓▓▓      ▄▄▓▓     \n");
		gotoxy(81, 8); printf("      ▓▓▓▓▓▓▀▄▓▓▓▓    ▓▓▓▄▀▀▓▓▓▓▓      \n");
		gotoxy(81, 9); printf("     ▓▓▓▓▓ ▀▓▓▓▓▓▓▄▄▄▄▓▓▓▓▓▓▓ ▀▓▓▓     \n");
		gotoxy(81, 10); printf("     ▓▓▓▄▄ ▓ ▄▀▀▀▀▀▀▀▀▀▀▀▀▀▄  ▄▄▓▓     \n");
		gotoxy(81, 11); printf("      ▓▓▓▓▓▄ ▀▓▓ ▓▓ ▓▓▓ ▓▓ ▀▄▓▓▓▓  ▄▄▄▄\n");
		gotoxy(81, 12); printf("▄▓▓▓▄  ▀▓▓▓▓▓▓▄▄ ▀▀ ▀▄▄ ▀▄▄▓▓▓▓▀ ▄▓▓▓▀▀\n");
		gotoxy(81, 13); printf("▀▀▀▓▓▓▄▄  ▀▄▄▄▓▓▓▓▓▓▓▓▓▓▓▓▀▀▀   ▓▓▓▀  ▀\n");
		gotoxy(81, 14); printf("      ▀▀▓▓▄     ▄     ▄      ▄▄▓▓▀     \n");
		gotoxy(81, 15); printf("          ▀▓▓▄▄ ▀▓▓▓▓▓▀  ▄▄▄▓▀▀▀   ▄▀ \n");
		gotoxy(81, 16); printf("           ▓  ▓▀  ▀▀▀▀ ▀▓▀▓        ▓  \n");
		gotoxy(81, 17); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
		gotoxy(81, 18); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
		gotoxy(81, 19); printf("             ▄▄▄▄▄▄▄▄▄▄▄▄▄             \n");
		dialogue(94, 22, ". . . 파피루스", 200);
		_getch();
		playSFX("dust.wav");
		for (int i = 19;i >= 0;i--)
		{
			gotoxy(81, i); printf("                                       ");
			Sleep(70);
		}
	}
}

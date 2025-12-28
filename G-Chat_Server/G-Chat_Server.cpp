// G-Chat_Server.cpp : Defines the entry point for the application.
//
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include "G-Chat_Server.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <Windowsx.h>//for edit_setreadonly function.
#include <MMSystem.h>
#include <list>

using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "27015"
char recvbuf[DEFAULT_BUFLEN];
char sendbuf[DEFAULT_BUFLEN];

#define MAX_LOADSTRING 100
#define IP_TXT_ID 1
#define CALL_BTN_ID 2
#define CHAT_TXT_ID 3
#define MSG_TXT_ID 4
#define SEND_BTN_ID 5
#define CON_LBL_ID 6
#define DISCON_BTN_ID 7
#define IDC_SVC 8
#define IDC_DVC 9
#define INP_BUFFER_SIZE 1024

//MESSAGE TYPE FOR SENDING COMMUNICATION
#define CONNECT 100
#define CHAT 101
#define VOICE_CHAT 102

//SEND STRUCTURE
typedef struct send_message
{
	int send;
	int msg_type;
	char *msg;
}SEND_MESSAGE;
SEND_MESSAGE my_msg;

SCROLLINFO si;
static HWND ip_text_hwnd, call_btn_hwnd, msg_txt_hwnd,msg_txt_hwnd2, chat_txt_hwnd, send_btn_hwnd, discon_btn_hwnd, hwnd_svc, hwnd_svc2, hwnd_dvc;
bool connected = false;
bool program_alive = true;
char connect_code[] = "***connect***";
char vc_chat_code[] = "###vc_chat###";
char vc_end_code[]= "###vc_end####";
char connection_code[14];
char client_ip[15];

//Voice chat variables.
int is_voice = 0;
bool voice_chat_on = false;
list<PBYTE> voice_list;
static bool bRecording, bPlaying, bEnding, bTerminating;
static DWORD dwDataLangth, dwRepetitions;
static HWAVEIN hWaveIn;
static HWAVEOUT hWaveOut;
static PBYTE BufferRecord1, BufferRecord2;
static PWAVEHDR WaveHdrRecord1, WaveHdrRecord2, WaveHdrPlay;
static TCHAR szOpenError[] = TEXT ("Error opening waveform audio!");
static TCHAR szmemError[] = TEXT ("Error allocating memory!");
static WAVEFORMATEX waveform;

// Global Variables:
HINSTANCE hInst;								// current instance
HWND window_hWnd;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void disable_all();
void enable_all();
int Listen();
bool record_init(HWND hWnd);
bool play_init(HWND hWnd);
bool vc_request();
void record_voice();
void play_voice();
void update_chat(char *msg);
DWORD WINAPI Listen_Thread(__in LPVOID lpParameter);
int send(char *msg, int msg_type);
DWORD WINAPI Send_Thread(__in LPVOID lpParameter );

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GCHAT_SERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GCHAT_SERVER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}




ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT_SERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+2);
	wcex.lpszMenuName	= NULL;//MAKEINTRESOURCE(IDC_GCHAT_SERVER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX,
      CW_USEDEFAULT, 0, 400, 600, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   window_hWnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	wchar_t *ip, *user_msg;
	int size;
	PBYTE voice_chunk;
	switch (message)
	{
	case WM_CREATE:
		CreateThread(0, 0, &Listen_Thread,0,0,0);
		CreateThread(0, 0, &Send_Thread,0,0,0);

		CreateWindow (TEXT("static"), TEXT("Enter IP"), WS_VISIBLE | WS_CHILD , 50, 30, 60, 20, hWnd, (HMENU) CON_LBL_ID, NULL, NULL);
		ip_text_hwnd = CreateWindow (TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL , 120, 30, 120, 20, hWnd, (HMENU) IP_TXT_ID, NULL, NULL);
		call_btn_hwnd = CreateWindow (TEXT("button"), TEXT("Call"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 250, 30, 60, 20, hWnd, (HMENU) CALL_BTN_ID, NULL, NULL);

		chat_txt_hwnd = CreateWindow (TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER |ES_MULTILINE | WS_TABSTOP/*| ES_AUTOHSCROLL*/ | WS_VSCROLL ,
									50, 60, 230, 300, hWnd, (HMENU) CHAT_TXT_ID, NULL, NULL);

		msg_txt_hwnd = msg_txt_hwnd2 = CreateWindow (TEXT("edit"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER |ES_MULTILINE | WS_TABSTOP/*| ES_AUTOHSCROLL*/ | ES_AUTOVSCROLL ,
									50, 370, 230, 80, hWnd, (HMENU) MSG_TXT_ID, NULL, NULL);

		send_btn_hwnd = CreateWindow (TEXT("button"), TEXT("Send"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 300, 370 + 30, 60, 20, hWnd, (HMENU) SEND_BTN_ID, NULL, NULL);

		discon_btn_hwnd = CreateWindow (TEXT("button"), TEXT("Exit"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 120, 450 + 10 , 80, 20, hWnd, (HMENU) DISCON_BTN_ID, NULL, NULL);

		hwnd_svc = hwnd_svc2 = CreateWindow (TEXT("button"), TEXT("Start Voice Chat"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 50, 480 + 10 , 120, 20, hWnd, (HMENU) IDC_SVC, NULL, NULL);

		hwnd_dvc = CreateWindow (TEXT("button"), TEXT("Stop Voice Chat"), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 200, 480 + 10 , 120, 20, hWnd, (HMENU) IDC_DVC, NULL, NULL);
		disable_all();	
		Edit_SetReadOnly(chat_txt_hwnd,true);

		WaveHdrRecord1 = (WAVEHDR *) malloc(sizeof(WAVEHDR));
		WaveHdrRecord2 = (WAVEHDR *) malloc(sizeof(WAVEHDR));
		WaveHdrPlay = (WAVEHDR *) malloc(sizeof(WAVEHDR));
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case CALL_BTN_ID:
			size = GetWindowTextLength(ip_text_hwnd)+sizeof(wchar_t);
			ip = (wchar_t*) calloc(1,size * sizeof(wchar_t));
			GetWindowText(ip_text_hwnd,ip,14);
			wcstombs(client_ip,ip,14);
			my_msg.msg_type = CONNECT;
			my_msg.msg = connect_code;
			my_msg.send = 1;
			break;
		case SEND_BTN_ID:
			strcpy(sendbuf,"\0");
			size = GetWindowTextLength(msg_txt_hwnd2);
			user_msg = (wchar_t*) calloc(1, (size + sizeof(wchar_t)) * sizeof(wchar_t));
			GetWindowText(msg_txt_hwnd2,user_msg,size+sizeof(wchar_t));
			//strcpy(sendbuf,"\n");
			wcstombs(sendbuf,user_msg,size+sizeof(wchar_t));
			//strcat(sendbuf,"\n");
			strcat(sendbuf,"\r\nYou---------------------------\r\n");
			update_chat(sendbuf);

			my_msg.msg_type = CHAT;
			my_msg.msg = sendbuf;
			my_msg.send = 1;

			free(user_msg);
			
			SetWindowText(msg_txt_hwnd2,TEXT(""));
			break;

		case DISCON_BTN_ID:
			program_alive = false;
			PostQuitMessage(0);
			break;

		case IDC_SVC:
			//this ensure system has microphone and speaker attached and prepaired
			//required header.

			if(record_init(hWnd), play_init(hWnd)) 
			{
				
				
				my_msg.msg_type = VOICE_CHAT;
				my_msg.msg = vc_chat_code;
				my_msg.send = 1;
				voice_chat_on = true;
				
			}
			break;

		case IDC_DVC:
			
			EnableWindow(send_btn_hwnd,true);
			EnableWindow(hwnd_svc2,true);

			my_msg.msg_type = CHAT;
			my_msg.msg = vc_end_code;
			my_msg.send = 1;
			voice_chat_on = false;
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case MM_WIM_OPEN:
		break;

	case MM_WIM_DATA:

		voice_chunk = (PBYTE) malloc(((PWAVEHDR) lParam)->dwBytesRecorded);
		CopyMemory(voice_chunk, ((PWAVEHDR) lParam)->lpData, ((PWAVEHDR) lParam)->dwBytesRecorded);
		/*voice_list.push_back(voice_chunk);*/
		//is_voice += 1;
		if(voice_chat_on == false)
		{
			waveInClose(hWaveIn);
			return true;
		}

		waveInAddBuffer(hWaveIn, (PWAVEHDR) lParam, sizeof(WAVEHDR));
		my_msg.msg_type = CHAT;
		my_msg.msg = (char*) voice_chunk;
		my_msg.send = 1;

		break;

	case MM_WIM_CLOSE:
		waveInUnprepareHeader(hWaveIn, WaveHdrRecord1, sizeof(WAVEHDR));
		//waveInUnprepareHeader(hWaveIn, WaveHdrRecord2, sizeof(WAVEHDR));
		free(BufferRecord1);
		free(BufferRecord2);
		break;

	case MM_WOM_OPEN:
		break;

	case MM_WOM_DONE:
		if(voice_chat_on && is_voice > 0)
		{
			free(WaveHdrPlay->lpData);
			play_voice();
		}

		if(voice_chat_on == false)
		{
			waveOutUnprepareHeader(hWaveOut, WaveHdrPlay, sizeof(WAVEHDR));
			waveOutClose(hWaveOut);
		}

		break;

	case MM_WOM_CLOSE:
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		program_alive = false;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void disable_all()
{
	EnableWindow(chat_txt_hwnd,false);
	EnableWindow(msg_txt_hwnd,false);
	EnableWindow(send_btn_hwnd,false);
	EnableWindow(discon_btn_hwnd,false);
	EnableWindow(hwnd_svc,false);
	EnableWindow(hwnd_dvc,false);
}

void enable_all()
{
	EnableWindow(chat_txt_hwnd,true);
	EnableWindow(msg_txt_hwnd,true);
	EnableWindow(send_btn_hwnd,true);
	EnableWindow(discon_btn_hwnd,true);
	EnableWindow(hwnd_svc,true);
}

int Listen()
{
	 WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;
	SOCKADDR_IN    client_addr = {0} ;
	//char client_ip[14];
    int iSendResult;
    //char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN + 50;
    

			// Initialize Winsock
			iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
			if (iResult != 0) {
				printf("WSAStartup failed with error: %d\n", iResult);
				return 1;
			}

			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM /*SOCK_DGRAM*/;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			// Resolve the server address and port
			iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
			if ( iResult != 0 ) {
				//printf("getaddrinfo failed with error: %d\n", iResult);
				WSACleanup();
				return 1;
			}

			// Create a SOCKET for connecting to server
			ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
			if (ListenSocket == INVALID_SOCKET) {
				//printf("socket failed with error: %ld\n", WSAGetLastError());
				freeaddrinfo(result);
				WSACleanup();
				return 1;
			}

			// Setup the TCP listening socket
			iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
			if (iResult == SOCKET_ERROR) {
				//printf("bind failed with error: %d\n", WSAGetLastError());
				freeaddrinfo(result);
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			}

			freeaddrinfo(result);
	
	/*do
	{*/
			iResult = listen(ListenSocket, SOMAXCONN);
			if (iResult == SOCKET_ERROR) {
				//printf("listen failed with error: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			}

			// Accept a client socket
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				//printf("accept failed with error: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			}

			// No longer need server socket
			//closesocket(ListenSocket);
			
			// Receive until the peer shuts down the connection
			do {

				iResult = recv(ClientSocket,recvbuf, recvbuflen, 0);
				if (iResult > 0)
				{
					//printf("%s\n", recvbuf);
					strncpy(connection_code,recvbuf,13);
					if(voice_chat_on == false && strlen(recvbuf) > 34)
					{
						recvbuf[iResult-34] = '\0';
					}
					else
					{
						recvbuf[iResult] = '\0';
					}
					
					if(strcmp(connection_code,connect_code) == 0 || strcmp(connection_code,vc_chat_code) == 0 || strcmp(connection_code,vc_end_code) == 0)
					{
						socklen_t  s = sizeof(client_addr);
						s = getpeername(ClientSocket, (struct sockaddr *) &client_addr,&s);
						char *ip = inet_ntoa(client_addr.sin_addr);
						if(strcmp(connection_code,connect_code) == 0 )
						{

							wchar_t masg[32];
						 
							mbstowcs(masg,ip,strlen(ip)+sizeof(wchar_t));
							wcscat(masg,L" Want to connect");
							if(MessageBox(window_hWnd,masg,L"Connect",MB_OKCANCEL) == IDOK)
							{
								 send( ClientSocket, connect_code, (int)strlen(connect_code), 0 );
								 enable_all();
								 wchar_t cip[15];
								 mbstowcs(cip,ip,15);
								 SetWindowText(ip_text_hwnd,cip);
								 strcpy(client_ip,ip);
								 connected = true;
							 }
							 else
							 {
								 send( ClientSocket, "NO", (int)strlen(connect_code), 0 );
							 }
						}

						if(strcmp(connection_code,vc_chat_code) == 0 )
						{
							if(MessageBox(window_hWnd,L"Voice Chat Request",L"Request",MB_OKCANCEL) == IDOK)
							{
								
								if(voice_chat_on == false)
								{
									voice_chat_on = true;

									if(record_init(window_hWnd) && play_init(window_hWnd))
									{
										record_voice();
										send( ClientSocket, vc_chat_code, (int)strlen(vc_chat_code), 0 );
										EnableWindow(send_btn_hwnd, false);
									}
									else
									{
										send( ClientSocket, "NO", (int)strlen("NO"), 0 );
									}
								}
								else
								{
									send( ClientSocket, vc_chat_code, (int)strlen(vc_chat_code), 0 );
								}
							 }
							 else
							 {
								 send( ClientSocket, "NO", (int)strlen(connect_code), 0 );
							 }
						}


						if(strcmp(connection_code,vc_end_code) == 0 && voice_chat_on == true)
						{
							voice_chat_on = false;
							waveInReset(hWaveIn);
							EnableWindow(send_btn_hwnd, true);
							EnableWindow(hwnd_svc2, true);
							EnableWindow(hwnd_dvc, false);

						}
					}
					else
					{
						if(voice_chat_on)
						{
							PBYTE voice_chunk = (PBYTE) calloc(1,sizeof(PBYTE) * DEFAULT_BUFLEN);
							CopyMemory(voice_chunk, recvbuf, DEFAULT_BUFLEN);
							voice_list.push_back(voice_chunk);
							is_voice += 1;
							if(is_voice == 1)
							{
								play_voice();
							}
						}
						else
						{
							strcat(recvbuf,"\r\nOther-----------------------\r\n");
							update_chat(recvbuf);
						}

					}
					
					strcpy(recvbuf,"");
					
				}
				else if (iResult == 0)
					;//printf("Connection closing...\n");
				else  
				{
					printf("recv failed with error: %d\n", WSAGetLastError());
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
				}

			} while (iResult > 0);

			// shutdown the connection since we're done
			iResult = shutdown(ClientSocket, SD_SEND);
			if (iResult == SOCKET_ERROR) {
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
	//}while(connected);


		// cleanup
		closesocket(ClientSocket);
		WSACleanup();

	
    return 0;
}

void update_chat(char *msg)
{
	int size;
	int max_pos=0;
	int min;
	wchar_t *chat_msg, *temp;
	RECT rect;
	
	
	size = strlen(msg)+GetWindowTextLength(chat_txt_hwnd)+sizeof(wchar_t)+5;
	chat_msg = (wchar_t *)  calloc(1,sizeof(wchar_t) * size);
	temp = (wchar_t*) calloc(1,(strlen(msg)+sizeof(wchar_t)) * sizeof(wchar_t));
	mbstowcs(chat_msg,msg,strlen(msg));
	GetWindowText(chat_txt_hwnd,chat_msg + wcslen(chat_msg), GetWindowTextLength(chat_txt_hwnd)+sizeof(wchar_t));
	//wcscat(chat_msg,TEXT(" "));
	//wcscat(chat_msg,temp);
	
	
	SetWindowText(chat_txt_hwnd, chat_msg);
	GetScrollRange(chat_txt_hwnd,SB_VERT,&min,&max_pos);
	GetScrollInfo(chat_txt_hwnd,SB_VERT,&si);
	

	
	
	free(chat_msg);
	free(temp);
}
DWORD WINAPI Listen_Thread(__in LPVOID lpParameter)
{
	while(program_alive)
	{
			Listen();
	}
	return 0;
}


int send(char *msg, int msg_type)
{
	 WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char sendbuf[DEFAULT_BUFLEN];
  
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        //printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM /*SOCK_DGRAM*/;
    hints.ai_protocol = IPPROTO_TCP;
	
    // Resolve the server address and port
    iResult = getaddrinfo(client_ip, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) 
	{
       // printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
	{

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            //printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) 
		{
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
	{
        //printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
	

			iResult = send( ConnectSocket,msg, (int)strlen(msg), 0 );
			if (iResult == SOCKET_ERROR) 
			{
				//printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}
			if(msg_type == CONNECT || msg_type == VOICE_CHAT)
			{
				char *con_reply = (char*) calloc(1,sizeof(char) * 20);
				recv(ConnectSocket, con_reply, 20, 0);
				if(msg_type == CONNECT)
				{
					if(strcmp(con_reply,connect_code) == 0)
					{
						enable_all();
						MessageBox(window_hWnd,L"connected",L"Connected",NULL);
					}
					else
					{
						MessageBox(window_hWnd,L"Connection Call Not Accepted",L"Not Connected",NULL);
						disable_all();
					}
				}
				else
				{
					if(strcmp(con_reply,vc_chat_code) == 0)
					{
						EnableWindow(hwnd_svc,false);
						EnableWindow(hwnd_dvc,true);
						EnableWindow(send_btn_hwnd,false);
						voice_chat_on = true;
						record_voice();
					}
					else
					{
						MessageBox(window_hWnd,L"Voice Chat Call Not Accepted",L"Not Accepted",NULL);
					}
				}
				
			}
			// shutdown the connection since no more data will be sent
			iResult = shutdown(ConnectSocket, SD_SEND);
			if (iResult == SOCKET_ERROR)
			{
				printf("shutdown failed with error: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				WSACleanup();
				return 1;
			}

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
	
    return 0;
}

DWORD WINAPI Send_Thread(__in LPVOID lpParameter )
{
	
	while(program_alive)
	{
			
		/*sprintf (a, "\nvalue of  send %d",my_msg.send);
			OutputDebugStringA (a);*/
		Sleep(10);
		if(my_msg.send == 1)
		{
			send(my_msg.msg,my_msg.msg_type);
			my_msg.send = 0;
		}
		
	}
	return 0;
}

bool record_init(HWND hWnd)
{
	waveform.wFormatTag = WAVE_FORMAT_PCM;
	waveform.nChannels = 1;
	waveform.nSamplesPerSec = 11025;
	waveform.nAvgBytesPerSec = 11025;
	waveform.nBlockAlign = 1;
	waveform.wBitsPerSample = 8;
	waveform.cbSize = 0;

	if (waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD) hWnd,0,CALLBACK_WINDOW))
	{
		free(BufferRecord1);
		free(BufferRecord2);
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox(hWnd,szOpenError,L"error",MB_ICONEXCLAMATION|MB_OK);
		return false;
	}

	BufferRecord1 = (PBYTE) malloc (INP_BUFFER_SIZE);
	BufferRecord2 = (PBYTE) malloc (INP_BUFFER_SIZE);

	if(WaveHdrRecord1 == NULL)
	{
		WaveHdrRecord1 = (WAVEHDR *) malloc(sizeof(WAVEHDR));
	}
	WaveHdrRecord1->lpData			= (LPSTR) BufferRecord1;
	WaveHdrRecord1->dwBufferLength	= INP_BUFFER_SIZE;
	WaveHdrRecord1->dwBytesRecorded	= 0;
	WaveHdrRecord1->dwUser			= 0;
	WaveHdrRecord1->dwFlags			= 0;
	WaveHdrRecord1->dwLoops			= 1;
	WaveHdrRecord1->lpNext			= NULL;
	WaveHdrRecord1->reserved		= 0;

	waveInPrepareHeader(hWaveIn, WaveHdrRecord1, sizeof(WAVEHDR));

	if(WaveHdrRecord2 == NULL)
	{
		WaveHdrRecord2 = (WAVEHDR *) malloc(sizeof(WAVEHDR));
	}
	WaveHdrRecord2->lpData			= (LPSTR) BufferRecord2;
	WaveHdrRecord2->dwBufferLength	= INP_BUFFER_SIZE;
	WaveHdrRecord2->dwBytesRecorded	= 0;
	WaveHdrRecord2->dwUser			= 0;
	WaveHdrRecord2->dwFlags			= 0;
	WaveHdrRecord2->dwLoops			= 1;
	WaveHdrRecord2->lpNext			= NULL;
	WaveHdrRecord2->reserved		= 0;

	waveInPrepareHeader(hWaveIn, WaveHdrRecord2, sizeof(WAVEHDR));

	return true;

}

bool play_init(HWND hWnd)
{
	waveform.wFormatTag = WAVE_FORMAT_PCM;
	waveform.nChannels = 1;
	waveform.nSamplesPerSec = 11025;
	waveform.nAvgBytesPerSec = 11025;
	waveform.nBlockAlign = 1;
	waveform.wBitsPerSample = 8;
	waveform.cbSize = 0;

	if(waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveform, (DWORD)hWnd, 0 , CALLBACK_WINDOW))
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBox( hWnd, szOpenError, L"error", MB_ICONEXCLAMATION|MB_OK);
		return false;
	}

	return true;
}

void record_voice()
{
	waveInAddBuffer(hWaveIn,WaveHdrRecord1, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn,WaveHdrRecord2, sizeof(WAVEHDR));
	waveInStart(hWaveIn);
}

void play_voice()
{
	if(is_voice > 0)
	{
			
		
		WaveHdrPlay->lpData				= (LPSTR) voice_list.front();
		WaveHdrPlay->dwBufferLength		= INP_BUFFER_SIZE;//dwDataLangth;
		WaveHdrPlay->dwBytesRecorded	= 0;
		WaveHdrPlay->dwUser				= 0;
		WaveHdrPlay->dwFlags			= WHDR_BEGINLOOP|WHDR_ENDLOOP;
		WaveHdrPlay->dwLoops			= dwRepetitions;
		WaveHdrPlay->lpNext				= NULL;
		WaveHdrPlay->reserved			= 0;

		waveOutPrepareHeader(hWaveOut, WaveHdrPlay, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, WaveHdrPlay, sizeof(WAVEHDR));
		
		is_voice--;
		voice_list.pop_front();
	}
	else
	{
		MessageBox(window_hWnd,L"no voice",NULL,NULL);
	}
}

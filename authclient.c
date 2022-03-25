#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <shellapi.h>
#include <winsock.h>


#define IDI_CONNECTED 1
#define IDI_DISCONNECTED 2
#define WM_SHELLNOTIFY WM_USER+5
#define SOCKET_READY WM_USER+10
#define WM_MENU_RESTORE WM_USER+15
#define WM_MENU_CONNECTION WM_USER+16
#define WM_MENU_CLOSE WM_USER+17
#define WM_MENU_ABOUT WM_USER+18



char host[100];
char port[10];
HWND hwnd, hwBtnSend, hwEdtLogin, hwEdtPasswd, hwLoginStatic, hwPasswdStatic, hwStatusStatic;
NOTIFYICONDATA m_NotifyIconData;
char buff[1024];
SOCKET my_sock;
char status=0;
int timerint=300;
HMENU	hPopupMenu;
typedef char * t_langstring;
t_langstring langstring[21];



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ShowResponse(char * respstr, long timeint){

  SetWindowText(hwEdtPasswd, "");

  if (!strcmp(respstr, "ACCEPT")){
      m_NotifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_CONNECTED));               
      Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData);
      SendMessage(hwnd,WM_SYSCOMMAND, SC_MINIMIZE, 0);
      SetWindowText(hwStatusStatic, langstring[0]);
      SetTimer(hwnd, 1, timeint*1000, NULL);
      status=2;
      EnableWindow(hwBtnSend, TRUE);
  }
  else if (!strcmp(respstr, "REJECT")){
       SetWindowText(hwStatusStatic, langstring[1]);
       SetTimer(hwnd, 2, 5000, NULL);
  }
  else if (!strcmp(respstr, "INVALID")){
       SetWindowText(hwStatusStatic, langstring[2]);
       SetTimer(hwnd, 2, 5000, NULL);
  }
  else if (!strcmp(respstr, "NORESPONSE")){
       SetWindowText(hwStatusStatic, langstring[3]);
       SetTimer(hwnd, 2, 5000, NULL);
  }
  else if (!strcmp(respstr, "FLOOD")){
       SetWindowText(hwStatusStatic, langstring[4]);
       status=1;
       SetTimer(hwnd, 2, 5000, NULL);
  }
  else {
       SetWindowText(hwStatusStatic, langstring[5]);
       SetTimer(hwnd, 2, 5000, NULL);
  }




}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int InitMySocket(void)
{

    EnableWindow(hwEdtPasswd, FALSE); 
    EnableWindow(hwEdtLogin, FALSE);
    EnableWindow(hwBtnSend, FALSE);
    status=0;
    // Шаг 1 - инициализация библиотеки Winsock
    if (WSAStartup(0x202,(WSADATA *)&buff[0]))
    {
      EnableWindow(hwEdtPasswd, TRUE);
      SetWindowText(hwStatusStatic, langstring[6]);
      return -1;
    }

    // Шаг 2 - создание сокета

    my_sock=socket(AF_INET,SOCK_STREAM,0);
    if (my_sock < 0)
    {
      EnableWindow(hwEdtPasswd, TRUE);
      EnableWindow(hwBtnSend, TRUE);
      SetWindowText(hwStatusStatic, langstring[7]);
      return -1;
    }

    // Шаг 3 - установка соединения, заполнение структуры sockaddr_in указание адреса и порта сервера
    SOCKADDR_IN dest_addr;
    dest_addr.sin_family=AF_INET;
    dest_addr.sin_port=htons(atoi(&port[0]));
    HOSTENT *hst;

    // преобразование IP адреса из символьного в сетевой формат
    if (inet_addr(host)!=INADDR_NONE)
      dest_addr.sin_addr.s_addr=inet_addr(host);
    else
      // попытка получить IP адрес по доменному имени сервера
      if (hst=gethostbyname(host))
         // hst->h_addr_list содержит не массив адресов, а массив указателей на адреса
         ((unsigned long *)&dest_addr.sin_addr)[0]=((unsigned long **)hst->h_addr_list)[0][0];
      else 
         {
         SetWindowText(hwStatusStatic, langstring[8]);
         EnableWindow(hwEdtPasswd, TRUE);
         EnableWindow(hwBtnSend, TRUE);
         closesocket(my_sock);
         WSACleanup();
         return -1;
         }

    // адрес сервера получен – пытаемся установить соединение 
    if (connect(my_sock,(SOCKADDR *)&dest_addr, sizeof(dest_addr)))
       {
       EnableWindow(hwEdtPasswd, TRUE);
       EnableWindow(hwBtnSend, TRUE);
       SetWindowText(hwStatusStatic, langstring[9]);
       closesocket(my_sock);
       WSACleanup();
       return -1;
       }

     
    WSAAsyncSelect(my_sock, hwnd,  SOCKET_READY, FD_READ | FD_CLOSE);
    SetWindowText(hwBtnSend, langstring[10]);
    EnableMenuItem(hPopupMenu, WM_MENU_CONNECTION, MF_BYCOMMAND || MF_ENABLED);
    memset(&buff[0], '\0', 1024);
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CloseMySocket(void)
{
  if (status==2) 
      {
       SetWindowText(hwStatusStatic, langstring[11]);
       EnableWindow(hwEdtPasswd, TRUE);
      }
  status=0;
  SetWindowText(hwBtnSend, langstring[12]);
  EnableMenuItem(hPopupMenu, WM_MENU_CONNECTION, MF_BYCOMMAND || MF_GRAYED);
  m_NotifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DISCONNECTED));               
  Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData);
  closesocket(my_sock);
  WSACleanup();
  //EnableWindow(hwEdtPasswd, TRUE);
  //EnableWindow(hwBtnSend, TRUE);
  KillTimer(hwnd, 1);

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wparam,LPARAM lparam)
{
 POINT pt;
  
  
  switch (Message){
    
    case SOCKET_READY:
         {
         if (LOWORD(lparam) == FD_READ)
            {
            int nbytes = recv(my_sock, buff, sizeof(buff), 0);
            buff[nbytes]='\0';
            
            if ((status==0) && ((timerint=atoi(&buff[0]))>0)){
                status=1;
                char * tempbuf='\0';
                memset(&buff[0], '\0', 1024);
                tempbuf = malloc(50);
                //Считываем логин
                memset(tempbuf, '\0', 50);
                GetWindowText(hwEdtLogin, tempbuf, 49);
                strncat(&buff[0], tempbuf, strlen(tempbuf));
                //Считываем пароль
                strcat(&buff[0],"@");
                memset(tempbuf, '\0', 50);
                GetWindowText(hwEdtPasswd, tempbuf, 49);
                strncat(&buff[0], tempbuf, strlen(tempbuf));
                memset(tempbuf, '\0', 50);
                free(tempbuf);
                send(my_sock, buff, strlen(buff), 0);
                memset(&buff[0], '\0', 1024);
                }
            else 
                {
                ShowResponse(&buff[0], timerint);
                }
            
            }
          else
            {
            ShowWindow(hwnd, SW_RESTORE);
            CloseMySocket();
            }
         break;
         }

    case WM_DESTROY:
         {
         closesocket(my_sock);
         WSACleanup();
         m_NotifyIconData.uFlags = 0;
         KillTimer(hwnd, 1);
         DestroyMenu(hPopupMenu);
         Shell_NotifyIcon(NIM_DELETE, &m_NotifyIconData);
         PostQuitMessage(0);
         return 0;
         break;
         }

    case WM_TIMER:
         {
         if (wparam==1)
            {
             send(my_sock, "ALIVE", 5, 0);
            }
         else
            {
             KillTimer(hwnd, 2);
             EnableWindow(hwBtnSend, TRUE);
             EnableWindow(hwEdtPasswd, TRUE);
            }
             break;
         }

    case WM_CLOSE: 
         {         
         if (IDNO==MessageBox(hwnd, langstring[13], langstring[14], MB_YESNO | MB_ICONQUESTION))
             return 0;
         break;
         }
 
    case WM_COMMAND: 
         { 
         if ((wparam == BN_CLICKED) && (lparam == (int)hwBtnSend))
            {
             if (GetWindowTextLength(hwBtnSend)==12)
                 InitMySocket();
             else
                 CloseMySocket();
            }
         else if (lparam == 0)
            {
             switch(LOWORD (wparam))
                   {
                    case WM_MENU_RESTORE:
                         ShowWindow(hwnd, SW_RESTORE);
                         break;
                    case WM_MENU_CLOSE:
                         SendMessage(hwnd, WM_CLOSE, 0, 0); 
                         break;
                    case WM_MENU_CONNECTION:
                         if (GetWindowTextLength(hwBtnSend)==12)
                             InitMySocket();
                         else
                             CloseMySocket();
                         break;
                    case WM_MENU_ABOUT:
                         MessageBox(hwnd, "Author: Ardzhanov Anton\nE-Mail: antonardov@mail.ru\n", langstring[15], MB_OK | MB_ICONINFORMATION);
                         break;
                    }
            }
         break;
         }
    
    case WM_SYSCOMMAND:
         {
         if (wparam==SC_MINIMIZE) 
            {
            ShowWindow(hwnd, SW_HIDE);
            return 0;
            }
         break;
         }
    case WM_SHOWWINDOW:
         {
          if (wparam)
             {
              Shell_NotifyIcon(NIM_DELETE, &m_NotifyIconData);
             }
          else
             {
              Shell_NotifyIcon(NIM_ADD, &m_NotifyIconData);
             }
          break;
         }
    case WM_SHELLNOTIFY:
         {
          if (wparam == 10)
              {
              if (lparam == WM_LBUTTONDBLCLK)
                  {
                   ShowWindow(hwnd, SW_RESTORE);
                  }
              else if (lparam == WM_RBUTTONDOWN)
                  {
                  SetForegroundWindow(hwnd);
                  GetCursorPos(&pt);
                  TrackPopupMenu(hPopupMenu, TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                  }
              
              }
          break;
         }
    
   
  }


  return DefWindowProc(hwnd,Message,wparam,lparam);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
 
  HWND chkhwnd;
  chkhwnd=FindWindow("AuthWindow", "Auth Gateway Client");
  if (chkhwnd) {
      ShowWindow(chkhwnd, SW_SHOW);
      SetForegroundWindow(chkhwnd);
      return 1;
      }
   
  if (GetPrivateProfileString("server", "host", "", &host[0], 100, ".\\authclient.ini")<=0 || GetPrivateProfileString("server", "port", "", &port[0], 10, ".\\authclient.ini")<=0)
      {
       MessageBox(hwnd, "Can not open authclient.ini", "Message", MB_ICONERROR);
       return 2;
      }

  langstring[0]=calloc(70, sizeof(char));
  langstring[1]=calloc(70, sizeof(char));
  langstring[2]=calloc(70, sizeof(char));
  langstring[3]=calloc(70, sizeof(char));
  langstring[4]=calloc(70, sizeof(char));
  langstring[5]=calloc(70, sizeof(char));
  langstring[6]=calloc(70, sizeof(char));
  langstring[7]=calloc(70, sizeof(char));
  langstring[8]=calloc(70, sizeof(char));
  langstring[9]=calloc(70, sizeof(char));
  langstring[10]=calloc(20, sizeof(char));
  langstring[11]=calloc(70, sizeof(char));
  langstring[12]=calloc(20, sizeof(char));
  langstring[13]=calloc(100, sizeof(char));
  langstring[14]=calloc(20, sizeof(char));
  langstring[15]=calloc(20, sizeof(char));
  langstring[16]=calloc(10, sizeof(char));
  langstring[17]=calloc(10, sizeof(char));
  langstring[18]=calloc(70, sizeof(char));
  langstring[19]=calloc(20, sizeof(char));
  langstring[20]=calloc(20, sizeof(char));

  
  if (GetPrivateProfileString("language", "string1", "", langstring[0], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[0], "Access allowed"); }
  if (GetPrivateProfileString("language", "string2", "", langstring[1], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[1], "Incorrect username or password"); }
  if (GetPrivateProfileString("language", "string3", "", langstring[2], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[2], "Incorrect request"); }
  if (GetPrivateProfileString("language", "string4", "", langstring[3], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[3], "No response from auth server"); }
  if (GetPrivateProfileString("language", "string5", "", langstring[4], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[4], "Flood detected"); }
  if (GetPrivateProfileString("language", "string6", "", langstring[5], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[5], "Incorrect Response from auth gateway"); }
  if (GetPrivateProfileString("language", "string7", "", langstring[6], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[6], "Can not initialize WinSock2 library"); }
  if (GetPrivateProfileString("language", "string8", "", langstring[7], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[7], "Can not create socket"); }
  if (GetPrivateProfileString("language", "string9", "", langstring[8], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[8], "Can not resolve hostname to IP"); }
  if (GetPrivateProfileString("language", "string10", "", langstring[9], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[9], "Connection to auth gateway failed"); }
  if (GetPrivateProfileString("language", "string11", "", langstring[10], 20, ".\\authclient.ini") <= 0) { strcpy (langstring[10], "Disconnect"); }
  if (GetPrivateProfileString("language", "string12", "", langstring[11], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[11], "Connection reset"); }
  if (GetPrivateProfileString("language", "string13", "", langstring[12], 20, ".\\authclient.ini") <= 0) { strcpy (langstring[12], "Connect"); }
  if (GetPrivateProfileString("language", "string14", "", langstring[13], 100, ".\\authclient.ini") <= 0) { strcpy (langstring[13], "Are you really want to exit?"); }
  if (GetPrivateProfileString("language", "string15", "", langstring[14], 20, ".\\authclient.ini") <= 0) { strcpy (langstring[14], "Exit"); }
  if (GetPrivateProfileString("language", "string16", "", langstring[15], 20, ".\\authclient.ini") <= 0) { strcpy (langstring[15], "About"); }
  if (GetPrivateProfileString("language", "string17", "", langstring[16], 10, ".\\authclient.ini") <= 0) { strcpy (langstring[16], "Login:"); }
  if (GetPrivateProfileString("language", "string18", "", langstring[17], 10, ".\\authclient.ini") <= 0) { strcpy (langstring[17], "Pass:"); }
  if (GetPrivateProfileString("language", "string19", "", langstring[18], 70, ".\\authclient.ini") <= 0) { strcpy (langstring[18], "Enter data"); }
  if (GetPrivateProfileString("language", "string20", "", langstring[19], 20, ".\\authclient.ini") <= 0) { strcpy (langstring[19], "Restore"); }
  if (GetPrivateProfileString("language", "string21", "", langstring[20], 20, ".\\authclient.ini") <= 0) { strcpy (langstring[20], "Close"); }

  
  
  
  

  MSG msg;
  WNDCLASS w;

  // Заводим свой класс для окна
  memset(&w,0,sizeof(WNDCLASS));
  w.style = CS_HREDRAW | CS_VREDRAW;
  w.lpfnWndProc = WndProc;
  w.hInstance = hInstance;
  w.hbrBackground = (HBRUSH) (1 + COLOR_BTNFACE);
  w.lpszClassName = "AuthWindow";
  w.hCursor=LoadCursor(0, IDC_ARROW);
  w.hIcon=LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONNECTED));
  RegisterClass(&w);
  
  //Создаем окно
  hwnd = CreateWindowEx(WS_EX_TOPMOST,"AuthWindow","Auth Gateway Client", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,10,10,345,160,NULL,NULL,hInstance,NULL);
  
  //Создаем на окне элементы управления
  hwLoginStatic=CreateWindowEx(0,"Static", langstring[16], WS_CHILD | WS_VISIBLE , 10,35,60,20,hwnd,0,hInstance,0);
  hwPasswdStatic=CreateWindowEx(0,"Static", langstring[17], WS_CHILD | WS_VISIBLE, 10,60,60,20,hwnd,0,hInstance,0);
  hwStatusStatic=CreateWindowEx(0,"Static", langstring[18], WS_CHILD | WS_VISIBLE | SS_CENTER, 10,8,320,20,hwnd,0,hInstance,0);

  if (!strcmp(lpCmdLine, "SecretUsage"))
      {
      hwEdtLogin = CreateWindowEx(0,"EDIT","", WS_CHILD | WS_VISIBLE | WS_BORDER, 75,35,255,20,hwnd,0,hInstance,0);
      }
  else 
      {
      TCHAR name[255]="\0"; 
      DWORD size = 255; 
      char szDomain[512]="\0"; 
      GetUserName(name, &size);
      GetEnvironmentVariable("UserDomain",szDomain,127);
      if (strlen(szDomain)>0)
         {
          strncat(szDomain, "\\", 512);
         }
      strncat(szDomain, name, 512);
      
      hwEdtLogin = CreateWindowEx(0,"EDIT", szDomain, WS_CHILD | WS_DISABLED | WS_VISIBLE | WS_BORDER, 75,35,255,20,hwnd,0,hInstance,0);
      }
  
  hwEdtPasswd = CreateWindowEx(0,"EDIT","", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 75,60,255,20,hwnd,0,hInstance,0);
  hwBtnSend = CreateWindowEx(0,"Button", langstring[12],WS_CHILD | WS_VISIBLE | SS_CENTER, 10,95,320,25,hwnd,0,hInstance,0);
  SetFocus(hwEdtPasswd);

  
  //Помещаем окно в центр рабочего стола
  RECT rc,rcDesk;
  SystemParametersInfo(SPI_GETWORKAREA,0,&rcDesk,0);
  GetWindowRect(hwnd,&rc);
  MoveWindow(hwnd,(rcDesk.right-rc.right+rc.left)/2, (rcDesk.bottom-rc.bottom+rc.top)/2, rc.right-rc.left, rc.bottom-rc.top,0);

  // THIS ADDS THE ICON TO THE STATUS BAR
  m_NotifyIconData.cbSize = sizeof m_NotifyIconData;
  m_NotifyIconData.hWnd = hwnd;
  m_NotifyIconData.uID = 10;
  m_NotifyIconData.uFlags = NIF_ICON|NIF_TIP;
  m_NotifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DISCONNECTED));
  strcpy(m_NotifyIconData.szTip, "Auth Gateway Client");
  m_NotifyIconData.uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
  m_NotifyIconData.uCallbackMessage = WM_SHELLNOTIFY;
  //Всплывающее меню для трея
  hPopupMenu = CreatePopupMenu();
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_RESTORE, langstring[19]);
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_CONNECTION, langstring[10]);
  EnableMenuItem(hPopupMenu, WM_MENU_CONNECTION, MF_BYCOMMAND || MF_GRAYED);
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_CLOSE, langstring[20]);
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_ABOUT, langstring[15]);

  //Показываем окно
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

 
  while(GetMessage(&msg,NULL,0,0))
  {
    if( ( WM_KEYDOWN == msg.message ) && ( VK_RETURN  == msg.wParam  ) && ( hwEdtPasswd ==msg.hwnd))
        {
        SendMessage(hwnd, WM_COMMAND, BN_CLICKED, (int)hwBtnSend);
        continue;
        }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

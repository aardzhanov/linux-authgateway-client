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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ShowResponse(char * respstr, long timeint){

  SetWindowText(hwEdtPasswd, "");

  if (!strcmp(respstr, "ACCEPT")){
      m_NotifyIconData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_CONNECTED));               
      Shell_NotifyIcon(NIM_MODIFY, &m_NotifyIconData);
      SendMessage(hwnd,WM_SYSCOMMAND, SC_MINIMIZE, 0);
      SetWindowText(hwStatusStatic, "Доступ разрешен");
      SetTimer(hwnd, 1, timeint*1000, NULL);
      status=2;
      EnableWindow(hwBtnSend, TRUE);
  }
  else if (!strcmp(respstr, "REJECT")){
       SetWindowText(hwStatusStatic, "Неверное сочетание логина и пароля");
       SetTimer(hwnd, 2, 5000, NULL);
  }

  else if (!strcmp(respstr, "INVALID")){
       SetWindowText(hwStatusStatic, "Неверный запрос");
       SetTimer(hwnd, 2, 5000, NULL);
  }

  else if (!strcmp(respstr, "NORESPONSE")){
       SetWindowText(hwStatusStatic, "Сервер авторизации не отвечает");
       SetTimer(hwnd, 2, 5000, NULL);
  }
  else {
       SetWindowText(hwStatusStatic, "Некорректный ответ шлюза");
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
      SetWindowText(hwStatusStatic, "Не могу инициализировать библиотеку WinSock2");
      return -1;
    }

    // Шаг 2 - создание сокета

    my_sock=socket(AF_INET,SOCK_STREAM,0);
    if (my_sock < 0)
    {
      EnableWindow(hwEdtPasswd, TRUE);
      EnableWindow(hwBtnSend, TRUE);
      SetWindowText(hwStatusStatic, "Ошибка создания сокета");
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
         SetWindowText(hwStatusStatic, "Невозможно разрешить имя в IP");
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
       SetWindowText(hwStatusStatic, "Ошибка соединения с сервером");
       closesocket(my_sock);
       WSACleanup();
       return -1;
       }

     
    WSAAsyncSelect(my_sock, hwnd,  SOCKET_READY, FD_READ | FD_CLOSE);
    SetWindowText(hwBtnSend, "Отключиться");
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
       SetWindowText(hwStatusStatic, "Соединение разорвано");
       EnableWindow(hwEdtPasswd, TRUE);
      }
  status=0;
  SetWindowText(hwBtnSend, "Подключиться");
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
            else if (status==1)
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
         if (IDNO==MessageBox(hwnd, "Вы действительно хотите выйти из программы?", "Выход", MB_YESNO | MB_ICONQUESTION))
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
                         MessageBox(hwnd, "Автор: Арджанов Антон\nE-Mail: AArdzhanov@krasnodar.kes.ru\nТелефон: (861) 224-03-13\nКраснодар, 2006", "О программе", MB_OK | MB_ICONINFORMATION);
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
       MessageBox(hwnd, "Не могу определить настройки\nПроверьте наличие файла authclient.ini", "Message", MB_ICONERROR);
       return 2;
      }



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
  hwLoginStatic=CreateWindowEx(0,"Static","Логин: ", WS_CHILD | WS_VISIBLE , 10,35,60,20,hwnd,0,hInstance,0);
  hwPasswdStatic=CreateWindowEx(0,"Static","Пароль:", WS_CHILD | WS_VISIBLE, 10,60,60,20,hwnd,0,hInstance,0);
  hwStatusStatic=CreateWindowEx(0,"Static","Введите данные", WS_CHILD | WS_VISIBLE | SS_CENTER, 10,8,320,20,hwnd,0,hInstance,0);

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
  hwBtnSend = CreateWindowEx(0,"Button","Подключиться",WS_CHILD | WS_VISIBLE | SS_CENTER, 10,95,320,25,hwnd,0,hInstance,0);
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
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_RESTORE, "Развернуть");
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_CONNECTION, "Отключить");
  EnableMenuItem(hPopupMenu, WM_MENU_CONNECTION, MF_BYCOMMAND || MF_GRAYED);
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_CLOSE, "Закрыть");
  AppendMenu(hPopupMenu, MF_STRING, WM_MENU_ABOUT, "О программе");

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

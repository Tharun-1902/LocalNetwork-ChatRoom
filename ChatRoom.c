#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h> //accessing files in Windows and Windows API
#include <commdlg.h> //common dialog to pass terminal lines
#include <winsock2.h> // socket programming and socket creation
#include <ws2tcpip.h> //TCP protocol and rules
#include <iphlpapi.h> //IP protocol(network details +

char* OpenFileDialog(HWND hwnd) { //Windows api.
    static char filePath[MAX_PATH] = "";

    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;// ties the file dialog to your main application window.
    ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.txt\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = sizeof(filePath);
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;// User can only choose files in valid paths|| User must select an existing file (not a new one).

    if (GetOpenFileName(&ofn)) {
        return filePath;
    } else {
        return NULL;
    }
}


DWORD WINAPI SrecvThread(LPVOID lpParam) {
    SOCKET sock = *(SOCKET*)lpParam;
    char buffer[4096];
    int bytesReceived;

    while (1) {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            printf("\nConnection closed or error.\n");
            break;
        }

        buffer[bytesReceived] = '\0';  // Null-terminate for safety in string comparison

        // Check if receiving a file
        if (strcmp(buffer, "/file") == 0) {
            long filesize;
            long totalReceived = 0;
            char filename[1024];

            int filenameLen;
            recv(sock, (char*)&filenameLen, sizeof(int), 0);// Receive length

            recv(sock, filename, filenameLen, 0);
            printf("%s",filename);
            printf("%ld",filesize);
            // Receive file size
            recv(sock, (char*)&filesize, sizeof(filesize), 0);



            // Open file to write
            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                printf("Error opening file to write!\n");
                continue;
            }

            // Receive file data and write it directly to the file (binary)
            while (totalReceived < filesize) {
                bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0) break;
                fwrite(buffer, 1, bytesReceived, fp); // Only write binary data to the file
                totalReceived += bytesReceived;
            }

            fclose(fp);
            printf("\nFile '%s' received successfully!\n", filename);
            printf("\nMessage:");
        }
        else if (strcmp(buffer, "/exit") == 0) {
            printf("\nExit command received. Closing...\n");
            return 0;
        }
        else {
            // Regular text message, print it
            printf("\nUser:%s\n", buffer);
            printf("\nMessage:");
            fflush(stdout); // To ensure the prompt is properly displayed
        }
    }

    return 0;
}

DWORD WINAPI CrecvThread(LPVOID lpParam) {
    SOCKET sock = *(SOCKET*)lpParam;
    char buffer[4096];
    int bytesReceived;

    while (1) {
        bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            printf("\nConnection closed or error.\n");
            break;
        }

        buffer[bytesReceived] = '\0';  // Null-terminate for safety in string comparison

        // Check if receiving a file
        if (strcmp(buffer, "/file") == 0) {
            long filesize;
            long totalReceived = 0;
            char filename[1024];

            int filenameLen;
            recv(sock, (char*)&filenameLen, sizeof(int), 0);// Receive length

            recv(sock, filename, filenameLen, 0);

            // Receive file size
            recv(sock, (char*)&filesize, sizeof(filesize), 0);

            // Open file to write
            FILE *fp = fopen(filename, "wb");
            if (!fp) {
                printf("Error opening file to write!\n");
                continue;
            }

            // Receive file data and write it directly to the file (binary)
            while (totalReceived < filesize) {
                bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0) break;
                fwrite(buffer, 1, bytesReceived, fp); // Only write binary data to the file
                totalReceived += bytesReceived;
            }

            fclose(fp);
            printf("\nFile '%s' received successfully!\n", filename);
            printf("\nMessage:");
        }
        else if (strcmp(buffer, "/exit") == 0) {
            printf("\nExit command received. Closing...\n");
            printf("\nMessage:");
            return 0;
        }
        else {
            // Regular text message, print it
            printf("\nUser:%s\n", buffer);
            printf("\nMessage:");
            fflush(stdout); // To ensure the prompt is properly displayed
        }
    }

    return 0;
}

void GetLocalIPAddress(char* ipBuffer, int bufferSize) {
    DWORD bufLen = 15000;
    IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(bufLen);

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, addresses, &bufLen) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES* adapter = addresses; adapter != NULL; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp)
                continue;

            // Only use Wireless LAN adapter (Wi-Fi)
            if (adapter->IfType == IF_TYPE_IEEE80211) {
                for (IP_ADAPTER_UNICAST_ADDRESS* ua = adapter->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
                    struct sockaddr_in* sa = (struct sockaddr_in*)ua->Address.lpSockaddr;
                    inet_ntop(AF_INET, &(sa->sin_addr), ipBuffer, bufferSize);
                    free(addresses);
                    return;
                }
            }
        }
    }

    free(addresses);
    strcpy(ipBuffer, "Not Found");
}

void ServerHere() // USED IN SERVER
{
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(udp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    char buffer[1024];
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);

    int bytes = recvfrom(udp_socket, buffer, sizeof(buffer)-1, 0, (struct sockaddr*)&client_addr, &addr_len);

    buffer[bytes] = 0;
    if (strcmp(buffer, "DISCOVER_SERVER") == 0) {
    char ip[INET_ADDRSTRLEN];
    GetLocalIPAddress(ip, sizeof(ip));
    sendto(udp_socket, ip, strlen(ip), 0,(struct sockaddr*)&client_addr, addr_len);
    }
}

char* DiscoverServer() // USED IN CLIENT
{
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    BOOL broadcast = TRUE;
    setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcast, sizeof(broadcast));

    struct sockaddr_in broadcast_addr;
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(8888);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char* message = "DISCOVER_SERVER";
    sendto(udp_socket, message, strlen(message), 0, (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    static char servip[1024];
    struct sockaddr_in from_addr;
    int addr_len = sizeof(from_addr);

    char bytes = recvfrom(udp_socket, servip, sizeof(servip)-1, 0, (struct sockaddr*)&from_addr, &addr_len);
    if (bytes > 0) {
        servip[(int)bytes] = '\0';
        }
    return servip;
}

void ChatServer()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
    }

    char ip[INET_ADDRSTRLEN];
    GetLocalIPAddress(ip, sizeof(ip));

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int port = 6060;
    struct sockaddr_in info;

    info.sin_family = AF_INET;
    info.sin_port = htons(port);
    info.sin_addr.s_addr = inet_addr(ip);

    if (listen_socket == INVALID_SOCKET) {
        printf("Socket Error!\n");
        WSACleanup();
    }

    if (bind(listen_socket, (struct sockaddr*)&info, sizeof(info)) == SOCKET_ERROR) {
        printf("Socket Binding Error!\n");
        closesocket(listen_socket);
        WSACleanup();
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listing Error\n");
        closesocket(listen_socket);
        WSACleanup();
    }

    ServerHere();

    printf("Chat Room Code::%d\n",port);

    SOCKET client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        printf("Accept Failure!\n");
        closesocket(listen_socket);
        WSACleanup();
    }

    printf("User connected!\n");

    CreateThread(NULL, 0, SrecvThread, &client_socket, 0, NULL);

    char buffer[4096];
    char data[4096];
    int bytesRead;
    while (1) {
        printf("\nMessage: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "/file")==0) {  // Use strcmp to compare strings
            char* selectedFile = OpenFileDialog(NULL);
            FILE *fp = fopen(selectedFile ,"rb");
            fseek(fp, 0, SEEK_END);
            long filesize = ftell(fp);
            fseek(fp,0,SEEK_SET);
            send(client_socket, "/file", strlen("/file"), 0);
            char* filename = strrchr(selectedFile, '\\');
            if (filename) {filename++;}
            int filenameLen = strlen(filename) + 1; // include null terminator
            send(client_socket, (char*)&filenameLen, sizeof(int), 0);        // Send length
            send(client_socket, filename, filenameLen, 0);                   // Send actual filename
            send(client_socket, (char*)&filesize, sizeof(filesize),0);
            Sleep(1000);
            while ((bytesRead = fread(data, 1, sizeof(data), fp)) > 0) {
                send(client_socket, data, bytesRead, 0);
                }
            printf("\nFile sent successfully!!");
            fclose(fp);
        }
        else if(strcmp(buffer,"/exit")==0){
            exit(0);
        }
        else {
            send(client_socket, buffer, strlen(buffer), 0);
            }
        }

    closesocket(client_socket);
    closesocket(listen_socket);
    WSACleanup();
}

void ChatClient()
{
    WSADATA wsaData;
    int status;
    if ((status = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", status);
    }

    char ip[INET_ADDRSTRLEN];
    GetLocalIPAddress(ip, sizeof(ip));

    struct sockaddr_in res;
    SOCKET listsoc;
    int port = 5050;

    res.sin_family = AF_INET;
    res.sin_port = htons(port);
    res.sin_addr.s_addr = inet_addr(ip);

    listsoc=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listsoc == INVALID_SOCKET){
        printf("Socket error");
        closesocket(listsoc);
        WSACleanup();
    }

    if(bind(listsoc, (struct sockaddr*)&res, sizeof(res)) == INVALID_SOCKET){
        printf("BIND failed");
        closesocket(listsoc);
        WSACleanup();
    }
    int serport;
    char* server_ip = DiscoverServer();
    printf("\nEnter the code::");
    scanf("%d",&serport);

    struct sockaddr_in ser;

    ser.sin_family = AF_INET;
    ser.sin_port = htons(serport);
    ser.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(listsoc, (struct sockaddr*)&ser, sizeof(ser)) == SOCKET_ERROR){
        printf("Connection Failed!");
        closesocket(listsoc);
        WSACleanup();
    }
    system("cls");
    printf("User Connected");
    CreateThread(NULL, 0, CrecvThread, &listsoc, 0, NULL);

    char buffer[4096];
    char data[4096];
    int bytesRead;
    while (1) {
        printf("\nMessage:");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "/file")==0) {  // Use strcmp to compare strings
            char* selectedFile = OpenFileDialog(NULL);
            FILE *fp = fopen(selectedFile ,"rb");
            fseek(fp, 0, SEEK_END);
            long filesize = ftell(fp);
            fseek(fp,0,SEEK_SET);
            send(listsoc, "/file", strlen("/file"), 0);
            char* filename = strrchr(selectedFile, '\\');
            if (filename) {filename++;}
            int filenameLen = strlen(filename) + 1; // include null terminator
            send(listsoc, (char*)&filenameLen, sizeof(int), 0);        // Send length
            send(listsoc, filename, filenameLen, 0);
            send(listsoc, (char*)&filesize, sizeof(filesize),0);
            Sleep(100);
            while ((bytesRead = fread(data, 1, sizeof(data), fp)) > 0) {
                send(listsoc, data, bytesRead, 0);
                }
            printf("\nFile sent successfully!!");
            fclose(fp);
        }
        else if(strcmp(buffer,"/exit")==0){
            exit(0);
        }
        else {
            send(listsoc, buffer, strlen(buffer), 0);
            }
        }
    closesocket(listsoc);
    WSACleanup();
}

int main()
{
    system("cls");
    int op;
    printf("=======The LocalNetwork ChatRoom=======");
    printf("\n1.Create a ChatRoom");
    printf("\n2.Join a ChatRoom");
    printf("\nYour Option::");
    scanf("%d",&op);
    system("cls");
    if(op == 1)
    {
        ChatServer();
    }
    else if(op == 2)
    {
        ChatClient();
    }
    else{printf("Wrong option entered!!!");}
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define SERVER_PORT 12345
#define QUEUE_SIZE 5




void send_file(char* filename, int socket)
{
  int bytes_read;
  int bytes_written;
  char buffer[80];
  FILE* file = fopen(filename, "rb");

  //переводим указатель текущей позиции файла на конец
  fseek(file, 0L, SEEK_END);
  //определяем текущую позицию файла
  size_t size = ftell(file);
  //на начало
  fseek(file, 0L, SEEK_SET);

  // send file size
  write(socket, &size, sizeof(size_t));//дескриптор сокета 

  do {
    bytes_read = fread(buffer, 1, sizeof(buffer), file);//куда читаем, размер в байтах сколько хотим прочитать, максимальный размер для чтения
    bytes_written = write(socket, buffer, bytes_read);
  } while (bytes_read > 0);
  fclose(file);
}

void *thread_func(void *sock)
{
  int asock = *((int*)sock);
  char buffer[80];
  //читаем данные из сокета
  int error = recv(asock, buffer, sizeof(buffer), 0);//дескритор сокета,,, 0-означает, что данные удаляются из сокета 

  // send file
  send_file(buffer, asock);
  close(asock);
}

int main (int argc, char *argv[])
{
  int error, on;
  int listen_sd;

  int accept_sd;
  char buffer[80];
  struct sockaddr_in addr;
  pid_t child_pid;

  // create listening socket
  // domain: AF_INET(AF_UNIX)определяет набор испоьзуемых протоколов
  // socket type: SOCK_STREAM(SOCK_DGRAM) с устновлением соединения/без
  // protocol: 0(TCP for Stream; UDP for DGRAM)определяет конктреный протокол 0-задается протокол по умолчанию для sock_stream - tcp
  listen_sd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sd < 0) {
    perror("socket() failed");
    exit(-1);
  }

  // set socket options
  // protocol level: SOL_SOCKET
  error = setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

  // setoption error
  if (error < 0) {
    perror("setsockopt() failed");
    close(listen_sd);
    exit(-1);
  }
  //устанавливает значение памяти в 0
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);//переводит порядок байт в сетевой
  addr.sin_port = htons(SERVER_PORT);
  // bind a name to a socket
  error = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
  
  if (error < 0) {
    perror("bind() failed");
    close(listen_sd);
    exit(-1);
  }

  //listen for socket connections and limit the queue of incoming connections
  error = listen(listen_sd, QUEUE_SIZE);//слушаем сокет
  if (error < 0) {
    perror("listen() failed");
    close(listen_sd);
    exit(-1);
  }

  printf("The server is ready\n");
  while (1) {
    // accept a new connection on a socket
    accept_sd = accept(listen_sd, NULL, NULL);//информация о принятом сокете не возвращается
    if (accept_sd < 0) {
      perror("accept() failed");
      close(listen_sd);
      exit(-1);
    }
    // here start new thread
    pthread_t pthread;
    if (thread_def) {
      pthread_create(&pthread, NULL, &thread_func, (void*)&accept_sd);//null - атрибуты 
    } else {	
      child_pid = fork();
      if(child_pid == 0) {
        thread_func((void*)&accept_sd);
      }
    }
  }
  close(listen_sd);
  return 0;
}
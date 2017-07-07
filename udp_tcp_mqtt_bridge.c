/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "MQTTPacket.h"

#define BUFSIZE 1024
#define MQTT_PORT 1883
#define UDP_PORT 8888

sem_t receivedUdpMutex,receivedTcpMutex,firstConnectMutex;
char sharedUdpBuffer[BUFSIZE];
char sharedTcpBuffer[BUFSIZE];
int udp_mqtt_len = 0;

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        printf("Caught signal SIGPIPE %d\n",signum);
}

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
}

void die(char *s)
{
    perror(s);
    exit(1);
}

void *UDPServer(void *port){
  struct sockaddr_in si_me, si_other;

  int s, i, slen = sizeof(si_other) , recv_len,true;

  //create a UDP socket
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
      die("socket");
  }
  true = 1;
  setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));

  // zero out the structure
  memset((char *) &si_me, 0, sizeof(si_me));

  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(UDP_PORT);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket to port
  if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
  {
      die("bind");
  }

  //keep listening for data
  while(1)
  {
      printf("Waiting for data...");
      fflush(stdout);

      //try to receive some data, this is a blocking call
      if ((recv_len = recvfrom(s, sharedUdpBuffer, BUFSIZE, 0, (struct sockaddr *) &si_other, &slen)) == -1)
      {
          die("recvfrom()");
      }

      //print details of the client/peer and the data received
      udp_mqtt_len = (int)sharedUdpBuffer[1] + 2;
      sem_post (&receivedUdpMutex);
      sem_post (&firstConnectMutex);


      //now reply the client with the  data from the tcp server
      sem_wait (&receivedTcpMutex);
      if (sendto(s, sharedTcpBuffer, strlen(sharedTcpBuffer), 0, (struct sockaddr*) &si_other, slen) == -1)
      {
          die("sendto()");
      }
  }

  close(s);
}

void *TCPClient(void *args){
  int sock,true,reconnectFlag;
  struct sockaddr_in server;
  char message[1000] , server_reply[2000];


  //Create socket
  sem_wait (&firstConnectMutex);
  RECONNECT:
  sock = socket(AF_INET , SOCK_STREAM , 0);
  if (sock == -1)
  {
      printf("Could not create socket");
  }

  true = 1;
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
  puts("Socket created");

  server.sin_addr.s_addr = inet_addr("192.168.1.5");
  server.sin_family = AF_INET;
  server.sin_port = htons(MQTT_PORT);

  //Connect to remote server

  if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
  {
      perror("connect failed. Error");
      return 1;
  }

  puts("Connected\n");

  //keep communicating with server
  while(1)
  {
      if(!reconnectFlag)
        sem_wait (&receivedUdpMutex);
      else
        reconnectFlag = 0;

      //Send some data
      if( send(sock , sharedUdpBuffer , udp_mqtt_len , 0) < 0)
      {
          puts("Send failed");
          reconnectFlag = 1;
          goto RECONNECT;
      }
      memset(sharedUdpBuffer,0,BUFSIZE);

      //Receive a reply from the server
      if( recv(sock , sharedTcpBuffer , BUFSIZE , 0) < 0)
      {
          puts("recv failed");
      }
      sem_post (&receivedTcpMutex);
      puts("TCP Server reply :");
      puts(sharedTcpBuffer);
  }

  close(sock);
}

int main(int argc, char **argv) {

   /* check command line arguments */
   if (argc != 3) {
      fprintf(stderr,"usage: %s <MQTT Broker Addr> <udp bridge server port>\n", argv[0]);
      exit(0);
   }
  signal(SIGPIPE, signal_callback_handler);
  sem_init(&receivedUdpMutex, 0, 0);
  sem_init(&receivedTcpMutex, 0, 0);
  sem_init(&firstConnectMutex,0, 0);
  pthread_t udp_pth_id,tcp_pth_id;	// this is our thread identifier
  int i = 0;

  /* Create worker thread */
  pthread_create(&udp_pth_id,NULL,UDPServer,argv);
  pthread_create(&tcp_pth_id,NULL,TCPClient,argv);

  /* wait for our thread to finish before continuing */
  pthread_join(udp_pth_id,NULL);
  pthread_join(tcp_pth_id,NULL);

  return 0;
}

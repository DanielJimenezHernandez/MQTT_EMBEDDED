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

sem_t receivedUdpMutex,receivedTcpMutex,tcpMsgWait;
unsigned char sharedUdpBuffer[BUFSIZE];
unsigned char sharedTcpBuffer[BUFSIZE];
int udp_mqtt_len = 0;
int first_run_flag = 1;
int reconnectFlag = 1;
unsigned char disconnect_package[2] = {0xe0,0x00};

char * strBrokerAddr,strBrokerPort,strUdpPort;
int brokerPort;
int udpPort;
int *s_out, *slen_out;
struct sockaddr_in * si_other_out;
int flag_send_reconnect = 0;
char ReconnectPackage = {0xff,0x01};

/* Catch Signal Handler functio */
void signal_callback_handler(int signum){

        flag_send_reconnect = 1;
        printf("Caught signal SIGPIPE Sending error to client %d\n",signum);

}

void printMessage(unsigned char *buf,int len,const char* description){
  int i;
  printf("%s -> [0x%02x]\n",description,buf[0]);
  for (i = 0; i < len; i++){
    printf("[0x%02x]",buf[i]);
  }
  printf("\n");
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

  int s, slen = sizeof(si_other) , recv_len,true;

  s_out = &s;
  slen_out = &slen;
  si_other_out = &si_other;

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
  si_me.sin_port = htons(udpPort);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket to port
  if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
  {
      die("bind");
  }

  //keep listening for data
  while(1)
  {
      //printf("Waiting for data...");
      fflush(stdout);
      if (flag_send_reconnect){
        printf("Sending Reconnect Package\n");
        if (sendto(s, ReconnectPackage, 2, 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
        flag_send_reconnect = 0;
      }
      //try to receive some data, this is a blocking call
      if ((recv_len = recvfrom(s, sharedUdpBuffer, BUFSIZE, 0, (struct sockaddr *) &si_other,(socklen_t *) &slen)) == -1)
      {
          die("recvfrom()");
      }

      sem_post (&receivedUdpMutex);
      //print details of the client/peer and the data received
      udp_mqtt_len = (int)sharedUdpBuffer[1] + 2;
      printMessage(sharedUdpBuffer,udp_mqtt_len,"UDP Recieved");
      if (first_run_flag){
        first_run_flag = 0;
      }

      //now reply the client with the  data from the tcp server
      //If The command arrived from UDP is for a connect expect response ACK
      if ((sharedUdpBuffer[0] >> 4) == CONNECT){
        sem_wait (&receivedTcpMutex);
        printMessage(sharedTcpBuffer,(int)sharedTcpBuffer[1],"UDP Send");
        if (sendto(s, sharedTcpBuffer, (int)sharedTcpBuffer[1], 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
        /* wait for tcp message to be sent Syn Issue */

    }
    sem_wait(&tcpMsgWait);
  }

  close(s);
}

void *TCPClient(void *args){
  int sock,true;
  struct sockaddr_in server;


  //Create socket
  while(first_run_flag);
  sock = socket(AF_INET , SOCK_STREAM , 0);
  if (sock == -1)
  {
      printf("Could not create socket");
  }

  true = 1;
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));
  puts("Socket created");

  server.sin_addr.s_addr = inet_addr(strBrokerAddr);
  server.sin_family = AF_INET;
  server.sin_port = htons(brokerPort);

  //Connect to remote server


  //keep communicating with server
  while(1)
  {
      if(reconnectFlag){
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
            perror("connect failed. Error");
        }
        reconnectFlag = 0;
      }
      //Send some data
      sem_wait (&receivedUdpMutex);
      printMessage(sharedUdpBuffer,udp_mqtt_len,"TCP Send");
      if( send(sock , sharedUdpBuffer , udp_mqtt_len , 0) < 0)
      {
          puts("Send failed");
      }
      sem_post(&tcpMsgWait);
      /*stop the udp thread until the mesage is sent*/

      //Receive a reply from the server
      if ((sharedUdpBuffer[0] >> 4) == CONNECT){
        if( recv(sock , sharedTcpBuffer , BUFSIZE , 0) < 0)
        {
            puts("TCP: recv failed");
        }
        printMessage(sharedTcpBuffer,sharedTcpBuffer[1],"TCP Recieved");
        sem_post (&receivedTcpMutex);
      }
  }

  close(sock);
}

int main(int argc, char **argv) {

   /* check command line arguments */
   if (argc != 4) {
      fprintf(stderr,"usage: %s <MQTT Broker Addr> <MQTT Broker port> <udp bridge server port>\n", argv[0]);
      exit(0);
   }
  strBrokerAddr = argv[1];

  brokerPort = atoi(argv[2]);
  udpPort = atoi(argv[3]);

  signal(SIGPIPE, signal_callback_handler);
  sem_init(&receivedUdpMutex, 0, 0);
  sem_init(&receivedTcpMutex, 0, 0);
  sem_init(&tcpMsgWait,0, 0);
  pthread_t udp_pth_id,tcp_pth_id;	// this is our thread identifier

  /* Create worker thread */
  pthread_create(&udp_pth_id,NULL,UDPServer,argv);
  pthread_create(&tcp_pth_id,NULL,TCPClient,argv);

  /* wait for our thread to finish before continuing */
  pthread_join(udp_pth_id,NULL);
  pthread_join(tcp_pth_id,NULL);

  return 0;
}

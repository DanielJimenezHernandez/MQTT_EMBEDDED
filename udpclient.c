/*
    Simple udp client
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include "MQTTPacket.h"

#define SERVER "192.168.1.88"
#define BUFLEN 1024  //Max length of buffer
#define PORT 8888   //The port on which to send data
#define PAYLOAD "Hello"
#define TOPIC "a/b"

void die(char *s)
{
    perror(s);
    exit(1);
}

enum states{
  NOT_CONNECTED,
  CONNECTED,
  PUBLISHED
};
 enum states state = NOT_CONNECTED;

void printMessage(unsigned char *buf,int len,const char* description){
  int i;
  printf("%s\n",description);
  for (i = 0; i < len; i++){
    printf("[0x%02x]",buf[i]);
  }
  printf("\n");
}

int main(void)
{
    struct sockaddr_in si_other;
    int s, slen=sizeof(si_other),len_connect,len_publish;
    char buf[BUFLEN];
    char message[BUFLEN];
    unsigned char MQTTPacket[BUFLEN];
    unsigned char MQTTPucblish[BUFLEN];
    int packetType;
    int packetLen;
    MQTTPacket_connectData *connect_options;
    MQTTPacket_publishData *publish_options;

    /*Connect packet serialization example */
    MQTTPacket_connectData default_options_connect = MQTTPacket_connectData_initializer;
    default_options_connect.MQTTVersion = 3;
    default_options_connect.keepAliveInterval = 10;
    default_options_connect.clientID.cstring = "FRDM-K64F";
    connect_options = &default_options_connect;
    len_connect = MQTTSerialize_connect(MQTTPacket,BUFLEN,connect_options);

    /*publish packet serialization example */
    MQTTPacket_publishData default_options_publish = MQTTPacket_publishData_initializer;
    publish_options = &default_options_publish;
    publish_options->topicName.cstring = TOPIC;
    publish_options->payload = (unsigned char *)PAYLOAD;
    publish_options->payloadlen = strlen((const char *)PAYLOAD);
    len_publish = MQTTSerialize_publish_opt(MQTTPucblish,BUFLEN,publish_options);


    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);

    if (inet_aton(SERVER , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while(1)
    {
        printf("Enter to send packet: ");
        fgets( message, 10, stdin );

        //send the connect message
        if (state == NOT_CONNECTED){
          printMessage(MQTTPacket,len_connect,"Sending Connect:");
          if (sendto(s, MQTTPacket, len_connect , 0 , (struct sockaddr *) &si_other, slen)==-1)
          {
              die("sendto()");
          }
          printMessage(MQTTPucblish,len_publish,"Sending PuConnect:");
          if (sendto(s, MQTTPucblish, len_publish , 0 , (struct sockaddr *) &si_other, slen)==-1)
          {
              die("sendto()");
          }
        }

        /* Send Publish Messages */
        if (state == CONNECTED){
          printMessage(MQTTPucblish,len_publish,"Sending Publish:");
          if (sendto(s, MQTTPucblish, len_publish , 0 , (struct sockaddr *) &si_other, slen)==-1)
          {
              die("sendto()");
          }
          state = PUBLISHED;
        }
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (state != PUBLISHED){
          if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, (socklen_t *)&slen) == -1)
          {
              die("recvfrom()");
          }
          packetType = ((int)buf[0]) >> 4;
          switch (packetType){
            case CONNACK:
              packetLen = buf[1];
              printMessage((unsigned char *)buf,packetLen,"CONNACK;");
              if ((packetLen == 2) && buf[2] == 0 && buf[3] == 0){
                state = CONNECTED;
                }
              else
                printf("connection rejected try again\n");
              break;
            case PUBACK:
              printf("Message Correctly Published");
              packetLen = buf[1];
            default:
              break;
          }
        }
        state = CONNECTED;
    }

    close(s);
    return 0;
}

/*
    Simple udp client
*/
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include "MQTTPacket.h"

#define SERVER "192.168.1.88"
#define BUFLEN 1024  //Max length of buffer
#define PORT 8888   //The port on which to send data
#define PAYLOAD "HELLO"
#define TOPIC "a/b"

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    struct sockaddr_in si_other;
    int s, i, slen=sizeof(si_other),len_connect;
    char buf[BUFLEN];
    char message[BUFLEN];
    unsigned char MQTTPacket[BUFLEN];
    MQTTPacket_connectData *connect_options;
    MQTTPacket_publishData *publish_options;

    /*Connect packet serialization example */
    MQTTPacket_connectData default_options_connect = MQTTPacket_connectData_initializer;
    default_options_connect.MQTTVersion = 4;
    default_options_connect.keepAliveInterval = 10;
    default_options_connect.clientID.cstring = "FRDM-K64F";
    connect_options = &default_options_connect;
    len_connect = MQTTSerialize_connect(MQTTPacket,BUFLEN,connect_options);
#if 0
    /*publish packet serialization example */
    MQTTPacket_publishData default_options_publish = MQTTPacket_publishData_initializer;
    publish_options = &default_options_publish;
    publish_options->topicName.cstring = TOPIC;
    publish_options->payload = PAYLOAD;
    publish_options->payloadlen = strlen((const char *)PAYLOAD);
    len_connect = MQTTSerialize_publish_opt(MQTTPacket,BUFLEN,publish_options);
#endif
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
        gets(message);

        //send the message
        if (sendto(s, MQTTPacket, len_connect , 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
            die("sendto()");
        }

        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            die("recvfrom()");
        }

        puts(buf);
    }

    close(s);
    return 0;
}

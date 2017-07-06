CC=gcc
MQTT_DIR=mqtt
CFLAGS=-I$(MQTT_DIR) -g -Wall

TESTAPP = testapp

OBJS = $(MQTT_DIR)/MQTTConnectClient.o \
$(MQTT_DIR)/MQTTUnsubscribeServer.o \
$(MQTT_DIR)/MQTTSerializePublish.o \
$(MQTT_DIR)/MQTTSubscribeServer.o \
$(MQTT_DIR)/MQTTConnectServer.o \
$(MQTT_DIR)/MQTTDeserializePublish.o \
$(MQTT_DIR)/MQTTPacket.o \
$(MQTT_DIR)/MQTTUnsubscribeClient.o \
$(MQTT_DIR)/MQTTFormat.o \
$(MQTT_DIR)/MQTTSubscribeClient.o \
test.o

DEPS = $(MQTT_DIR)/MQTTConnectClient.c \
$(MQTT_DIR)/MQTTUnsubscribeServer.c \
$(MQTT_DIR)/MQTTSerializePublish.c \
$(MQTT_DIR)/MQTTSubscribeServer.c \
$(MQTT_DIR)/MQTTConnectServer.c \
$(MQTT_DIR)/MQTTDeserializePublish.c \
$(MQTT_DIR)/MQTTPacket.c \
$(MQTT_DIR)/MQTTUnsubscribeClient.c \
$(MQTT_DIR)/MQTTFormat.o \
$(MQTT_DIR)/MQTTSubscribeClient.o \
test.o



$(TESTAPP): $(OBJS)
	gcc -o $@ $(OBJS) $(CFLAGS)

.PHONY: clean

clean:
	-rm ./*.o
	-rm ./$(MQTT_DIR)/*.o
	-rm $(TESTAPP)

CC = gcc
CFLAGS = -Wall

WINDOWS_FLAGS := -lws2_32
LINUX_FLAGS :=

BUILD_DIR := build

ifeq ($(OS), Windows_NT)
	EXE_EXT := .exe
	FILE_SUFFIX := windows
	LDLIBS := $(WINDOWS_FLAGS)
	RM_BUILD_DIR := del /s /q $(subst /,\,$(BUILD_DIR)\*) # / or // ?
else
	EXE_EXT :=
	FILE_SUFFIX := linux
	LDLIBS := $(LINUX_FLAGS)
	RM_BUILD_DIR := rm -rf $(BUILD_DIR)/*
endif

APP_LAUNCHER_С := src/app_launcher/app_launcher_$(FILE_SUFFIX).c
NETWORK_MANAGER_С := src/network_manager/network_manager_$(FILE_SUFFIX).c
SERVER_C := src/remote_app_launch_server.c
CLIENT_C := src/remote_app_launch_client.c

APP_LAUNCHER_O := $(BUILD_DIR)/app_launcher_$(FILE_SUFFIX).o
NETWORK_MANAGER_O := $(BUILD_DIR)/network_manager_$(FILE_SUFFIX).o
SERVER_O := $(BUILD_DIR)/server_$(FILE_SUFFIX).o
CLIENT_O := $(BUILD_DIR)/client_$(FILE_SUFFIX).o

.PHONY: all clean help
all: client server

server: $(SERVER_O) $(NETWORK_MANAGER_O)
	@echo "------ Make $(@) ------"
	$(CC) $(LDLIBS) $^ -o $(BUILD_DIR)/remote_app_launch_server$(EXE_EXT)

client: $(CLIENT_O) $(NETWORK_MANAGER_O) $(APP_LAUNCHER_O)
	@echo "------ Make $(@) ------"
	$(CC) $(LDLIBS) $^ -o $(BUILD_DIR)/remote_app_launch_client$(EXE_EXT)

$(NETWORK_MANAGER_O): $(NETWORK_MANAGER_С) | $(BUILD_DIR)
	@echo "------ Compile $(@) ------"
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(APP_LAUNCHER_O): $(APP_LAUNCHER_С) | $(BUILD_DIR)
	@echo "------ Compile $(@) ------"
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(SERVER_O): $(SERVER_C) | $(BUILD_DIR)
	@echo "------ Compile $(@) ------"
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(CLIENT_O): $(CLIENT_C) | $(BUILD_DIR)
	@echo "------ Compile $(@) ------"
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(BUILD_DIR):
	@echo "------ Make build dir $(@) ------"
	mkdir -p $(BUILD_DIR)

clean:
	@echo "------ Clean build dir------"
	$(RM_BUILD_DIR)

help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all         Build server and client"
	@echo "  server      Build server remote_app_launch_server"
	@echo "  client      Build client remote_app_launch_client"
	@echo "  clean       Remove build files"
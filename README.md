# Task:
Client/Server application <b> to launch and track application on the remote host </b>.
- server will request to launch application with `<application name>`
- the client has to launch an application on his host. (Ex `Notepad.exe`)
- the client has to replay with success or failure
- the client has to replay with application status `<Running>` each second
- when the application is closed by another application Client has to replay to the server also.
- server can request to close the application with `<application name>`

## Requirements:
The following approaches are required:
- Console application.
- “`make`” file to compile application.
- Language/libraries: C/C++, usage of additional libraries or 3rd party code is prohibited. 
For threading, networking should be used platform-dependent API. (details can be discussed individually with an assigned mentor).
- Application should be cross-compile, e.g. should be compilable on Linux OS (Ubuntu 18.04) and Windows OS (Windows 10) without any code modifications.
- It's allowed to use C99 and C11.

Application logic must be divided between Network and System API.
- Platform-dependent code must be in a separate module.
- Network-dependent code must be in a separate module.

# Installation
## Linux
1. Download and install gcc if it is not already installed on your computer.
2. Clone this repository onto your computer.
3. Navigate to the project's directory.
4. Clean build directory if present: `make clean`.
5. Choose target to compile:
   - `make all` - compile both client and server.
   - `make server` - compile server.
   - `make client` - compile client.

- Additional targets:
   - `make help` - print list of targets.
   - `make clean` - remove build files.

## Windows
Support for windows in progress.

# Usage
## Server
1. Start the server on one machine: `./build/remote_app_launch_server`.
2. Wait for clients to connect. Clients will send their unique name. 
3. Available commands:
   - Write `[client name] run [/path/to/application] [arg1] [arg2] [...]` 
   to run `/path/to/application` with 0 or more arguments on client's machine.
   - Write `[client name] kill [pid]` to stop execution of application with process id `pid`.
   - Write `[client name] kill all` to disconnect client and stop execution of all his applications.
4. Write `q` to disconnect all clients and stop server execution.

## Client
1. Start the client on another machine or terminal: `./build/remote_app_launch_client`. Client will send to server status of all his running application every second.
2. Wait for response from server with next command. Client will run/kill application.
3. Client will stop when server send required command: `[client name] kill all` or `q`.

# Requirements:
The following approaches are required:
- Console application.
- “make” file to compile application.
- Language/libraries: C/C++, usage of additional libraries or 3rd party code is prohibited. For threading, networking should be used platform-dependent API. (details can be discussed individually with an assigned mentor).
- Application should be cross-compile, e.g. should be compilable on Linux OS (Ubuntu 18.04) and Windows OS (Windows 10) without any code modifications.
- It's allowed to use C99 and C11.


Application logic must be divided between Network and System API.
- Platform-dependent code must be in a separate module.
- Network-dependent code must be in a separate module.

# Task:
Client/Server application <b> to launch and track application on the remote host </b>.
- server will request to launch application with \<application name>
- the client has to launch an application on his host. (Ex Notepad.exe)
- the client has to replay with success or failure
- the client has to replay with application status \<Running> each second
- when the application is closed by another application Client has to replay to the server also.
- server can request to close the application with \<application name>

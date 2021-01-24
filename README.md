# CppServer
Socket and HTTP clients and servers for C++

# Todo
## HTTP
- Add the different statuses (like not found, internal server error)
- Format the status to accept content type, length, etc
- Read the request from the socket to get the method (GET, POST) and data
- After that, implement all that data into an easily accessable form in request
- (ie, proper http headers) Possibly use send/recv (sys/socket.h) instead of
- write/read to reduce size Handle for empty buffers Ex of above: in a
- webbrowser, go to index, go to home, push back button, refresh, and push...
- foward button
- Look at throwing errors rather than returning error codes
- Implement a file server
- Implement option to either set thread count of pool or start thread per conn
- Change from read and write (unistd.h) to recv and send (socket.h)
- Possibly use map for content type in write file method
- Implement websocket
- Use socket net_socket.hpp
## Socket
- Add UDP
- Add shutdown read and write individually
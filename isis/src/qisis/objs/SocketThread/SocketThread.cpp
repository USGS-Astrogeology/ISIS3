#include "SocketThread.h"
#include "IException.h"
#include "Application.h"

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <QObject>
#include <QThread>

#include "IString.h"

namespace Isis {

  /**
   * Constructor for the SocketThread
   *
   *
   * @param parent
   */
  SocketThread::SocketThread(QObject *parent): QThread(parent) {
  }

  //! Destroys the SocketThread object
  SocketThread::~SocketThread() {
    // wait();
  }

  //! Starts the socket thread
  void SocketThread::run() {
    std::string p_socketFile = ("/tmp/isis_qview_" + Application::UserName()).toLatin1().data();
    struct sockaddr_un p_socketName;
    p_socketName.sun_family = AF_UNIX;
    strcpy(p_socketName.sun_path, p_socketFile.c_str());
    int p_socket;

    // Create a socket
    if((p_socket = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
      std::string msg = "Unable to create socket";
      std::cerr << msg << std::endl;
      remove(p_socketFile.c_str());
      return;
    }

    // Setting a timeout didn't work for Mac, so we're using a non-blocking mode
    //   instead.
    fcntl(p_socket, F_SETFL, O_NONBLOCK);

    // Bind the file to the socket
    int status =  bind(p_socket, (struct sockaddr *)&p_socketName, sizeof(p_socketName));
    if(status < 0) {
      std::string msg = "Unable to bind to socket [" + p_socketFile + "]";
      std::cerr << msg << std::endl;
      remove(p_socketFile.c_str());
      return;
    }

    // Set up to listen to the socket
    if(listen(p_socket, 5) < 0) {
      std::string msg = "Unable to listen to socket [" + p_socketFile + "]";
      std::cerr << msg << std::endl;
      remove(p_socketFile.c_str());
      return;
    }

    p_done = false;

    while(!p_done) {
      // Accept Socket
      socklen_t len = sizeof(&p_socketName);
      int childSocket = accept(p_socket, (struct sockaddr *)&p_socketName, &len);
      if (childSocket < 0)
        if (errno == EWOULDBLOCK) {
          msleep(100);
        continue; // probably timed out, we cant do anything about this anyways
      }

      // Receive Data
      int bytes;
      // This used to be char buf[1024*1024]; but when that line existed the
      //   mac OS's would crash unpredictably, even when the code on that
      //   line wasn't executed.
      QScopedPointer< char, QScopedPointerArrayDeleter<char> > buf(
          new char[1024*1024]);
      if((bytes = recv(childSocket, buf.data(), 1024 * 1024, 0)) < 0) {
        std::string msg = "Unable to read from socket [" + p_socketFile + "]";
        std::cerr << msg << std::endl;
        remove(p_socketFile.c_str());
        return;
      }

      // Push everything onto our string buffer
      IString buffer;
      for(int i = 0; i < bytes; i++) buffer += buf.data()[i];
      QChar escape(27);
      IString escape2 = QString(escape).toStdString();
      while(buffer.size() > 0) {
        IString token = buffer.Token(escape2);
        if(token == "raise") {
          emit focusApp();
        }
        else emit newImage(token.c_str());
      }
    };
  }
}

#include<Python.h>
#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>



#define SOCKET_PATH "/tmp/igs-host-control"
#define BUFLEN 1024



FILE *sink;

/* unix socket client */
static int sock;
static char buf[BUFLEN];

//pthread_t t;
//pthread_attr_t attr;
//int do_loop;

static PyObject *
ipcbridge_send(PyObject *self, PyObject *args)
{
   const char *msg;
   int ret = 0;

   //convert python type to C type
   if (!PyArg_ParseTuple(args, "s", &msg))
      return NULL;

   //send stuff away return code in ret
   ret = send(sock, msg, strnlen(msg, BUFLEN), 0);

   Py_RETURN_NONE;
   //convert C type to python type
   //return PyLong_FromLong(ret);
}

/* TODO read latest and discard the rest - only newest value interesting */
static PyObject *
ipcbridge_read(PyObject *self)
{
    int nbytes;

    Py_BEGIN_ALLOW_THREADS 
    nbytes = read(sock, buf, BUFLEN - 1);
    Py_END_ALLOW_THREADS

    if (nbytes < 0)
    {
       PyErr_SetString(PyExc_RuntimeError, "read()");
    }
    else if (nbytes == 0)
    {
       /* end of connection */
       PyErr_SetString(PyExc_RuntimeError, "connection lost");
    }
    else
    {
       /* data read */
       buf[nbytes] = '\0';
       goto success;
    }
     
   return NULL;

   success:
   return PyUnicode_FromString(buf);
}



static PyMethodDef IpcbridgeMethods[] = {
   {"send", ipcbridge_send, METH_VARARGS, "send data over IPC."},
   {"read", ipcbridge_read, METH_VARARGS, "recv data over IPC."},
   {NULL, NULL, 0, NULL}
};


static struct PyModuleDef ipcbridgemodule = {
   PyModuleDef_HEAD_INIT,
   "ipcbridge",
   NULL, /*module doc*/
   -1, /* size of per-interpreter state of the module, -1 module keeps state in global var */
   IpcbridgeMethods
};


/* initialization handler when module gets loaded */
static int init_handler(void)
{
   struct sockaddr_un s;
   int err;
   char locstr[BUFLEN];

   sock = socket(AF_UNIX, SOCK_STREAM, 0);
   if (sock < 0) {
      PyErr_SetString(PyExc_RuntimeError, "socket()");
      return 1;
   }

   s.sun_family = AF_UNIX;
   strncpy(s.sun_path, SOCKET_PATH, strlen(SOCKET_PATH));
  

   /* module import hangs until successful connect - easy approach */
   err = 1;
   while(err)
   {
      if(connect(sock, (struct sockaddr *) &s, sizeof(struct sockaddr_un)) < 0 )
      {
         err = errno;
         if(err != ENOENT)
         {
            snprintf(locstr, BUFLEN, "%d %s", errno, strerror(errno)); 
            PyErr_SetString(PyExc_RuntimeError, locstr);
            return 2;
         }
         //if path doesn't exist - server was not started, yet TODO module import hangs...
         else
         {
            //retry timeout
            sleep(3);
         }
      } 
      else
      {
         err = 0;
      }
   }


   sink = fopen("/tmp/ipcbridge.debug", "w");
   fprintf(sink, "LOADMODULE sock %d", sock);
   fflush(sink);

   return 0;
}

/* cleanup handler when interpreter shuts down / module gets unloaded */
static void exit_handler(void)
{

   close(sock);

   fprintf(sink, "EXITMODULE");
   fclose(sink);

   return;
}



PyMODINIT_FUNC
PyInit_ipcbridge(void)
{
   if (Py_AtExit(exit_handler))
   {
      PyErr_SetString(PyExc_RuntimeError, "Registering exit handler failed");
      return NULL;
   }
   
   if(init_handler())
   {
      return NULL;
   }

   return PyModule_Create(&ipcbridgemodule);
}

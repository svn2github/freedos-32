#include <dr-env.h>
typedef signed long LONG;
typedef signed char CHAR;
#include <devices.h>


typedef enum MouseEvent
{
    NOEVENT = 0,
    WM_LBUTTONDOWN,
    WM_LBUTTONUP,
    WM_RBUTTONDOWN,
    WM_RBUTTONUP,
    WM_MOUSEMOVE,
    WM_MOUSEWHEEL
}
MouseEvent;
static const char *eventstr[] =
{
    "no",
    "WM_LBUTTONDOWN",
    "WM_LBUTTONUP",
    "WM_RBUTTONDOWN",
    "WM_RBUTTONUP",
    "WM_MOUSEMOVE",
    "WM_MOUSEWHEEL"
};
static MouseEvent event = NOEVENT;
static MouseData  prev;


static MouseCallback cb;
static void cb(const MouseData *data)
{
    MouseEvent e = WM_MOUSEMOVE;
    if (data->buttons != prev.buttons)
    {
             if ( (data->buttons & MOUSE_LBUT) && !(prev.buttons & MOUSE_LBUT)) e = WM_LBUTTONDOWN;
        else if (!(data->buttons & MOUSE_LBUT) &&  (prev.buttons & MOUSE_LBUT)) e = WM_LBUTTONUP;
        else if ( (data->buttons & MOUSE_RBUT) && !(prev.buttons & MOUSE_RBUT)) e = WM_RBUTTONDOWN;
        else if (!(data->buttons & MOUSE_RBUT) &&  (prev.buttons & MOUSE_RBUT)) e = WM_RBUTTONUP;
    }
    else if (((data->flags & MOUSE_AXES) > 2) && (data->axes[3] != prev.axes[3])) e = WM_MOUSEWHEEL;
    prev = *data;
    event = e;
}


void mouse_test_init(void)
{
    int             hdev;
    int             res;
    fd32_request_t *request;

    hdev = fd32_dev_search("mouse");
    if (hdev < 0) return;// hdev;
    res = fd32_dev_get(hdev, &request, NULL, NULL, 0);
    if (res < 0) return;// res;
    res = request(FD32_MOUSE_SETCB, cb);
    if (res < 0) return;// res;

    for (;;)
    {
        WFC(!event);
        fd32_message("dx=%li dy=%li but=%lu %s\n",
                     prev.axes[MOUSE_X], prev.axes[MOUSE_Y], prev.buttons, eventstr[event]);
        event = 0;
    }
}

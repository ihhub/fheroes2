#include <Python.h>
#include "screen.h"
#include "game/main_lib.h"
#include "localevent.h"
#include <SDL_events.h>
#include <vector>

static PyObject *py_start_game(PyObject *, PyObject *args)
{
    PyObject *listObj = nullptr;
    if ( !PyArg_ParseTuple(args, "O!", &PyList_Type, &listObj) )
        return nullptr;

    const Py_ssize_t argc = PyList_Size(listObj);
    std::vector<char *> argv(argc);
    for ( Py_ssize_t i = 0; i < argc; ++i ) {
        PyObject *item = PyList_GetItem(listObj, i);
        argv[i] = const_cast<char *>( PyUnicode_AsUTF8(item) );
    }

    const int res = fheroes2_main(static_cast<int>(argc), argv.data());
    return PyLong_FromLong(res);
}

static PyObject *py_send_key(PyObject *, PyObject *args)
{
    int key = 0;
    int pressed = 0;
    if ( !PyArg_ParseTuple(args, "ii", &key, &pressed) )
        return nullptr;

    SDL_Event e{};
    e.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.sym = key;
    SDL_PushEvent(&e);
    Py_RETURN_NONE;
}

static PyObject *py_move_mouse(PyObject *, PyObject *args)
{
    int x = 0;
    int y = 0;
    if ( !PyArg_ParseTuple(args, "ii", &x, &y) )
        return nullptr;

    SDL_Event e{};
    e.type = SDL_MOUSEMOTION;
    e.motion.x = x;
    e.motion.y = y;
    SDL_PushEvent(&e);
    Py_RETURN_NONE;
}

static PyObject *py_mouse_button(PyObject *, PyObject *args)
{
    int button = 0;
    int pressed = 0;
    int x = 0;
    int y = 0;
    if ( !PyArg_ParseTuple(args, "iiii", &button, &pressed, &x, &y) )
        return nullptr;

    SDL_Event e{};
    e.type = pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    e.button.button = button == 1 ? SDL_BUTTON_LEFT : button == 2 ? SDL_BUTTON_RIGHT : SDL_BUTTON_MIDDLE;
    e.button.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    e.button.x = x;
    e.button.y = y;
    SDL_PushEvent(&e);
    Py_RETURN_NONE;
}

static PyObject *py_capture(PyObject *, PyObject *)
{
    fheroes2::Display &display = fheroes2::Display::instance();
    const uint8_t *img = display.image();
    const int w = display.width();
    const int h = display.height();
    const size_t size = static_cast<size_t>(w) * h * (display.singleLayer() ? 1 : 2);
    return Py_BuildValue("y#ii", img, size, w, h);
}

static PyMethodDef module_methods[] = {
    { "start_game", py_start_game, METH_VARARGS, "Start the game" },
    { "send_key", py_send_key, METH_VARARGS, "Send a keyboard event" },
    { "move_mouse", py_move_mouse, METH_VARARGS, "Move mouse cursor" },
    { "mouse_button", py_mouse_button, METH_VARARGS, "Send mouse button event" },
    { "capture", py_capture, METH_NOARGS, "Capture display buffer" },
    { nullptr, nullptr, 0, nullptr }
};

static struct PyModuleDef moduledef = { PyModuleDef_HEAD_INIT, "pyfheroes2", nullptr, -1, module_methods };
PyMODINIT_FUNC PyInit_pyfheroes2(void)
{
    PyObject *m = PyModule_Create(&moduledef);
    if (!m)
        return nullptr;

    PyModule_AddIntConstant(m, "KEY_LEFT", static_cast<int>(fheroes2::Key::KEY_LEFT));
    PyModule_AddIntConstant(m, "KEY_RIGHT", static_cast<int>(fheroes2::Key::KEY_RIGHT));
    PyModule_AddIntConstant(m, "KEY_UP", static_cast<int>(fheroes2::Key::KEY_UP));
    PyModule_AddIntConstant(m, "KEY_DOWN", static_cast<int>(fheroes2::Key::KEY_DOWN));
    return m;
}

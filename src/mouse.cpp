#include "mouse.hpp"

void Mouse::setButton( int button, int state ) {
    for (unsigned int i=0;i<buttons.size();i++ ) {
        if ( buttons[i].x == button ) {
            buttons[i].y = state;
            return;
        }
    }
    buttons.push_back(glm::ivec2(button,state));
}

int Mouse::getButton( int button ) {
    for (unsigned int i=0;i<buttons.size();i++ ) {
        if ( buttons[i].x == button ) {
            return buttons[i].y;
        }
    }
    return 0;
}

glm::vec2 Mouse::getMousePos() {
    Window root, child;
    int mx, my;
    int wx, wy;
    unsigned int mask;
    XQueryPointer( x11->display, x11->root, &root, &child, &mx, &my, &wx, &wy, &mask );
    return glm::vec2( mx, my );
}

void Mouse::setCursor( int cursor ) {
    if ( currentCursor == cursor ) {
        return;
    }
    XFreeCursor( x11->display, xcursor );
    xcursor = XCreateFontCursor( x11->display, cursor );
    XChangeActivePointerGrab( x11->display,
                              PointerMotionMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask,
                              xcursor, CurrentTime );
}

Mouse::Mouse(X11* x11, bool nodecorations, Window ignoreWindow ) {
    this->x11 = x11;
    currentCursor = XC_cross;
    xcursor = XCreateFontCursor( x11->display, XC_cross );
    hoverWindow = None;
    XGrabPointer( x11->display, x11->root, True,
                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask,
                  GrabModeAsync, GrabModeAsync, None, xcursor, CurrentTime );
    this->nodecorations = nodecorations;
    this->ignoreWindow = ignoreWindow;
    hoverWindow = findWindow(x11->root);
}

Mouse::~Mouse() {
	XUngrabPointer( x11->display, CurrentTime );
}

void Mouse::update() {
    XEvent event;
    while ( XCheckTypedEvent( x11->display, ButtonPress, &event ) ) {
		setButton( event.xbutton.button, 1 );
	}
    bool findNewWindow = false;
    while ( XCheckTypedEvent( x11->display, MotionNotify, &event ) ) {
        findNewWindow = true;
	}
    if ( findNewWindow ) {
        hoverWindow = findWindow(x11->root);
    }
    while ( XCheckTypedEvent( x11->display, ButtonRelease, &event ) ) {
		setButton( event.xbutton.button, 0 );
	}
    while ( XCheckTypedEvent( x11->display, EnterNotify, &event ) ) {
        hoverWindow = event.xcrossing.window;
	}
}

Window Mouse::findWindow( Window foo ) {
    glm::vec2 pos = getMousePos();
    Window root, parent;
    Window* children;
    unsigned int nchildren;
    Window selectedWindow;
    XQueryTree( x11->display, foo, &root, &parent, &children, &nchildren );
    // The children are ordered, so we traverse backwards.
    if ( !children || nchildren <= 0 ) {
        return foo;
    }
    for( int i=nchildren-1;i>=0;i-- ) {
        if ( children[i] == ignoreWindow ) {
            continue;
        }
        // We need to make sure the window isn't something that currently isn't mapped.
        XWindowAttributes attr;         
        XGetWindowAttributes( x11->display, children[i], &attr );
        if ( attr.map_state != IsViewable ) {
            continue;
        }
        glm::vec4 rect = getWindowGeometry(children[i], false);
        float a = pos.x - rect.x;
        float b = pos.y - rect.y;
        if ( a <= rect.z && a >= 0 ) {
            if ( b <= rect.w && b >= 0 ) {
                selectedWindow = children[i];
                if ( !nodecorations ) {
                    XFree(children);
                    return selectedWindow;
                } else {
                    XFree(children);
                    return findWindow( selectedWindow );
                }
            }
        }
    }
    return foo;
}